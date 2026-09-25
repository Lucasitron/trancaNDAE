# 📡 Módulo Wi-Fi Manager

Gerencia conexão Wi-Fi com **IP fixo**, **eventos** e **OTA**.

## API

### `void conectarWiFi()`
Inicia a conexão Wi-Fi em modo estação. Aplica IP fixo se `USE_STATIC_IP` estiver definido em `secrets.h`. Bloqueia até conectar ou timeout de 10s.

```cpp
WiFi.onEvent(eventoWiFi);
conectarWiFi();
```

### `void eventoWiFi(WiFiEvent_t event)`
Callback registrado via `WiFi.onEvent()`. Atualiza a flag global `wifiConectado`.

| Evento | Ação |
| :--- | :--- |
| `ARDUINO_EVENT_WIFI_STA_CONNECTED` | Log: conectado ao roteador |
| `ARDUINO_EVENT_WIFI_STA_GOT_IP` | `wifiConectado = true` |
| `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` | `wifiConectado = false` |

### `void iniciarOTA()`
Inicializa o serviço ArduinoOTA com hostname e senha. Callbacks registrados:
- `onStart` → desliga o relé
- `onProgress` → log a cada 10%
- `onError` → mensagens traduzidas

### `void processarOTA()`
Chamada **não-bloqueante**. Deve ser executada no `loop()` principal.

```cpp
void loop() {
    processarOTA();
    // ...
}
```

## Variáveis Globais

| Variável | Tipo | Descrição |
| :--- | :--- | :--- |
| `wifiConectado` | `volatile bool` | Estado atual da conexão |

## Dependências

- `WiFi.h` (core ESP32)
- `ArduinoOTA.h` (core ESP32)
- `secrets.h` (credenciais + hostname OTA)

## Exemplo de Uso

```cpp
#include "wifi_manager.h"

void setup() {
    WiFi.onEvent(eventoWiFi);
    conectarWiFi();
    if (wifiConectado) iniciarOTA();
}

void loop() {
    processarOTA();
}
```