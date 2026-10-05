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
// Metadados (nome, data, validade, status) ficam em /pessoas/{device},
// gerenciados pela PÁGINA — o ESP não publica resumo.
#define COMANDO_UNICO_PATH "/comandos/dispositivo1"
#define COMANDO_LIMPAR "LIMPAR"

// Gerenciamento do Firebase
void iniciarFirebase();
void processarFirebase(); // Chame no loop() para manter o app vivo

// true quando autenticado (stream ativo). Para o modo status.
bool firebase_pronto();

#endif
