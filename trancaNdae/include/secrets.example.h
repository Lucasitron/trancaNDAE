// secrets.example.h — TEMPLATE público (sem credenciais reais).
// Uso: cp include/secrets.example.h include/secrets.h  e preencha.
// O secrets.h real está no .gitignore e NUNCA deve ser commitado.
#ifndef SECRETS_H
#define SECRETS_H

// Credenciais Wi-Fi
#define WIFI_SSID "SUA_REDE"
#define WIFI_PASSWORD "SUA_SENHA"

#define USER_UID "seu-dispositivo_esp32"
#define API_KEY "AIzaSUA_WEB_API_KEY_DO_FIREBASE"
#define USER_EMAIL "device@seu-projeto.local"
#define USER_PASSWORD "SENHA_FORTE"
#define DATABASE_URL "https://seu-projeto-default-rtdb.firebaseio.com"

// --- Rede: fixo multi-perfil + DHCP de reserva ---
// O ESP tenta o perfil A (10.0.0.x), depois o B (192.168.1.x); sem
// conexão em nenhum, usa DHCP. DNS = gateway. Ajuste IPs/gateways
// para não conflitar com o DHCP do seu roteador.
#define USE_STATIC_IP true
#define STATIC_IP 10, 0, 0, 150
#define STATIC_GATEWAY 10, 0, 0, 1
#define STATIC_IP2 192, 168, 1, 150
#define STATIC_GATEWAY2 192, 168, 1, 1
#define STATIC_SUBNET 255, 255, 255, 0
#define IP_FALLBACK_DHCP

// --- OTA ---
#define OTA_HOSTNAME "esp32-rele"
#define OTA_PASSWORD "troque_esta_senha"

// --- Admin (modo status no display: senha + '*') ---
// TROQUE por uma senha só sua. Nunca abre a porta nem entra na tabela.
#define ADMIN_PASSWORD "9999"

// --- Telnet (log remoto via rede) ---
// Conecte: telnet <IP> 23
#define TELNET_PORT 23

#endif
