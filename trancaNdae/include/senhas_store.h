// senhas_store.h — Tabela local de chaves (RAM + NVS). Só existe no ESP.
//
// Cada chave: PIN (4 dígitos) + nome (etiqueta única) + criadaEm (epoch,
// enviada pela web) + validadeH (0 = sem expiração) + ativa (bloqueio).
// O PIN NUNCA sai do ESP: o resumo publicado contém só metadados.
//
// - Capacidade fixa em RAM estática, sem alocação dinâmica.
// - Revogar = BLOQ/DEL/LIMPAR. Sem blocklist que cresce.
// - NVS espelha a RAM ("n", "t%02d", "n%02d", "e%02d", "v%02d", "a%02d").
#ifndef SENHAS_STORE_H
#define SENHAS_STORE_H

#include <Arduino.h>

#define MAX_SENHAS 20
#define TOKEN_HEX_LEN 4 // tamanho do slot (= PIN_LEN dígitos)
#define PIN_LEN 4
#define NOME_MAX_LEN 24

// Carrega a tabela da NVS para a RAM. Chame no setup().
void senhas_init();

// Adiciona PIN+nome (+epoch/validadeH opcionais). Nome deve ser único.
// Ignora inválido/duplicado (PIN ou nome)/lotado. Retorna true se mudou.
bool senhas_adicionar(const String &pin, const String &nome,
                      uint32_t epoch = 0, uint16_t validadeH = 0);

// Remove pela etiqueta (nome). Retorna true se existia.
bool senhas_remover_nome(const String &nome);

// Troca o PIN da etiqueta, mantendo nome; atualiza a data.
// Retorna true se trocou.
bool senhas_renovar(const String &nome, const String &pinNovo, uint32_t epoch = 0);

// Bloqueia (ativa=0) / libera (ativa=1). Retorna true se mudou.
bool senhas_bloquear(const String &nome, bool bloquear);

// Apaga TODA a tabela (RAM + NVS). Retorna true se havia algo.
bool senhas_limpar();

// Consulta a tabela em RAM (sem I/O). contem() exige ativa==1.
int senhas_total();
int senhas_total_ativas();
bool senhas_contem(const String &pin);

// Monta o resumo SEM o PIN: {"total":N,"chaves":[
// {"nome":"..","criadaEm":E,"validadeH":H,"ativa":0/1},...]}.
void senhas_resumo_json(String &out);

// Flag consumível: true uma vez após cada mudança.
bool senhas_consumirMudanca();

#endif
