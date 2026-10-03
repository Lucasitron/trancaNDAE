// pins.h
#ifndef PINS_H
#define PINS_H

// --- Atuadores ---
// Relé no GPIO 4 (seguro, sem strapping). Sem buzzer no hardware.
#define PINO_RELE 4


// --- Teclado matricial 3x4 (hardware real, conector H2) ---
// ORDEM IMPORTA: ROW_1/COL_1 = fileira/coluna física do '1' (canto superior
// esquerdo). R0..R3 = linhas, C0..C2 = colunas do esquema.
#define KEYPAD_ROW_1 27 // R0
#define KEYPAD_ROW_2 13 // R1 (era 35: só-entrada, sem pull-up -> erro gpio_pullup_en a cada scan)
#define KEYPAD_ROW_3 32 // R2
#define KEYPAD_ROW_4 25 // R3
#define KEYPAD_COL_1 26 // C0
#define KEYPAD_COL_2 14 // C1
#define KEYPAD_COL_3 33 // C2
//#define KEYPAD_COL_4 32 // <-- Só usada no Wokwi (4ª coluna)

// --- LCD I2C (20x4) ---
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLUMNS 20
#define LCD_ROWS 4

// --- Pinos de simulação Wokwi ---
#ifdef WOKWI_SIM
#define PINO_BOTAO_TESTE 5 // livre (fora do Wokwi não é compilado)
#define PINO_LED_STATUS 2
#endif

#endif