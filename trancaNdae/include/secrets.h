// secrets.h
#ifndef SECRETS_H
#define SECRETS_H

// Credenciais Wi-Fi
#define WIFI_SSID "lucasitron"
#define WIFI_PASSWORD "lucaireless"

#define USER_UID "casaDoEstunteUFPA_esp32"
#define API_KEY "AIzaSyByBylcrbdPD6el0C2UbBeAdFvtFzWhFr4"
#define USER_EMAIL "esp32@casadoestudante.ufpa"
#define USER_PASSWORD "rs32596tuf@15"
#define DATABASE_URL "https://trancandae-cd59b-default-rtdb.firebaseio.com"

// --- Rede Fixa (IP estático) ---
// Escolha IPs fora do range do DHCP do seu roteador para evitar conflitos
#define USE_STATIC_IP true
#define STATIC_IP 10, 0, 0, 150
#define STATIC_GATEWAY 10, 0, 0, 1
#define STATIC_SUBNET 255, 255, 255, 0
#define STATIC_DNS 8, 8, 8, 8

// --- OTA ---
#define OTA_HOSTNAME "esp32-rele"
#define OTA_PASSWORD "senha_ota_segura"

// --- Telnet (log remoto via rede) ---
// Conecte: telnet 10.0.0.150 23
#define TELNET_PORT 23

#endif