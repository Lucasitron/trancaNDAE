// pins.h
#ifndef PINS_H
#define PINS_H

// --- Atuadores ---
#define PINO_RELE 33
#define PINO_BUZZER 25 // era GPIO 4, que conflita com KEYPAD_COL_2

// --- Teclado matricial 3x4 (real) ---
#define KEYPAD_ROW_1 19
#define KEYPAD_ROW_2 18
#define KEYPAD_ROW_3 5
#define KEYPAD_ROW_4 17
#define KEYPAD_COL_1 16
#define KEYPAD_COL_2 4
#define KEYPAD_COL_3 15
//#define KEYPAD_COL_4 32 // <-- Só usada no Wokwi (4ª coluna)

// --- LCD I2C (20x4) ---
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLUMNS 20
#define LCD_ROWS 4

// --- Pinos de simulação Wokwi ---
#ifdef WOKWI_SIM
#define PINO_BOTAO_TESTE 5
#define PINO_LED_STATUS 2
#endif

#endif