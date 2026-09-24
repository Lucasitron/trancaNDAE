// firebase_client.h
#ifndef FIREBASE_CLIENT_H
#define FIREBASE_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <SSLClient.h>
#include <Preferences.h>
#include "secrets.h"


// Funções de gerenciamento do Firebase
void iniciarFirebase();
void processarFirebase(); // Chame no loop() para manter o app vivo

// Funções de NVS (via Preferences)
void salvar_token_nvs(const String &token);
String ler_token_nvs();
void limpar_token_nvs();

#endif