
// ================= CONFIGURAÇÃO DA BIBLIOTECA =================
// Define as funcionalidades que queremos usar
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "firebase_client.h"
#include "senhas_store.h"
#include "telnet_log.h"
#include <Preferences.h>

#include "secrets.h"


// ================= OBJETOS =================
// WiFiClientSecure do core Arduino (FirebaseClient já sabe usar).
// NÃO usar digitaldragon/SSLClient aqui: construído vazio ele deixa
// o ponteiro interno null -> "init_tcp_connection(): Client pointer is null".
WiFiClientSecure ssl_client;
WiFiClientSecure stream_ssl_client;
WiFiClientSecure senhas_ssl_client;

// Clientes assíncronos
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client);
AsyncClient senhasClient(senhas_ssl_client);

// Autenticação
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000); // Token expira em 50min

// Aplicação e Database
FirebaseApp app;
RealtimeDatabase Database;

// NVS via Preferences
Preferences preferences;

// ================= NVS (MESMO CÓDIGO DE ANTES) =================
void salvar_token_nvs(const String &token)
{
    preferences.begin("meu_app", false);
    preferences.putString("token", token);
    preferences.end();
    tlogln("✅ Token salvo na NVS: " + token);
}

String ler_token_nvs()
{
    preferences.begin("meu_app", true);
    // Guarda anti-spam: getString() com chave inexistente loga
    // "[E] getString(): nvs_get_str len fail: token NOT_FOUND" no core novo.
    // isKey() evita o erro (e o NVS read a cada 2s no loop some do monitor).
    if (!preferences.isKey("token"))
    {
        preferences.end();
        return "";
    }
    String token = preferences.getString("token", "");
    preferences.end();
    return token;
}

void limpar_token_nvs()
{
    preferences.begin("meu_app", false);
    preferences.remove("token");
    preferences.end();
    tlogln("🗑️ Token removido da NVS.");
}

// Flag de controle dos streams (iniciam só após autenticação)
static bool streamIniciado = false;
static bool senhasStreamIniciado = false;

// ================= CALLBACK UNIFICADO =================
// Esta função é chamada para TUDO: eventos, erros, debug e dados.
// IMPORTANTE: NÃO chamar app.loop() aqui dentro (causa recursão e
// estoura a pilha da loopTask -> "Stack canary watchpoint triggered").
void processData(AsyncResult &aResult)
{
    // Sai quando não há resultado (chamada a partir do loop)
    if (!aResult.isResult())
        return;

    // 1. Eventos (ex: início de conexão, desconexão)
    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n",
                        aResult.uid().c_str(),
                        aResult.eventLog().message().c_str(),
                        aResult.eventLog().code());
    }

    // 2. Debug (logs detalhados da biblioteca)
    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n",
                        aResult.uid().c_str(),
                        aResult.debug().c_str());
    }

    // 3. Erros
    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n",
                        aResult.uid().c_str(),
                        aResult.error().message().c_str(),
                        aResult.error().code());
    }

    // 4. Dados recebidos (o que nos interessa)
    if (aResult.available())
    {
        RealtimeDatabaseResult &stream = aResult.to<RealtimeDatabaseResult>();

        // Verifica se é um evento de stream
        if (stream.isStream())
        {
            bool ehListaSenhas = (aResult.uid() == "senhasTask");

            // Nó removido: lista vazia = zero chaves (fail-closed);
            // comando único vazio = nada a fazer.
            if (stream.type() == 0 /* null */)
            {
                if (ehListaSenhas)
                    senhas_sync_csv("");
                else
                    tlogln("ℹ️ Nó do Firebase vazio (null).");
                return;
            }

            // Se for uma string, processa o token / lista
            if (stream.type() == 5 /* string */)
            {
                String valor = stream.to<String>();
                if (ehListaSenhas)
                {
                    tlogf("🔑 Lista de senhas recebida (%d chars).\n", valor.length());
                    senhas_sync_csv(valor);
                }
                else
                {
                    tlog("🔐 Token recebido: ");
                    tlogln(valor);

                    if (valor.length() > 0)
                    {
                        salvar_token_nvs(valor);
                        // Aqui você pode avisar o usuário via LCD/buzzer
                    }
                }
            }
        }
    }
}

// ================= INICIALIZAÇÃO =================
void iniciarFirebase()
{
    // 0. Garante que o namespace da NVS exista (evita
    // "nvs_open failed: NOT_FOUND" no primeiro boot em modo read-only)
    preferences.begin("meu_app", false);
    preferences.end();

    // 1. Recupera token pendente da NVS (caso tenha reiniciado)
    String tokenPendente = ler_token_nvs();
    if (tokenPendente.length() > 0)
    {
        tlogln("♻️ Token pendente recuperado da NVS: " + tokenPendente);
    }

    // 2. Configura SSL (para testes, use setInsecure)
    // Em produção, use certificados específicos
    ssl_client.setInsecure();
    stream_ssl_client.setInsecure();
    senhas_ssl_client.setInsecure();
    // NOTA: NetworkClientSecure (core novo) não tem setBufferSizes();
    // esse ajuste só existe no ESP_SSLClient. Removido para compilar.

    // 3. Inicializa a autenticação e o app
    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");

    // 4. Obtém a instância do Database
    app.getApp(Database);
    Database.url(DATABASE_URL);

    // 5. Os streamings são iniciados em processarFirebase() assim que
    // app.ready() for true (evita erro "unauthenticate" + retry frenético).
    // Ver exemplo oficial StreamConcurentcy.ino.
    streamIniciado = false;
    senhasStreamIniciado = false;

    tlogln("🚀 Firebase pronto (modo assíncrono).");
}

// ================= LOOP =================
void processarFirebase()
{
    // Mantém o app rodando (processa tarefas assíncronas)
    app.loop();

    // Inicia os streams UMA vez, somente após autenticação completa
    if (!streamIniciado && app.ready())
    {
        streamIniciado = true;
        Database.get(streamClient, COMANDO_UNICO_PATH, processData, true, "streamTask");
        tlogln("📡 Stream do Firebase iniciado.");
    }
    if (!senhasStreamIniciado && app.ready())
    {
        senhasStreamIniciado = true;
        Database.get(senhasClient, SENHAS_LISTA_PATH, processData, true, "senhasTask");
        tlogln("📡 Stream da lista de senhas iniciado.");
    }
}