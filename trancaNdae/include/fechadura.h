// fechadura.h — Relé, validação e reinício.
// Relé energizado = porta ABERTA. GPIO 4 é seguro (sem strapping).
// Se o módulo for ativo em LOW, inverta RELE_ABERTO/RELE_FECHADO.
#ifndef FECHADURA_H
#define FECHADURA_H

#include <Arduino.h>

#define RELE_FECHADO LOW
#define RELE_ABERTO HIGH

// Fecha sozinha após este tempo (verificação não-bloqueante via millis).
#define TEMPO_PORTA_ABERTA_MS 5000

// Digite + '*' = REINICIA o ESP (só reboot; as chaves NÃO são apagadas).
// NUNCA cadastre "0000" como chave de abertura.
#define CODIGO_RESET "0000"

// Relé começa FECHADO. Chame no setup().
void fechadura_init();

// Abre (destrava) e arma o auto-fechamento.
void abrirPorta();

// Fecha (trava) imediatamente.
void fecharPorta();

// true com a porta aberta (teclado normal fica pausado).
bool fechadura_aberta();

// Verifica o timeout e fecha sozinha. Chame todo loop().
void fechadura_loop();

// Valida o PIN contra a tabela local. SOMENTE com '#'. Admin nunca abre.
void validarChave(const String &chave);

// Reinicia o ESP (chaves preservadas).
void executarReinicio();

#endif
