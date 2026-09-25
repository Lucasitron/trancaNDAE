// firebase_client.h
#ifndef TRANCA_FIREBASE_CLIENT_H
#define TRANCA_FIREBASE_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Preferences.h>
#include "secrets.h"


// Nó com a lista compacta de chaves (CSV de tokens HMAC, 4 hex).
// Gerenciado pela página web (web/js/models/SenhasModel.js).
#define SENHAS_LISTA_PATH "/senhas/dispositivo1/lista"

// Nó legado de comando único (mantido para compatibilidade/emergência).
#define COMANDO_UNICO_PATH "/comandos/dispositivo1"

// Funções de gerenciamento do Firebase
void iniciarFirebase();
void processarFirebase(); // Chame no loop() para manter o app vivo

// Funções de NVS (via Preferences)
void salvar_token_nvs(const String &token);
String ler_token_nvs();
void limpar_token_nvs();

#endif