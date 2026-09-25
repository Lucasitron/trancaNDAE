#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"
#include "pins.h"

extern volatile bool wifiConectado;

void conectarWiFi();
void eventoWiFi(WiFiEvent_t event);

// Novas funções
void configurarIPFixo();
void iniciarOTA();
void processarOTA(); // Chame no loop() para processar atualizações

#endif