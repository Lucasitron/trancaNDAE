<div align="center">

# 🔐 Tranca Inteligente ESP32

**Sistema de controle de acesso com autenticação remota via Firebase e validação local por HMAC-SHA256**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?logo=platformio)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/Board-ESP32--DevKit-red)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Wokwi](https://img.shields.io/badge/Simulate-Wokwi-purple)](https://wokwi.com/)

</div>

---

## 📖 Sobre o Projeto

Este projeto implementa uma **tranca eletrônica segura** baseada em ESP32, onde o comando de abertura é enviado remotamente via **Firebase Realtime Database** e validado localmente por um **PIN digitado no teclado matricial**.

A segurança é garantida por **HMAC-SHA256**: o PC (remetente) cifra a palavra-comando com o PIN, e o ESP32 só aceita o comando se o PIN digitado gerar o mesmo hash. Isso elimina a necessidade de transmitir a chave pela rede.

### ✨ Funcionalidades

- 🔐 **Autenticação HMAC-SHA256** com anti-replay (token consumido após uso)
- 📡 **Wi-Fi com IP fixo** + reconexão automática via eventos
- 🔥 **Firebase Realtime Database** em modo assíncrono (streaming)
- 💾 **Persistência em NVS** — token sobrevive a reboot
- 🔄 **OTA** — atualização de firmware sem cabo
- ⌨️ **Teclado matricial 3x4** (4x4 no Wokwi)
- 🖥️ **LCD 20x4 I2C** com layout em 4 zonas
- 🧪 **Modo simulação Wokwi** isolado em módulo próprio

---

## 🏗️ Arquitetura

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│   PC     │───▶│ Firebase │───▶│  ESP32   │───▶│  Relé    │
│ (Python) │    │   RTDB   │    │ (stream) │    │ (carga)  │
└──────────┘    └──────────┘    └────┬─────┘    └──────────┘
                                     │
                                     ▼
                              ┌──────────────┐
                              │ Teclado 3x4  │
                              │ + LCD 20x4   │
                              └──────────────┘
```

**Fluxo:**
1. O PC cifra `"ABRIR"` com o PIN → gera token HMAC
2. Publica o token no Firebase
3. ESP32 recebe via streaming e salva na NVS
4. Usuário digita o PIN no teclado
5. ESP32 recalcula o HMAC e compara com o token salvo
6. Se válido: comuta o relé e **remove o token** (anti-replay)

---

## 📁 Estrutura do Projeto

```
tranca-esp32/
├── include/
│   ├── pins.h                 # Definições de pinos
│   ├── secrets.h              # Credenciais (gitignored)
│   ├── wifi_manager.h
│   ├── firebase_client.h
│   └── simulation.h
├── src/
│   ├── main.ino               # Ponto de entrada
│   ├── wifi_manager.cpp       # Wi-Fi, IP fixo, OTA
│   ├── firebase_client.cpp    # Firebase + NVS
│   └── simulation.cpp         # Modo Wokwi (no-op em produção)
├── diagram.json               # Circuito Wokwi (branch simulação)
├── wokwi.toml                 # Config Wokwi (branch simulação)
├── platformio.ini
└── README.md
```

---

## 🔧 Hardware

| Componente | Pino ESP32 | Observação |
| :--- | :--- | :--- |
| **Relé** | GPIO 15 | Via transistor 2N2222A + diodo 1N4007 |
| **Buzzer** | GPIO 4 | Passivo ou ativo |
| **LCD 20x4 I2C** | GPIO 21 (SDA), 22 (SCL) | Endereço `0x27` |
| **Teclado R1-R4** | GPIO 13, 12, 14, 27 | Linhas |
| **Teclado C1-C3** | GPIO 26, 25, 33 | Colunas (3x4 real) |
| **Teclado C4** | GPIO 32 | Só no Wokwi (4x4) |
| **Alimentação** | LM7805 → 5V | VIN do ESP32 |

---

## 🚀 Como Usar

### 1. Clonar o repositório

```bash
git clone https://github.com/seu-usuario/tranca-esp32.git
cd tranca-esp32
```

### 2. Configurar credenciais

Crie `include/secrets.h` (baseado em `secrets.example.h`):

```cpp
#define WIFI_SSID "SUA_REDE"
#define WIFI_PASSWORD "SUA_SENHA"

#define USE_STATIC_IP true
#define STATIC_IP 192, 168, 1, 150
#define STATIC_GATEWAY 192, 168, 1, 1
#define STATIC_SUBNET 255, 255, 255, 0
#define STATIC_DNS 8, 8, 8, 8

#define OTA_HOSTNAME "esp32-rele"
#define OTA_PASSWORD "senha_ota"

#define API_KEY "..."
#define USER_EMAIL "..."
#define USER_PASSWORD "..."
#define DATABASE_URL "https://seu-projeto-default-rtdb.firebaseio.com/"
```

### 3. Compilar e gravar

```bash
pio run -t upload
pio device monitor
```

### 4. Simular no Wokwi

```bash
git checkout simulation
pio run
# Abrir diagram.json no VS Code e pressionar Ctrl+Shift+P → "Wokwi: Start Simulator"
```

---

## 🧪 Modo Simulação (Wokwi)

O projeto inclui uma **branch `simulation`** dedicada ao Wokwi. Ela contém:

- `diagram.json` — circuito completo
- `wokwi.toml` — configuração do simulador
- `simulation.cpp/h` — botão TEST e LED de status
- `main.ino` com teclado 4x4 (teclas A/B/C/D mapeadas)
- Flag `-D WOKWI_SIM` no `platformio.ini`

**Chave de teste:** `12345678` (usada pelo botão TEST para gerar o HMAC).

---

## 🔒 Segurança

| Camada | Implementação |
| :--- | :--- |
| **Transporte** | HTTPS/TLS (Firebase) |
| **Autenticação** | Firebase Auth (e-mail/senha) |
| **Validação local** | HMAC-SHA256 (mbedtls) |
| **Anti-replay** | Token consumido após uso |
| **Persistência** | NVS (sobrevive a reboot) |
| **OTA** | Senha obrigatória |

> ⚠️ **Produção:** habilite **Flash Encryption** + **NVS Encryption** no ESP-IDF para proteger a chave de acesso contra leitura física.

---

## 📜 Licença

MIT © 2026 — Lucas Gonçalves

---

<div align="center">

**Feito com ❤️ usando ESP32 + Firebase + Wokwi**

</div>