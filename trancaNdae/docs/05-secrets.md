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