// firebase_client.h
#ifndef TRANCA_FIREBASE_CLIENT_H
#define TRANCA_FIREBASE_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "secrets.h"

// Nó de comandos VOLÁTEIS (a web escreve; o ESP consome e apaga no loop,
// fora do callback — a lib não admite tarefa async dentro do callback).
// Nome é a chave de gerência (único por tabela):
//   "PIN:nome"                     -> cadastra (ex.: "4829:Maria")
//   "RENOVAR:nome:novo"            -> troca o PIN mantendo o nome
//   "BLOQ:nome" / "LIB:nome"       -> bloqueia / libera (sem apagar)
//   "DEL:nome"                     -> remove uma chave
//   "LIMPAR"                       -> apaga toda a tabela local
//   "WIFI2:ssid:senha"             -> salva Wi-Fi secundário e reconecta
//   "WIFI2OFF"                     -> desativa o secundário (só padrão)
// Metadados (nome, data, validade, status) ficam em /pessoas/{device},
// gerenciados pela PÁGINA — o ESP não publica resumo.
#define COMANDO_UNICO_PATH "/comandos/dispositivo1"
#define COMANDO_LIMPAR "LIMPAR"

// Gerenciamento do Firebase
void iniciarFirebase();
void processarFirebase(); // Chame no loop() para manter o app vivo

// true quando autenticado (porta de entrada do modo status).
bool firebase_pronto();

// Força recriação do stream na próxima volta (após queda/reconexão Wi-Fi).
void firebase_reset_stream();

#endif
