// senhas_store.h — Tabela local de chaves (RAM + NVS). Só existe no ESP.
//
// Cada chave: PIN (4 dígitos) + nome (etiqueta única) + ativa (bloqueio).
// Os metadados (data, validade, status) ficam no Firebase, gerenciados
// pela página — o ESP não os publica nem lê.
// O PIN NUNCA sai do ESP.
//
// - Capacidade fixa em RAM estática, sem alocação dinâmica.
// - Revogar = BLOQ/DEL/LIMPAR. Sem blocklist que cresce.
// - NVS espelha a RAM ("n", "t%02d", "n%02d", "a%02d").
#ifndef SENHAS_STORE_H
#define SENHAS_STORE_H

#include <Arduino.h>

#define MAX_SENHAS 20
#define TOKEN_HEX_LEN 4 // tamanho do slot (= PIN_LEN dígitos)
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
