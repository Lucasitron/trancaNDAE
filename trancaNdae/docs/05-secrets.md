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

// --- IP (DHCP recomendado; fixo só com gateway/DNS da rede atual) ---
#define USE_STATIC_IP false

// --- OTA ---
#define OTA_HOSTNAME "esp32-rele"
#define OTA_PASSWORD "troque_esta_senha"

// --- Firebase ---
#define API_KEY "AIzaSUA_WEB_API_KEY"
#define USER_EMAIL "device@projeto.local"
#define USER_PASSWORD "SENHA_FORTE"
#define DATABASE_URL "https://projeto-default-rtdb.firebaseio.com/"

// --- Admin (modo status: senha + '*'; nunca abre a porta) ---
#define ADMIN_PASSWORD "9999"

// --- Telnet ---
#define TELNET_PORT 23

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