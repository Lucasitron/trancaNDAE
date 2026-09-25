// secrets.h
#ifndef SECRETS_H
#define SECRETS_H

// Credenciais Wi-Fi
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

#define USER_UID "casaDoEstunteUFPA_esp32"
#define API_KEY "PywQ3HVcqCvRjdt8sDtXW9VL6V7gVQ1H78XhVgO7"
#define USER_EMAIL "esp32@casadoestudante.ufpa"
#define USER_PASSWORD "rs32596tuf@15"
#define DATABASE_URL "https://trancandae-cd59b-default-rtdb.firebaseio.com"

// --- Rede Fixa (IP estático) ---
// Escolha IPs fora do range do DHCP do seu roteador para evitar conflitos
#define USE_STATIC_IP true
#define STATIC_IP 192, 168, 1, 150
#define STATIC_GATEWAY 192, 168, 1, 1
#define STATIC_SUBNET 255, 255, 255, 0
#define STATIC_DNS 8, 8, 8, 8

// --- OTA ---
#define OTA_HOSTNAME "esp32-rele"
#define OTA_PASSWORD "senha_ota_segura"

#endif