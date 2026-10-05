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

Este projeto implementa uma **tranca eletrônica** baseada em ESP32. As senhas
ficam **somente no ESP** (RAM+NVS): a página web envia comandos voláteis via
**Firebase Realtime Database** (um PIN ou `LIMPAR`), o ESP consome, cadastra
localmente e **apaga o nó** — nada de senha permanece na nuvem. A abertura é
**somente no teclado**, digitando o PIN + `#`.

### ✨ Funcionalidades

- 🔐 **Senhas só no ESP** (tabela local 20 PINs, sobrevive a reboot)
- ⌨️ **Abertura somente com `#`** + auto-fechamento em 5s (não-bloqueante)
- 🖥️ **Modo status no display** (senha de admin + `*`, navegação 2/8/4/6)
- 📡 **Wi-Fi com DHCP** + Telnet de logs (porta 23) + **OTA** sem cabo
- 🔥 **Firebase RTDB assíncrono** (stream consome-e-apaga comandos)
- 🖥️ **LCD 20x4 I2C** com auto-detecção e anti-spam de escritas
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
1. A página envia PIN (ou `LIMPAR`) em `comandos/{dispositivo}`
2. ESP32 recebe via streaming, cadastra/zera a tabela local e **apaga o nó**
3. Usuário digita o PIN no teclado + `#`
4. Se válido: relé abre por 5s e fecha sozinho

---

## 📁 Estrutura do Projeto

```
tranca-esp32/
├── include/
│   ├── pins.h                 # Definições de pinos
│   ├── secrets.h              # Credenciais (gitignored; ver secrets.example.h)
│   ├── wifi_manager.h
│   ├── firebase_client.h
│   ├── senhas_store.h         # Tabela local de PINs (RAM+NVS)
│   ├── telnet_log.h
│   └── simulation.h
├── src/
│   ├── main.cpp               # Fechadura (teclado manual + modo status)
│   ├── wifi_manager.cpp       # Wi-Fi DHCP, OTA
│   ├── firebase_client.cpp    # Stream consome-e-apaga
│   ├── senhas_store.cpp       # Tabela local (add/limpar)
│   ├── telnet_log.cpp         # Log remoto porta 23
│   └── simulation.cpp         # Modo Wokwi (no-op em produção)
├── web/                       # Página: login + envio volátil (sem senhas no DB)
├── platformio.ini
└── README.md
```

---

## 🔧 Hardware

| Componente | Pino ESP32 | Observação |
| :--- | :--- | :--- |
| **Relé** | GPIO 4 | Via transistor 2N2222A + diodo 1N4007 |
| **Buzzer** | — | Removido |
| **LCD 20x4 I2C** | GPIO 21 (SDA), 22 (SCL) | Endereço `0x27` |
| **Teclado R0-R3** | GPIO 27, 13, 32, 25 | Linhas |
| **Teclado C0-C2** | GPIO 26, 14, 33 | Colunas (3x4 real) |
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

```bash
cp trancaNdae/include/secrets.example.h trancaNdae/include/secrets.h
# edite com Wi-Fi, Firebase, ADMIN_PASSWORD, OTA e Telnet
```

> `secrets.h` é gitignored — nunca commitado. Ver `docs/06-operacao.md`.

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

**Chave de teste:** `1234` (usada pelo botão TEST para gerar o HMAC).

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