// main.cpp — Fechadura eletrônica (oficial, sem cripto).
//
// Fluxo: página web grava código plain de 4 dígitos no Firebase
//   - /senhas/dispositivo1/lista  = "1234,5678" (CSV, várias chaves)
//   - /comandos/dispositivo1      = "1234"      (uso único, legado)
// O ESP espelha no NVS/RAM e compara direto com o digitado.
// Relé no GPIO 4. Sem buzzer.
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= Módulos do Projeto =================
#include "pins.h"
#include "secrets.h"
#include "wifi_manager.h"
#include "firebase_client.h"
#include "senhas_store.h"
#include "telnet_log.h"

// ================= Teclado 3x4 (varredura manual) =================
// Linhas = saída (HIGH em repouso, LOW para varrer).
// Colunas = entrada com pull-up (tecla = LOW).
const byte ROWS = 4;
const byte COLS = 3;
byte rowPins[ROWS] = {KEYPAD_ROW_1, KEYPAD_ROW_2, KEYPAD_ROW_3, KEYPAD_ROW_4};
byte colPins[COLS] = {KEYPAD_COL_1, KEYPAD_COL_2, KEYPAD_COL_3};
char keysMap[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

// Varre a matriz e retorna a tecla pressionada (0 = nenhuma).
// Debounce de 20ms + espera soltar. outR/outC = índices físicos (diagnóstico).
char lerTeclado(byte *outR = nullptr, byte *outC = nullptr)
{
    for (byte r = 0; r < ROWS; r++)
    {
        digitalWrite(rowPins[r], LOW);
        delayMicroseconds(50);
        for (byte c = 0; c < COLS; c++)
        {
            if (digitalRead(colPins[c]) == LOW)
            {
                delay(20);
                if (digitalRead(colPins[c]) == LOW)
                {
                    while (digitalRead(colPins[c]) == LOW)
                        delay(10); // espera soltar
                    digitalWrite(rowPins[r], HIGH);
                    if (outR)
                        *outR = r;
                    if (outC)
                        *outC = c;
                    return keysMap[r][c];
                }
            }
        }
        digitalWrite(rowPins[r], HIGH);
    }
    return 0;
}

// ================= LCD (I2C, com auto-detecção) =================
// USA_LCD 0 = roda sem I2C (só Serial/Telnet). Com 1, se o endereço não
// responder no boot o LCD desliga sozinho (lcdOK=false) sem travar o teste.
#define USA_LCD 1
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
bool lcdOK = false;

bool escanearI2C()
{
    tlogf("I2C: procurando LCD em 0x%02X...\n", LCD_I2C_ADDRESS);
    bool achou = false;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            tlogf("I2C: dispositivo em 0x%02X%s\n",
                  addr, (addr == LCD_I2C_ADDRESS ? " <- LCD" : ""));
            if (addr == LCD_I2C_ADDRESS)
                achou = true;
        }
    }
    if (!achou)
        tlogln("I2C: LCD NAO responde. Seguindo sem display.");
    return achou;
}

// ================= Estado da Fechadura =================
// Relé energizado = porta ABERTA. GPIO 4 é seguro (sem strapping).
// Se o seu módulo for ativo em LOW, inverta os dois defines abaixo.
#define RELE_FECHADO LOW
#define RELE_ABERTO HIGH
const unsigned long TEMPO_PORTA_ABERTA_MS = 5000; // fecha sozinha após 5s

bool portaAberta = false;
unsigned long momentoAbertura = 0;

// ================= Estado da Validação =================
String pinDigitado = "";
const int PIN_MAX_LENGTH = PIN_LEN; // 4 dígitos: valida sozinho

// ================= Display (com anti-spam I2C) =================
String linhaStatus = "Sistema Iniciado";
String linhaMensagem = "Aguardando...";
String linhaTeclado = "";
String linhaRodape = "Aguardando comando";

void limparLinha(uint8_t linha)
{
    lcd.setCursor(0, linha);
    for (uint8_t i = 0; i < LCD_COLUMNS; i++)
        lcd.print(" ");
}

String centralizar(const String &texto)
{
    if (texto.length() >= LCD_COLUMNS)
        return texto.substring(0, LCD_COLUMNS);
    int espacos = (LCD_COLUMNS - texto.length()) / 2;
    String resultado = "";
    for (int i = 0; i < espacos; i++)
        resultado += " ";
    resultado += texto;
    return resultado;
}

void atualizarDisplay()
{
    static String ultStatus = "";
    static String ultMsg = "";
    static String ultTeclado = "";
    static String ultRodape = "";
    if (linhaStatus == ultStatus && linhaMensagem == ultMsg &&
        linhaTeclado == ultTeclado && linhaRodape == ultRodape)
        return; // sem mudança: poupa o I2C
    ultStatus = linhaStatus;
    ultMsg = linhaMensagem;
    ultTeclado = linhaTeclado;
    ultRodape = linhaRodape;

    if (!lcdOK)
    {
        tlogf("[LCD] %s | %s | %s | %s\n",
              linhaStatus.c_str(), linhaMensagem.c_str(),
              linhaTeclado.c_str(), linhaRodape.c_str());
        return;
    }

    lcd.setCursor(0, 0);
    lcd.print(centralizar(linhaStatus));
    limparLinha(1);
    lcd.setCursor(0, 1);
    lcd.print(linhaMensagem);
    limparLinha(2);
    lcd.setCursor(0, 2);
    lcd.print(linhaTeclado);
    limparLinha(3);
    lcd.setCursor(0, 3);
    lcd.print(linhaRodape);
}

void mostrarNoLCD(const String &status, const String &mensagem,
                  const String &teclado = "", const String &rodape = "")
{
    linhaStatus = status;
    linhaMensagem = mensagem;
    linhaTeclado = teclado;
    linhaRodape = rodape;
    atualizarDisplay();
}

void telaAguardando()
{
    if (senhas_total() > 0)
    {
        char rodape[21];
        snprintf(rodape, sizeof(rodape), "%d chave(s) ativa(s)", senhas_total());
        mostrarNoLCD("Aguardando...", "Digite a chave + #", "", rodape);
    }
    else
    {
        mostrarNoLCD("Aguardando...", "Comando remoto", "", "Digite a chave + #");
    }
}

// ================= Fechadura =================
void abrirPorta(const String &motivo)
{
    digitalWrite(PINO_RELE, RELE_ABERTO);
    portaAberta = true;
    momentoAbertura = millis();
    pinDigitado = "";
    mostrarNoLCD("PORTA ABERTA", "Empurre a porta", "", "Fechando sozinha...");
    tlogln("🔓 Porta ABERTA (" + motivo + ").");
}

void fecharPorta()
{
    digitalWrite(PINO_RELE, RELE_FECHADO);
    portaAberta = false;
    tlogln("🔒 Porta FECHADA (timeout).");
    telaAguardando();
}

// ================= Validação (comparação direta, sem cripto) =================
// 1) Tabela de chaves (web -> /senhas/dispositivo1/lista, CSV "1234,5678").
// 2) Fallback: comando único (/comandos/dispositivo1 = "1234", uso único).
void validarChave(const String &chave)
{
    String codigo = chave;
    codigo.trim();

    if (codigo.length() != PIN_LEN)
    {
        mostrarNoLCD("!! INVALIDO !!", "Codigo incompleto", "", "4 digitos + #");
        delay(1500);
        telaAguardando();
        return;
    }

    if (senhas_contem(codigo))
    {
        abrirPorta("chave cadastrada");
        return;
    }

    String token = ler_token_nvs();

    if (token.length() == 0)
    {
        mostrarNoLCD("Sem comando", "Nenhum comando pendente", "", "Aguarde o PC enviar");
        delay(1500);
        telaAguardando();
        return;
    }

    if (codigo == token)
    {
        limpar_token_nvs(); // Anti-replay (uso único)
        abrirPorta("comando único");
    }
    else
    {
        mostrarNoLCD("!! INVALIDO !!", "Chave incorreta", "", "Tente novamente");
        delay(1500);
        telaAguardando();
    }
}

// ================= SETUP =================
void setup()
{
    Serial.begin(115200);
    delay(500);
    tlogln("\n=== Iniciando Fechadura ===");

    // 0. Chaves em cache (NVS -> RAM). Sem Wi-Fi a fechadura segue operando.
    senhas_init();

    // 1. Relé (porta começa FECHADA). Sem buzzer no hardware.
    pinMode(PINO_RELE, OUTPUT);
    digitalWrite(PINO_RELE, RELE_FECHADO);

    // 2. Teclado: linhas saída HIGH, colunas entrada pull-up
    for (byte r = 0; r < ROWS; r++)
    {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH);
    }
    for (byte c = 0; c < COLS; c++)
        pinMode(colPins[c], INPUT_PULLUP);

    // 3. LCD (com auto-detecção)
#if USA_LCD
    Wire.begin(); // SDA=21, SCL=22 (padrão)
    if (escanearI2C())
    {
        lcd.init();
        lcd.backlight();
        lcdOK = true;
    }
#else
    tlogln("LCD desligado (USA_LCD=0). Teste via Serial/Telnet.");
#endif
    mostrarNoLCD("Sistema Iniciado", "Bem-vindo!", "", "Versao 3.0");
    delay(800);

    // 4. Wi-Fi
    WiFi.onEvent(eventoWiFi);
    conectarWiFi();

    // 5. OTA + Telnet + Firebase
    if (wifiConectado)
    {
        iniciarOTA();
        telnet_setup(); // log remoto: telnet <IP> 23
        iniciarFirebase();
        mostrarNoLCD("WiFi OK", "Sistema Online", "", "Aguardando comando");
    }
    else
    {
        mostrarNoLCD("WiFi OFF", "Modo offline", "", "Chaves em cache OK");
    }

    // 6. Descarta token em formato antigo (era hash; agora são 4 dígitos)
    String tokenPendente = ler_token_nvs();
    if (tokenPendente.length() > 0 && tokenPendente.length() != TOKEN_HEX_LEN)
    {
        limpar_token_nvs();
        tokenPendente = "";
    }
    if (tokenPendente.length() > 0)
    {
        tlogln("Token pendente na NVS: " + tokenPendente);
        mostrarNoLCD("Comando Pendente", "Digite a chave", "", "Pressione # para OK");
    }
    else
    {
        telaAguardando();
    }

    tlogln("=== Sistema pronto (fechadura) ===");
    tlog("Telnet: telnet ");
    tlog_raw(WiFi.localIP().toString());
    tlogf(" %d\n", TELNET_PORT);
}

// ================= LOOP =================
void loop()
{
    // 1. OTA + Firebase + Telnet
    processarOTA();
    processarFirebase();
    telnet_loop();

    // 2. Auto-fechamento (não-bloqueante)
    if (portaAberta && (millis() - momentoAbertura >= TEMPO_PORTA_ABERTA_MS))
        fecharPorta();

    // 3. Verifica lista/token recebidos via stream
    static unsigned long ultimoCheckToken = 0;
    if (millis() - ultimoCheckToken > 2000)
    {
        ultimoCheckToken = millis();
        if (senhas_consumirMudanca())
        {
            if (!portaAberta)
                telaAguardando();
        }
        static String tokenAnterior = "";
        String tokenAtual = ler_token_nvs();
        if (tokenAtual.length() == 0)
        {
            tokenAnterior = "";
        }
        else if (tokenAtual != tokenAnterior && !portaAberta)
        {
            tokenAnterior = tokenAtual;
            mostrarNoLCD("Comando Pendente", "Digite a chave", "", "Pressione # para OK");
        }
    }

    // 4. Teclado (ignorado com a porta aberta)
    if (portaAberta)
        return;

    byte idxR = 0, idxC = 0;
    char tecla = lerTeclado(&idxR, &idxC);
    if (tecla)
    {
        tlogf("Tecla: %c (R%d C%d)\n", tecla, idxR, idxC);

        if (tecla == '#')
        {
            tlog("Valor digitado: ");
            tlogln(pinDigitado);
            validarChave(pinDigitado);
            pinDigitado = "";
        }
        else if (tecla == '*')
        {
            pinDigitado = "";
            linhaTeclado = "";
            atualizarDisplay();
            tlogln("Buffer limpo.");
        }
        else
        {
            if (pinDigitado.length() < PIN_MAX_LENGTH)
            {
                pinDigitado += tecla;
                String mascara = "";
                for (unsigned int i = 0; i < pinDigitado.length(); i++)
                    mascara += '*';
                linhaTeclado = mascara;
                atualizarDisplay();
                tlog("Buffer atual: ");
                tlogln(pinDigitado);
                if (pinDigitado.length() >= PIN_MAX_LENGTH)
                {
                    validarChave(pinDigitado); // 4 dígitos: valida sozinho
                    pinDigitado = "";
                }
            }
        }
    }
}
