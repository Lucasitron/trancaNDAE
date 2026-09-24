// main.ino
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ================= Módulos do Projeto =================
#include "pins.h"            // Todas as definições de pinos
#include "secrets.h"         // Credenciais (Wi-Fi, Firebase, OTA, IP fixo)
#include "wifi_manager.h"    // Wi-Fi, IP fixo e OTA
#include "firebase_client.h" // Firebase + NVS do token

// ================= Configurações do Teclado =================
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {KEYPAD_ROW_1, KEYPAD_ROW_2, KEYPAD_ROW_3, KEYPAD_ROW_4};
byte colPins[COLS] = {KEYPAD_COL_1, KEYPAD_COL_2, KEYPAD_COL_3};
Keypad teclado = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ================= Objetos Globais =================
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// ================= Estado da Validação =================
String pinDigitado = "";
const int PIN_MAX_LENGTH = 16;
const char *PALAVRA_ESPERADA = "ABRIR"; // Palavra-comando que o PC cifrou

// ================= Funções Auxiliares =================
void mostrarNoLCD(const String &linha1, const String &linha2 = "")
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(linha1);
    if (linha2.length() > 0)
    {
        lcd.setCursor(0, 1);
        lcd.print(linha2);
    }
}

// Calcula HMAC-SHA256 (versão recomendada para validar a chave)
#include <mbedtls/md.h>
String hmacSha256(const String &mensagem, const String &chave)
{
    byte hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)chave.c_str(), chave.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)mensagem.c_str(), mensagem.length());
    mbedtls_md_hmac_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    char hex[33];
    for (int i = 0; i < 16; i++)
        sprintf(hex + (i * 2), "%02x", hash[i]);
    hex[32] = '\0';
    return String(hex);
}

// ================= Validação da Chave =================
void validarChave(const String &chave)
{
    String token = ler_token_nvs();

    if (token.length() == 0)
    {
        mostrarNoLCD("Sem comando", "pendente");
        tone(PINO_BUZZER, 400, 300);
        return;
    }

    // --- OPÇÃO RECOMENDADA: HMAC ---
    String hashCalculado = hmacSha256(PALAVRA_ESPERADA, chave);
    bool valido = (hashCalculado == token);

    // --- OPÇÃO ALTERNATIVA: XOR (sua ideia original) ---
    // (requer Base64 decode antes — descomente se preferir)
    // String decifrado = xorCipher(base64Decode(token), chave);
    // bool valido = (decifrado == PALAVRA_ESPERADA);

    if (valido)
    {
        mostrarNoLCD("OK! Acionando...", "Rele alternado");
        tone(PINO_BUZZER, 1500, 200);

        // Comuta o relé
        digitalWrite(PINO_RELE, !digitalRead(PINO_RELE));

        // 🔒 Anti-replay: consome o token (só pode ser usado uma vez)
        limpar_token_nvs();

        delay(2000);
        mostrarNoLCD("Aguardando...", "Comando remoto");
    }
    else
    {
        mostrarNoLCD("Chave invalida", "Tente novamente");
        tone(PINO_BUZZER, 400, 300);
        delay(1500);
        mostrarNoLCD("Digite a chave:", "");
    }
}

// ================= SETUP =================
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Iniciando Sistema ===");

    // 1. Configuração dos pinos
    pinMode(PINO_RELE, OUTPUT);
    digitalWrite(PINO_RELE, LOW); // Relé começa desligado
    pinMode(PINO_BUZZER, OUTPUT);
    digitalWrite(PINO_BUZZER, LOW);

    // 2. LCD
    lcd.init();
    lcd.backlight();
    mostrarNoLCD("Sistema Iniciado", "Aguardando...");
    tone(PINO_BUZZER, 1000, 200);
    delay(300);

    // 3. Registra callback de eventos Wi-Fi (antes de conectar)
    WiFi.onEvent(eventoWiFi);

    // 4. Conecta ao Wi-Fi (com IP fixo, se USE_STATIC_IP=true no secrets.h)
    conectarWiFi();

    // 5. Inicia OTA (só se Wi-Fi conectou)
    if (wifiConectado)
    {
        iniciarOTA();

        // 6. Inicia Firebase (streaming do nó de comandos)
        iniciarFirebase();

        mostrarNoLCD("WiFi OK", "Aguardando cmd");
    }
    else
    {
        mostrarNoLCD("WiFi OFF", "Verifique config");
    }

    // 7. Recupera token pendente (caso tenha reiniciado antes de usar)
    String tokenPendente = ler_token_nvs();
    if (tokenPendente.length() > 0)
    {
        Serial.println("♻️ Token pendente na NVS: " + tokenPendente);
        mostrarNoLCD("Comando pendente", "Digite a chave:");
    }

    Serial.println("=== Sistema pronto ===\n");
}

// ================= LOOP =================
void loop()
{
    // 1. Processa OTA (não-bloqueante)
    processarOTA();

    // 2. Processa Firebase (streaming assíncrono)
    processarFirebase();

    // 3. Verifica se há novo token recebido via stream
    static unsigned long ultimoCheckToken = 0;
    if (millis() - ultimoCheckToken > 2000)
    {
        ultimoCheckToken = millis();
        static String tokenAnterior = "";
        String tokenAtual = ler_token_nvs();
        if (tokenAtual != tokenAnterior && tokenAtual.length() > 0)
        {
            tokenAnterior = tokenAtual;
            mostrarNoLCD("Comando pendente", "Digite a chave:");
            tone(PINO_BUZZER, 1500, 150);
        }
    }

    // 4. Leitura do teclado
    char tecla = teclado.getKey();
    if (tecla)
    {
        Serial.print("Tecla: ");
        Serial.println(tecla);

        if (tecla == '#')
        {
            // Confirma a chave
            validarChave(pinDigitado);
            pinDigitado = "";
        }
        else if (tecla == '*')
        {
            // Cancela / limpa
            pinDigitado = "";
            mostrarNoLCD("Digite a chave:", "");
        }
        else
        {
            // Adiciona dígito (mostra como asteriscos)
            if (pinDigitado.length() < PIN_MAX_LENGTH)
            {
                pinDigitado += tecla;
                String mascara = "";
                for (unsigned int i = 0; i < pinDigitado.length(); i++)
                    mascara += '*';
                lcd.setCursor(0, 1);
                lcd.print("                "); // limpa linha
                lcd.setCursor(0, 1);
                lcd.print(mascara);
            }
        }
    }
}