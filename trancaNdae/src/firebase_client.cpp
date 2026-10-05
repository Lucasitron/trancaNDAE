
// firebase_client.cpp — Firebase em modo assíncrono (mobizt/FirebaseClient).
//
// Arquitetura (senhas SÓ no ESP, nada persistido no RTDB):
// - A web escreve comandos VOLÁTEIS em /comandos/dispositivo1:
//     "1234"   -> cadastra o PIN na tabela local (RAM+NVS)
//     "LIMPAR" -> apaga TODA a tabela local
// - O ESP consome via stream e APAGA o nó (Database.remove) em seguida:
//   nada de senha permanece no database.
// - A abertura é SOMENTE pelo teclado com PIN local. Sem token em NVS.
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "firebase_client.h"
#include "senhas_store.h"
#include "telnet_log.h"

#include "secrets.h"

// ================= OBJETOS =================
// WiFiClientSecure do core Arduino (FirebaseClient já sabe usar).
// Um cliente POR tarefa assíncrona (a lib não permite compartilhar):
//   auth  -> aClient | stream -> streamClient
//   delete do comando -> deleteClient | resumo -> resumoClient
WiFiClientSecure ssl_client;
WiFiClientSecure stream_ssl_client;
WiFiClientSecure delete_ssl_client;
WiFiClientSecure resumo_ssl_client;

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client);
AsyncClient deleteClient(delete_ssl_client);
AsyncClient resumoClient(resumo_ssl_client);

// Autenticação
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000);

// Aplicação e Database
FirebaseApp app;
RealtimeDatabase Database;

// Stream de comandos inicia só após autenticação
static bool streamIniciado = false;
static unsigned long ultimoResumo = 0;
static bool resumoPendente = false;

// Declaradas antes do callback (definições abaixo).
static void publicarResumo();
static void consumirComando(const String &valor);

// ================= CALLBACK UNIFICADO =================
// IMPORTANTE: NÃO chamar app.loop() aqui dentro (recursão -> estoura a
// pilha da loopTask: "Stack canary watchpoint triggered").
void processData(AsyncResult &aResult)
{
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n",
                        aResult.uid().c_str(),
                        aResult.eventLog().message().c_str(),
                        aResult.eventLog().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n",
                        aResult.uid().c_str(),
                        aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n",
                        aResult.uid().c_str(),
                        aResult.error().message().c_str(),
                        aResult.error().code());
    }

    if (!aResult.available())
        return;

    RealtimeDatabaseResult &stream = aResult.to<RealtimeDatabaseResult>();
    if (!stream.isStream())
        return;

    // Nó ausente/apagado: nada a fazer (estado normal após consumo).
    if (stream.type() == 0 /* null */)
        return;

    if (stream.type() != 5 /* string */)
        return;

    String valor = stream.to<String>();
    valor.trim();

    // Consome e APAGA o nó: comando volátil, não permanece no database.
    Database.remove(deleteClient, COMANDO_UNICO_PATH, processData, "deleteTask");

    consumirComando(valor);
}

// Publica o resumo (nome+data, SEM pin) para a página acompanhar.
static void publicarResumo()
{
    String json;
    senhas_resumo_json(json);
    Database.set(resumoClient, RESUMO_PATH, json, processData, "resumoTask");
}

// Interpreta um comando volátil (chamado após apagar o nó).
static void consumirComando(const String &valor)
{
    if (valor == COMANDO_LIMPAR)
    {
        if (senhas_limpar())
        {
            tlogln("Comando remoto: tabela zerada.");
            resumoPendente = true;
        }
        else
            tlogln("Comando remoto LIMPAR: tabela já vazia.");
        return;
    }

    // Nunca aceitar a senha de admin como chave de abertura.
    if (valor == ADMIN_PASSWORD || valor.startsWith(String(ADMIN_PASSWORD) + ":"))
    {
        tlogln("Comando remoto recusado (senha de admin).");
        return;
    }

    // Formato "CMD:a:b:c" — separa em até 4 campos.
    String f[4];
    int ini = 0, n = 0;
    while (n < 4)
    {
        int fim = valor.indexOf(':', ini);
        if (fim < 0)
        {
            f[n++] = valor.substring(ini);
            break;
        }
        f[n++] = valor.substring(ini, fim);
        ini = fim + 1;
    }
    while (n < 4)
        f[n++] = "";

    if (f[0] == "DEL" && f[1].length() > 0)
    {
        if (senhas_remover_nome(f[1]))
            resumoPendente = true;
        else
            tlogln("DEL ignorado (nome inexistente).");
        return;
    }

    if (f[0] == "RENOVAR" && f[1].length() > 0 && f[2].length() > 0)
    {
        uint32_t ep = (f[3].length() > 0) ? (uint32_t)f[3].toInt() : 0;
        if (senhas_renovar(f[1], f[2], ep))
            resumoPendente = true;
        else
            tlogln("RENOVAR ignorado (nome ausente/novo inválido).");
        return;
    }

    // Cadastro: "PIN:nome[:epoch]" (nome pode ser vazio).
    uint32_t ep = (f[2].length() > 0) ? (uint32_t)f[2].toInt() : 0;
    if (senhas_adicionar(f[0], f[1], ep))
        resumoPendente = true;
    else
        tlogln("Comando remoto ignorado (formato/duplicado/cheio).");
}

// ================= INICIALIZAÇÃO =================
void iniciarFirebase()
{
    ssl_client.setInsecure();
    stream_ssl_client.setInsecure();
    delete_ssl_client.setInsecure();
    resumo_ssl_client.setInsecure();
    // NOTA: NetworkClientSecure (core novo) não tem setBufferSizes().

    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");

    app.getApp(Database);
    Database.url(DATABASE_URL);

    // O streaming inicia em processarFirebase() com app.ready() == true.
    streamIniciado = false;
    ultimoResumo = 0;

    tlogln("Firebase pronto (modo assíncrono).");
}

// ================= LOOP =================
void processarFirebase()
{
    app.loop();

    if (!streamIniciado && app.ready())
    {
        streamIniciado = true;
        Database.get(streamClient, COMANDO_UNICO_PATH, processData, true, "streamTask");
        tlogln("Stream de comandos iniciado.");
        resumoPendente = true; // publica o estado atual já no próximo loop
    }

    // Publica o resumo quando há mudança (resumoPendente) ou a cada
    // 30s (auto-cura). Tudo em UM lugar => sem conflito de cliente.
    if (streamIniciado && app.ready() &&
        (resumoPendente || millis() - ultimoResumo > 30000))
    {
        resumoPendente = false;
        ultimoResumo = millis();
        publicarResumo();
    }
}

// Pronto quando autenticado (porta de entrada do modo status).
bool firebase_pronto()
{
    return app.ready();
}
