// main.ino
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ================= Módulos do Projeto =================
#include "pins.h"
#include "secrets.h"
#include "wifi_manager.h"
#include "firebase_client.h"
#include "senhas_store.h"
#include "telnet_log.h"

// ================= Configurações do Teclado (3x4) =================
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
const int PIN_MAX_LENGTH = PIN_LEN; // senhas de 4 dígitos (senhas_store.h)
const char *PALAVRA_ESPERADA = "ABRIR";

// ================= Variáveis de Estado do Display =================
String linhaStatus = "Sistema Iniciado";
String linhaMensagem = "Aguardando...";
String linhaTeclado = "";
String linhaRodape = "Aguardando comando";

// ================= Funções Auxiliares =================
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

// ================= HMAC-SHA256 (truncado) =================
// Calcula o HMAC-SHA256 completo mas retorna só os primeiros
// TOKEN_HEX_LEN/2 bytes em hex (TOKEN_HEX_LEN=4 -> 2 bytes -> "a3f5").
// Deve ser idêntico ao gerarToken() da web (web/js/models/TokenModel.js).
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

    char hex[TOKEN_HEX_LEN + 1];
    for (int i = 0; i < TOKEN_HEX_LEN / 2; i++)
        sprintf(hex + (i * 2), "%02x", hash[i]);
    hex[TOKEN_HEX_LEN] = '\0';
    return String(hex);
}

// ================= Validação da Chave =================
// 1) Tabela de senhas cadastradas (gerenciador web -> /senhas/.../lista).
// 2) Fallback: comando único legado (/comandos/dispositivo1, uso único).
void validarChave(const String &chave)
{
    String hashCalculado = hmacSha256(PALAVRA_ESPERADA, chave);

    if (senhas_contem(hashCalculado))
    {
        mostrarNoLCD(">>> SUCESSO <<<", "Acesso liberado", "", "Chave cadastrada OK");
        tone(PINO_BUZZER, 1500, 200);

        digitalWrite(PINO_RELE, !digitalRead(PINO_RELE));

        delay(2000);
        mostrarNoLCD("Aguardando...", "Comando remoto", "", "Digite a chave + #");
        return;
    }

    String token = ler_token_nvs();

    if (token.length() == 0)
    {
        mostrarNoLCD("Sem comando", "Nenhum comando pendente", "", "Aguarde o PC enviar");
        tone(PINO_BUZZER, 400, 300);
        delay(1500);
        mostrarNoLCD("Aguardando...", "Comando remoto", "", "Digite a chave + #");
        return;
    }

    String hashCalculado2 = hmacSha256(PALAVRA_ESPERADA, chave);
    bool valido = (hashCalculado2 == token);

    if (valido)
    {
        mostrarNoLCD(">>> SUCESSO <<<", "Rele alternado", "", "Comando consumido");
        tone(PINO_BUZZER, 1500, 200);

        digitalWrite(PINO_RELE, !digitalRead(PINO_RELE));
        limpar_token_nvs(); // Anti-replay

        delay(2000);
        mostrarNoLCD("Aguardando...", "Comando remoto", "", "Digite a chave + #");
    }
    else
    {
        mostrarNoLCD("!! INVALIDO !!", "Chave incorreta", "", "Tente novamente");
        tone(PINO_BUZZER, 400, 300);
        delay(1500);
        mostrarNoLCD("Aguardando...", "Comando remoto", "", "Digite a chave + #");
    }
}

// ================= SETUP =================
void setup()
{
    Serial.begin(115200);
    delay(500);
    tlogln("\n=== Iniciando Sistema ===");

    // 0. Tabela de senhas cadastradas (NVS -> RAM, máx. MAX_SENHAS)
    senhas_init();

    // 1. Pinos
    pinMode(PINO_RELE, OUTPUT);
    digitalWrite(PINO_RELE, LOW);
    pinMode(PINO_BUZZER, OUTPUT);
    digitalWrite(PINO_BUZZER, LOW);



    // 3. LCD
    lcd.init();
    lcd.backlight();
    mostrarNoLCD("Sistema Iniciado", "Bem-vindo!", "", "Versao 1.0");
    tone(PINO_BUZZER, 1000, 200);
    delay(800);

    // 4. Wi-Fi
    WiFi.onEvent(eventoWiFi);
    conectarWiFi();

    // 5. OTA + Firebase + Telnet
    if (wifiConectado)
    {
        iniciarOTA();
        telnet_setup(); // log remoto: telnet <IP> 23
        iniciarFirebase();
        mostrarNoLCD("WiFi OK", "Sistema Online", "", "Aguardando comando");
    }
    else
    {
        mostrarNoLCD("WiFi OFF", "Sem conexao", "", "Verifique config");
    }

    // 6. Recupera token pendente da NVS (migração: descarta formato antigo)
    String tokenPendente = ler_token_nvs();
    if (tokenPendente.length() > 0 && tokenPendente.length() != TOKEN_HEX_LEN)
    {
        limpar_token_nvs(); // token do formato antigo (32/64 hex) — inválido agora
        tokenPendente = "";
    }
    if (tokenPendente.length() > 0)
    {
        tlogln("Token pendente na NVS: " + tokenPendente);
        mostrarNoLCD("Comando Pendente", "Digite a chave", "", "Pressione # para OK");
    }
    else if (senhas_total() > 0)
    {
        char rodape[21];
        snprintf(rodape, sizeof(rodape), "%d chave(s) ativa(s)", senhas_total());
        mostrarNoLCD("Sistema Online", "Digite a chave + #", "", rodape);
    }

    tlogln("=== Sistema pronto ===");
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

    // 3. Verifica novo token recebido via stream
    static unsigned long ultimoCheckToken = 0;
    if (millis() - ultimoCheckToken > 2000)
    {
        ultimoCheckToken = millis();
        if (senhas_consumirMudanca())
        {
            char rodape[21];
            snprintf(rodape, sizeof(rodape), "%d chave(s) ativa(s)", senhas_total());
            mostrarNoLCD("Lista atualizada", "Digite a chave + #", "", rodape);
            tone(PINO_BUZZER, 1500, 150);
        }
        static String tokenAnterior = "";
        String tokenAtual = ler_token_nvs();
        if (tokenAtual.length() == 0)
        {
            // Token consumido/limpo: reseta para não perder o próximo aviso
            tokenAnterior = "";
        }
        else if (tokenAtual != tokenAnterior)
        {
            tokenAnterior = tokenAtual;
            mostrarNoLCD("Comando Pendente", "Digite a chave", "", "Pressione # para OK");
            tone(PINO_BUZZER, 1500, 150);
        }
    }

    // 4. Teclado
    char tecla = teclado.getKey();
    if (tecla)
    {
        tlog("Tecla: ");
        tlogln(String(tecla));

        if (tecla == '#')
        {
            validarChave(pinDigitado);
            pinDigitado = "";
        }
        else if (tecla == '*')
        {
            pinDigitado = "";
            linhaTeclado = "";
            atualizarDisplay();
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
                if (pinDigitado.length() >= PIN_MAX_LENGTH)
                {
                    validarChave(pinDigitado); // PIN de 4 dígitos: valida sozinho
                    pinDigitado = "";
                }
            }
        }
    }
}