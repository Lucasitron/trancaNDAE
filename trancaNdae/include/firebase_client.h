// firebase_client.h
#ifndef TRANCA_FIREBASE_CLIENT_H
#define TRANCA_FIREBASE_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "secrets.h"

// Nó de comandos VOLÁTEIS (a web escreve, o ESP consome e apaga).
// Nome é a chave de gerência (único por tabela):
//   "PIN:nome[:epoch]"        -> cadastra (ex.: "4829:Maria:1758760000")
//   "RENOVAR:nome:novo[:epoch]" -> troca o PIN mantendo o nome
//   "DEL:nome"                -> remove uma chave
//   "LIMPAR"                  -> apaga toda a tabela local
// Nó de leitura p/ a página (SEM pin — só nome+data):
//   /resumo/dispositivo1 = {"total":N,"chaves":[{"nome":"..","criadaEm":E}]}
#define COMANDO_UNICO_PATH "/comandos/dispositivo1"
#define RESUMO_PATH "/resumo/dispositivo1"
#define COMANDO_LIMPAR "LIMPAR"

// Gerenciamento do Firebase
void iniciarFirebase();
void processarFirebase(); // Chame no loop() para manter o app vivo

// true quando autenticado (stream ativo). Para o modo status.
bool firebase_pronto();

#endif
