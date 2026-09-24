
// ================= CONFIGURAÇÃO DA BIBLIOTECA =================
// Define as funcionalidades que queremos usar
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "firebase_client.h"
#include <Preferences.h>
#include <SSLClient.h>

#include "secrets.h"


// ================= OBJETOS =================
// SSL client (pode ser ajustado para certificados específicos)
SSLClient ssl_client;
SSLClient stream_ssl_client;

// Clientes assíncronos
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client);

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
    Serial.println("✅ Token salvo na NVS: " + token);
}

String ler_token_nvs()
{
    preferences.begin("meu_app", true);
    String token = preferences.getString("token", "");
    preferences.end();
    return token;
}

void limpar_token_nvs()
{
    preferences.begin("meu_app", false);
    preferences.remove("token");
    preferences.end();
    Serial.println("🗑️ Token removido da NVS.");
}

// ================= CALLBACK UNIFICADO =================
// Esta função é chamada para TUDO: eventos, erros, debug e dados.
void processData(AsyncResult &aResult)
{
    // Mantém o app rodando (essencial para o async)
    app.loop();

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
            // Ignora valores nulos (nó removido)
            if (stream.type() == 0 /* null */)
            {
                Serial.println("ℹ️ Nó do Firebase vazio (null).");
                return;
            }

            // Se for uma string, processa o token
            if (stream.type() == 5 /* string */)
            {
                String token = stream.to<String>();
                Serial.print("🔐 Token recebido: ");
                Serial.println(token);

                if (token.length() > 0)
                {
                    salvar_token_nvs(token);
                    // Aqui você pode avisar o usuário via LCD/buzzer
                }
            }
        }
    }
}

// ================= INICIALIZAÇÃO =================
void iniciarFirebase()
{
    // 1. Recupera token pendente da NVS (caso tenha reiniciado)
    String tokenPendente = ler_token_nvs();
    if (tokenPendente.length() > 0)
    {
        Serial.println("♻️ Token pendente recuperado da NVS: " + tokenPendente);
    }

    // 2. Configura SSL (para testes, use setInsecure)
    // Em produção, use certificados específicos
    ssl_client.setInsecure();
    stream_ssl_client.setInsecure();

    // 3. Inicializa a autenticação e o app
    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");

    // 4. Obtém a instância do Database
    app.getApp(Database);
    Database.url(DATABASE_URL);

    // 5. Inicia o streaming no nó desejado
    // O último parâmetro "true" indica que é um stream contínuo
    Database.get(streamClient, "/comandos/dispositivo1", processData, true, "streamTask");

    Serial.println("🚀 Firebase pronto (modo assíncrono).");
}

// ================= LOOP =================
void processarFirebase()
{
    // Mantém o app rodando (processa tarefas assíncronas)
    app.loop();
}