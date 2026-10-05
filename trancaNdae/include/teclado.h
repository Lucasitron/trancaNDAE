// teclado.h — Varredura manual do teclado matricial 3x4.
// Linhas = saída (HIGH em repouso, LOW para varrer).
// Colunas = entrada com pull-up (tecla pressionada = LOW).
#ifndef TECLADO_H
#define TECLADO_H

#include <Arduino.h>

// Configura pinos (linhas saída HIGH, colunas INPUT_PULLUP). Chame no setup().
void teclado_init();

// Varre a matriz e retorna a tecla ('0'-'9','*','#') ou 0 = nenhuma.
// Debounce 20ms + espera soltar com timeout 500ms (nunca trava o loop).
// outR/outC = índices físicos (diagnóstico; posição não revela o PIN).
char teclado_ler(byte *outR = nullptr, byte *outC = nullptr);

#endif
