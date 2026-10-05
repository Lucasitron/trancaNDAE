// teclado.cpp — implementação da varredura manual 3x4.
#include "teclado.h"
#include "pins.h"

#define TEC_ROWS 4
#define TEC_COLS 3

static byte rowPins[TEC_ROWS] = {KEYPAD_ROW_1, KEYPAD_ROW_2, KEYPAD_ROW_3, KEYPAD_ROW_4};
static byte colPins[TEC_COLS] = {KEYPAD_COL_1, KEYPAD_COL_2, KEYPAD_COL_3};
static char keysMap[TEC_ROWS][TEC_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

void teclado_init()
{
    for (byte r = 0; r < TEC_ROWS; r++)
    {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH);
    }
    for (byte c = 0; c < TEC_COLS; c++)
        pinMode(colPins[c], INPUT_PULLUP);
}

char teclado_ler(byte *outR, byte *outC)
{
    for (byte r = 0; r < TEC_ROWS; r++)
    {
        digitalWrite(rowPins[r], LOW);
        delayMicroseconds(50);
        for (byte c = 0; c < TEC_COLS; c++)
        {
            if (digitalRead(colPins[c]) == LOW)
            {
                delay(20); // debounce
                if (digitalRead(colPins[c]) == LOW)
                {
                    // Espera soltar, máx. 500ms: linha presa não trava o loop
                    // (Wi-Fi/Firebase/OTA continuam rodando).
                    unsigned long t0 = millis();
                    while (digitalRead(colPins[c]) == LOW && (millis() - t0 < 500))
                        delay(5);
                    digitalWrite(rowPins[r], HIGH);
                    if (outR)
                        *outR = r;
                    if (outC)
                        *outC = c;
                    return keysMap[r][c];
                }
            }
        }
        digitalWrite(rowPins[r], HIGH);
    }
    return 0;
}
