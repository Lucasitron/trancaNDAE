
// firebase_client.cpp — Firebase em modo assíncrono (mobizt/FirebaseClient).
//
// Arquitetura (senhas SÓ no ESP, nada persistido no RTDB):
// - A web escreve comandos VOLÁTEIS em /comandos/dispositivo1:
//     "1234"   -> cadastra o PIN na tabela local (RAM+NVS)
//     "LIMPAR" -> apaga TODA a tabela local
// - O ESP consome via stream e APAGA o nó (Database.remove) em seguida:
//   nada de senha permanece no database.
// - A abertura é SOMENTE pelo teclado com PIN local. Sem token em NVS.
//
// Auth: o app roda SEM login por padrão (NoAuth), pois database.rules.json é
// público. Para endurecer as Rules (auth != null), descomente a linha abaixo
// e preencha USER_EMAIL/USER_PASSWORD em secrets.h (economiza flash sem auth).
// #define FIREBASE_USE_USER_AUTH

#define ENABLE_DATABASE
#if defined(FIREBASE_USE_USER_AUTH)
#define ENABLE_USER_AUTH
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "firebase_client.h"
#include "senhas_store.h"
#include "telnet_log.h"
#include "wifi_manager.h"

#include "secrets.h"

// ================= OBJETOS =================
// WiFiClientSecure do core Arduino (FirebaseClient já sabe usar).
// Cada cliente TLS consome heap (~dezenas de KB). Usamos o MÍNIMO:
// auth (aClient) + stream (streamClient) + manutenção (maintClient, só
// para apagar o comando). Os metadados ficam no Firebase, geridos pela
// página — o ESP não publica resumo.
WiFiClientSecure ssl_client;
WiFiClientSecure stream_ssl_client;
WiFiClientSecure maint_ssl_client;

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client);
AsyncClient maintClient(maint_ssl_client);

// Autenticação (ver FIREBASE_USE_USER_AUTH no topo do arquivo).
// Padrão = NoAuth: `app.ready()` fica true de imediato e o ESP não fica
// mudo se o usuário do Firebase Auth não existir.
#if defined(FIREBASE_USE_USER_AUTH)
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000);
#else
NoAuth no_auth;
#endif

// Aplicação e Database
FirebaseApp app;
RealtimeDatabase Database;

// true após initializeApp. Firebase só sobe com Wi-Fi (idempotente).
static bool firebaseIniciado = false;
// Stream de comandos inicia só depois do app pronto (ready()).
static bool streamIniciado = false;
// Apagar o nó é feito NO LOOP (fora do callback): a lib não admite
// iniciar tarefa async dentro do próprio callback (reentrância = reboot).
static bool apagarComandoPendente = false;
// Idem para reconectar o Wi-Fi (bloqueia ~20s): via flag, no loop.
static bool wifiReconfig = false;

// Declarada antes do callback (definição abaixo).
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

        // Stream caiu (rede/cliente): rearma para recriá-lo no próximo loop.
        if (aResult.uid() == "streamTask")
            streamIniciado = false;
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

    // Marca para apagar no loop (ver processarFirebase). O comando é
    // processado AGORA (só RAM/NVS, sem reentrância) e o nó some em seguida:
    // janela mínima de exposição no database.
    apagarComandoPendente = true;
    consumirComando(valor);
}

// true se o valor começa com ADMIN_PASSWORD, sozinho ou seguido de ':'
// (sem montar "ADMIN_PASSWORD:" em uma String temporária).
static bool comecaComAdmin(const String &v)
{
    const size_t n = strlen(ADMIN_PASSWORD);
    if (v.length() < n || memcmp(v.c_str(), ADMIN_PASSWORD, n) != 0)
        return false;
    return v.length() == n || v[n] == ':';
}

// Separa "a:b:c" em até 3 campos; os ausentes ficam vazios.
static void dividirCampos(const String &s, String out[3])
{
    int ini = 0;
    for (uint8_t i = 0; i < 3; i++)
    {
        int fim = s.indexOf(':', ini);
        if (fim < 0)
        {
            out[i] = s.substring(ini);
            return;
        }
        out[i] = s.substring(ini, fim);
        ini = fim + 1;
    }
}

// Interpreta um comando volátil (chamado após marcar o nó para apagar).
static void consumirComando(const String &valor)
{
    if (valor == COMANDO_LIMPAR)
    {
        if (senhas_limpar())
            tlogln("Comando remoto: tabela zerada.");
        else
            tlogln("Comando remoto LIMPAR: tabela já vazia.");
        return;
    }

    // Nunca aceitar a senha de admin como chave de abertura.
    if (comecaComAdmin(valor))
    {
        tlogln("Comando remoto recusado (senha de admin).");
        return;
    }

    // Formato "CMD:a:b".
    String f[3];
    dividirCampos(valor, f);

    if (f[0] == "DEL" && f[1].length() > 0)
    {
        if (!senhas_remover_nome(f[1]))
            tlogln("DEL ignorado (nome inexistente).");
        return;
    }

    if (f[0] == "WIFI2OFF")
    {
        wifi2_desativar();
        return;
    }

    if (f[0] == "WIFI2" && f[1].length() > 0)
    {
        // f[2] pode ser "" (rede aberta). Aplica no loop (bloqueia ~20s).
        if (wifi2_salvar(f[1], f[2]))
            wifiReconfig = true;
        else
            tlogln("WIFI2 ignorado (SSID 1-32 / senha 0-63, sem ':').");
        return;
    }

    if ((f[0] == "BLOQ" || f[0] == "LIB") && f[1].length() > 0)
    {
        if (!senhas_bloquear(f[1], f[0] == "BLOQ"))
            tlogln("BLOQ/LIB ignorado (nome inexistente/sem mudança).");
        return;
    }

    if (f[0] == "RENOVAR" && f[1].length() > 0 && f[2].length() > 0)
    {
        if (!senhas_renovar(f[1], f[2]))
            tlogln("RENOVAR ignorado (nome ausente/novo inválido).");
        return;
    }

    // Cadastro: "PIN:nome" (nome obrigatório).
    if (!senhas_adicionar(f[0], f[1]))
        tlogln("Comando remoto ignorado (formato/duplicado/cheio).");
}

// ================= INICIALIZAÇÃO =================
void iniciarFirebase()
{
    if (firebaseIniciado)
        return; // idempotente: não reinicializa o app

    ssl_client.setInsecure();
    stream_ssl_client.setInsecure();
    maint_ssl_client.setInsecure();
    // NOTA: NetworkClientSecure (core novo) não tem setBufferSizes().

#if defined(FIREBASE_USE_USER_AUTH)
    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
#else
    initializeApp(aClient, app, getAuth(no_auth));
#endif

    app.getApp(Database);
    Database.url(DATABASE_URL);

    // O streaming inicia em processarFirebase() com app.ready() == true.
    streamIniciado = false;
    firebaseIniciado = true;

    tlogln("Firebase pronto (modo assíncrono).");
}

// ================= LOOP =================
void processarFirebase()
{
    if (!firebaseIniciado)
        return; // só roda depois de iniciarFirebase() (exige Wi-Fi)

    // Sem rede não há stream a manter (o Wi-Fi desconectado já rearmou o
    // stream via firebase_reset_stream()). Evita retry-loop offline.
    if (!wifiConectado)
        return;

    app.loop();

    // Auth caiu/expirada (modo login) ou app não pronto: rearma o stream
    // para recriá-lo assim que voltar. Com NoAuth, ready() é sempre true.
    if (streamIniciado && !app.ready())
        streamIniciado = false;

    if (!streamIniciado && app.ready())
    {
        streamIniciado = true;
        Database.get(streamClient, COMANDO_UNICO_PATH, processData, true, "streamTask");
        tlogln("Stream de comandos iniciado.");
    }

    // Apaga o comando consumido (fora do callback, sem reentrância).
    // Garante que o PIN não permanece no database.
    if (streamIniciado && app.ready() && apagarComandoPendente)
    {
        apagarComandoPendente = false;
        Database.remove(maintClient, COMANDO_UNICO_PATH, processData, "deleteTask");
    }

    // Aplica troca de Wi-Fi pedida pela página (fora do callback).
    if (wifiReconfig)
    {
        wifiReconfig = false;
        wifi_reconectar();
    }
}

// Pronto quando autenticado (porta de entrada do modo status).
bool firebase_pronto()
{
    return firebaseIniciado && app.ready();
}

// Força recriação do stream na próxima volta (após queda/reconexão Wi-Fi).
void firebase_reset_stream()
{
    streamIniciado = false;
}
