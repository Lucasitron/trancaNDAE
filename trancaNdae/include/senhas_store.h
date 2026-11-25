// senhas_store.h — Tabela local de chaves (RAM + NVS). Só existe no ESP.
//
// Cada chave: PIN (4 dígitos) + nome (etiqueta única) + ativa (bloqueio).
// Os metadados (data, validade, status) ficam no Firebase, gerenciados
// pela página — o ESP não os publica nem lê.
// O PIN NUNCA sai do ESP.
//
// - Capacidade fixa em RAM estática, sem alocação dinâmica.
// - PIN guardado como uint16 (0..9999): 2 bytes, em vez de 5 ("1234\0").
// - NVS: um único blob "regs" no namespace "senhas" (uma entrada, não três
//   por chave). O formato antigo (t00/n00/a00) é migrado no boot.
// - Revogar = BLOQ/DEL/LIMPAR. Sem blocklist que cresce.
#ifndef SENHAS_STORE_H
#define SENHAS_STORE_H

#include <Arduino.h>

#define MAX_SENHAS 30
#define PIN_LEN 4
#define NOME_MAX_LEN 24

// Carrega a tabela da NVS para a RAM. Chame no setup().
void senhas_init();

// Adiciona PIN+nome. Nome deve ser único. Ignora inválido/duplicado/lotado.
// Retorna true se a tabela mudou.
bool senhas_adicionar(const String &pin, const String &nome);

// Remove pela etiqueta (nome). Retorna true se existia.
bool senhas_remover_nome(const String &nome);

// Troca o PIN da etiqueta, mantendo o nome. Retorna true se trocou.
bool senhas_renovar(const String &nome, const String &pinNovo);

// Bloqueia (ativa=0) / libera (ativa=1). Retorna true se mudou.
bool senhas_bloquear(const String &nome, bool bloquear);

// Apaga TODA a tabela (RAM + NVS). Retorna true se havia algo.
bool senhas_limpar();

// Consulta a tabela em RAM (sem I/O). contem() exige ativa==1.
int senhas_total();
int senhas_total_ativas();
bool senhas_contem(const String &pin);

// Flag consumível: true uma vez após cada mudança.
bool senhas_consumirMudanca();

#endif
