# 📍 Pins

Centraliza **todas** as definições de pinos do projeto. Fonte única de verdade.

## Atuadores

| Define | GPIO | Componente |
| :--- | :--- | :--- |
| `PINO_RELE` | 4 | Módulo relé (via 2N2222A) |
| Buzzer | — | Removido do hardware |

## Teclado Matricial (conector H2 — hardware real)

| Define | GPIO | Função |
| :--- | :--- | :--- |
| `KEYPAD_ROW_1` | 27 | Linha R0 (teclas 1, 2, 3) |
| `KEYPAD_ROW_2` | 13 | Linha R1 (teclas 4, 5, 6) |
| `KEYPAD_ROW_3` | 32 | Linha R2 (teclas 7, 8, 9) |
| `KEYPAD_ROW_4` | 25 | Linha R3 (teclas *, 0, #) |
| `KEYPAD_COL_1` | 26 | Coluna C0 |
| `KEYPAD_COL_2` | 14 | Coluna C1 |
| `KEYPAD_COL_3` | 33 | Coluna C2 |
| `KEYPAD_COL_4` | 32 | Coluna 4 (só Wokwi) |

## LCD I2C

| Define | Valor |
| :--- | :--- |
| `LCD_I2C_ADDRESS` | `0x27` |
| `LCD_COLUMNS` | `20` |
| `LCD_ROWS` | `4` |

## Simulação Wokwi (condicional)

```cpp
#ifdef WOKWI_SIM
  #define PINO_BOTAO_TESTE  5
  #define PINO_LED_STATUS   2
#endif
```

## Regras

- ✅ Sempre adicione novos pinos aqui — nunca hardcode no `.cpp`.
- ✅ Use `#ifdef WOKWI_SIM` para pinos exclusivos da simulação.
- ❌ Nunca duplique defines em outros arquivos.

## Como Incluir

```cpp
#include "pins.h"
```

# 🔑 Secrets (Template)

Arquivo de credenciais. **Nunca comite o `secrets.h` real.**

## Uso

```bash
cp include/secrets.example.h include/secrets.h
# editar secrets.h com suas credenciais
```

O `secrets.h` está no `.gitignore`.

## Conteúdo

```cpp
#ifndef SECRETS_H
#define SECRETS_H

// --- Wi-Fi ---
#define WIFI_SSID "SUA_REDE"
#define WIFI_PASSWORD "SUA_SENHA"

// --- IP Fixo ---
#define USE_STATIC_IP true
#define STATIC_IP 192, 168, 1, 150
#define STATIC_GATEWAY 192, 168, 1, 1
#define STATIC_SUBNET 255, 255, 255, 0
#define STATIC_DNS 8, 8, 8, 8

// --- OTA ---
#define OTA_HOSTNAME "esp32-rele"
#define OTA_PASSWORD "senha_ota_segura"

// --- Firebase ---
#define API_KEY "SUA_API_KEY"
#define USER_EMAIL "device@projeto.local"
#define USER_PASSWORD "SENHA_FORTE"
#define DATABASE_URL "https://projeto-default-rtdb.firebaseio.com/"

#endif
```

## Para Wokwi

Troque as credenciais de Wi-Fi:

```cpp
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
```

## Segurança

- ⚠️ Adicione `secrets.h` ao `.gitignore`
- ⚠️ Nunca compartilhe o arquivo real
- ✅ Use `secrets.example.h` como template público