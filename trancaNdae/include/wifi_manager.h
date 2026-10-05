#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"
#include "pins.h"

extern volatile bool wifiConectado;
extern String wifiPerfil; // "padrao" | "secundario" | "" (sem conexão)

void conectarWiFi();
void eventoWiFi(WiFiEvent_t event);

// Novas funções
void iniciarOTA();
void processarOTA(); // Chame no loop() para processar atualizações

// Wi-Fi secundário (NVS, configurado pela página: WIFI2:ssid:senha).
// O padrão (secrets.h) é usado sempre que o secundário está inativo
// ou falha. Retorna false se inválido (SSID 1-32, senha 0-63, sem ':').
bool wifi2_ativo();
bool wifi2_salvar(const String &ssid, const String &pass);
void wifi2_desativar();

// Reconecta (padrão -> secundário) e rearma OTA. Para retry offline.
void wifi_reconectar();

#endif