# Módulo Wi-Fi Manager

Conexão Wi-Fi **multi-rede** (padrão + secundário), IP fixo por sub-rede,
eventos e OTA.

## Estratégia de conexão (`conectarWiFi()`)

1. **Padrão** (`secrets.h`: `WIFI_SSID`/`WIFI_PASSWORD`) — sempre primeiro.
2. **Secundário** (NVS `wifi`, via página: `WIFI2:ssid:senha`) — só se ativo
   e se o padrão falhou. `WIFI2OFF` desativa (volta a só-padrão).
3. Cada tentativa: **DHCP primeiro** (descobre gateway/DNS corretos de
   qualquer rede) e, se a sub-rede do lease casar com um perfil
   (`STATIC_IP` 10.0.0.x ou `STATIC_IP2` 192.168.1.x), **reaplica aquele IP
   fixo** (mesmo gateway/DNS/sub-rede do DHCP). Fora dos perfis, mantém DHCP.
4. `wifiPerfil` = `"padrao"` | `"secundario"` | `""` (offline).

Sem nenhuma rede, `wifi_reconectar()` (retry a cada 60s no `loop`, fora de
porta aberta/status) tenta de novo e rearma o OTA.

## API

| Função | Descrição |
| :--- | :--- |
| `conectarWiFi()` | Padrão → secundário (bloqueante, com timeouts) |
| `eventoWiFi(event)` | `GOT_IP` → `wifiConectado=true`; `DISCONNECTED` → `false` + `firebase_reset_stream()` (o stream SSE morreu) |
| `iniciarOTA()` / `processarOTA()` | ArduinoOTA (hostname/senha); `onStart` desliga o relé |
| `wifi2_ativo()` | `true` se há secundário salvo e ativado |
| `wifi2_salvar(ssid, pass)` | Valida (SSID 1–32, senha 0–63, sem `:`) e ativa |
| `wifi2_desativar()` | Desativa (mantém credenciais) |
| `wifi_reconectar()` | `conectarWiFi()` + rearma OTA |

## Variáveis globais

| Variável | Tipo | Descrição |
| :--- | :--- | :--- |
| `wifiConectado` | `volatile bool` | Estado atual |
| `wifiPerfil` | `String` | Perfil ativo |

## Dependências

`WiFi.h`, `ArduinoOTA.h`, `Preferences.h` (NVS `wifi`), `firebase_client.h`
(reset do stream), `secrets.h`, `telnet_log.h`, `pins.h` (relé no OTA).
