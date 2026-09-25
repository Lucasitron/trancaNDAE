// senhas_store.h — Tabela limitada de chaves autorizadas (RAM + NVS).
//
// Desenho anti-estouro de memória:
// - Capacidade fixa: MAX_SENHAS (20) tokens de 4 hex em RAM estática
//   (20 x 5 bytes = 100 bytes), sem alocação dinâmica e sem String acumulada.
// - O token é o HMAC-SHA256("ABRIR", PIN) truncado nos 2 primeiros bytes
//   (4 hex). PIN de 4 dígitos (PIN_LEN).
// - A revogação (bloquear/excluir/expirar) é representada por AUSÊNCIA
//   do token na lista do Firebase. Não existe blocklist que cresce:
//   bloquear = remover da lista = slot liberado.
// - NVS espelha a RAM (chaves "n", "t00".."t19") para sobreviver a reboot.
#ifndef SENHAS_STORE_H
#define SENHAS_STORE_H

#include <Arduino.h>

#define MAX_SENHAS 20
#define TOKEN_HEX_LEN 4
#define PIN_LEN 4

// Carrega a tabela da NVS para a RAM. Chame no setup().
void senhas_init();

// Substitui a tabela pelo CSV "tok1,tok2,..." vindo do Firebase.
// Ignora tokens malformados, remove duplicados e limita a MAX_SENHAS.
// Retorna true se a tabela mudou (para a UI sinalizar).
bool senhas_sync_csv(const String &csv);

// Consulta a tabela em RAM (O(MAX_SENHAS), sem I/O).
int senhas_total();
bool senhas_contem(const String &token);

// Flag consumível: true uma vez após cada mudança via sync.
bool senhas_consumirMudanca();

#endif
