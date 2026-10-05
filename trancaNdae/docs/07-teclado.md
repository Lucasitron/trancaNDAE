# Módulo Teclado (`teclado.h/.cpp`)

Varredura manual do teclado matricial **3x4** (sem biblioteca externa).

## Elétrica

- Linhas (`KEYPAD_ROW_*`): saída, HIGH em repouso, LOW para varrer.
- Colunas (`KEYPAD_COL_*`): `INPUT_PULLUP`; tecla pressionada = LOW.

## API

### `void teclado_init()`
Configura os pinos (chame no `setup()`).

### `char teclado_ler(byte *outR = nullptr, byte *outC = nullptr)`
Varre linha a linha e retorna `'0'`–`'9'`, `'*'`, `'#'`, ou `0` (nenhuma).
Debounce 20ms + espera soltar com **timeout de 500ms** (linha presa nunca
trava o `loop` — Wi-Fi/Firebase/OTA continuam). `outR`/`outC` = índices
físicos (diagnóstico; posição não revela o PIN).

## Ordem do keymap

`ROW_1/COL_1` = tecla `1` (canto superior esquerdo). Inverter a fiação
espelha o mapa — confira com o log `Tecla @R?C?` ao pressionar 1–9.
