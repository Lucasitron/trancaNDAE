// display.cpp — implementação do LCD com auto-detecção e anti-spam.
//
// Sem heap no caminho de render: as 4 linhas ficam em buffers char fixos
// (LCD_COLUMNS + 1) e cada linha é escrita de uma vez, já preenchida com
// espaços até 20 colunas. Só as linhas alteradas vão ao barramento.
#include "display.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

#include "pins.h"
#include "senhas_store.h"
#include "telnet_log.h"
#include "wifi_manager.h"

// Reescrita periódica: recupera o LCD se ele perder o estado (tela em
// branco) por pico de consumo ao acionar o relé, sem reiniciar o ESP.
static const unsigned long DISPLAY_REFRESH_MS = 1000;

static LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
static bool lcdOK = false;

// Conteúdo atual e última versão efetivamente escrita no painel.
static char atual[LCD_ROWS][LCD_COLUMNS + 1];
static char escrito[LCD_ROWS][LCD_COLUMNS + 1];
static bool cacheValido = false;

bool display_ok()
{
    return lcdOK;
}

static bool escanearI2C()
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

bool display_init()
{
#if USA_LCD
    Wire.begin(); // SDA=21, SCL=22 (padrão)
    if (escanearI2C())
    {
        lcd.init();
        lcd.backlight();
        lcdOK = true;
    }
#else
    tlogln("LCD desligado (USA_LCD=0).");
#endif
    return lcdOK;
}

// Copia para um buffer fixo, truncando em LCD_COLUMNS (sem sair da tela).
static void definirLinha(char *dst, const String &texto)
{
    size_t n = texto.length();
    if (n > LCD_COLUMNS)
        n = LCD_COLUMNS;
    if (n)
        memcpy(dst, texto.c_str(), n);
    dst[n] = '\0';
}

// Escreve exatamente LCD_COLUMNS colunas: completa com espaços (apagando
// restos da linha anterior) e, se pedido, centraliza o texto.
static void escreverLinha(uint8_t linha, const char *texto, bool centralizar)
{
    char buf[LCD_COLUMNS];
    memset(buf, ' ', sizeof(buf));

    size_t n = strlen(texto);
    if (n > sizeof(buf))
        n = sizeof(buf);
    size_t ini = centralizar ? (sizeof(buf) - n) / 2 : 0;
    if (n)
        memcpy(buf + ini, texto, n);

    lcd.setCursor(0, linha);
    // A lib só expõe write(uint8_t); enviamos as 20 colunas de uma vez.
    for (uint8_t i = 0; i < sizeof(buf); i++)
        lcd.write((uint8_t)buf[i]);
}

// true quando a linha i ainda não foi escrita nesta sessão ou mudou.
static bool linhaMudou(uint8_t i)
{
    return !cacheValido || strcmp(atual[i], escrito[i]) != 0;
}

static void atualizarDisplay(bool forcar)
{
    bool mudou[LCD_ROWS];
    bool alguma = false;
    for (uint8_t i = 0; i < LCD_ROWS; i++)
    {
        mudou[i] = linhaMudou(i);
        alguma = alguma || mudou[i];
    }
    if (!alguma && !forcar)
        return; // nada mudou: poupa o I2C

    for (uint8_t i = 0; i < LCD_ROWS; i++)
        strcpy(escrito[i], atual[i]);
    cacheValido = true;

    if (!lcdOK)
    {
        // Sem display físico: espelha estrutura (SEM segredos) no log.
        tlogf("[LCD] %s | %s | %u dig. | %s\n",
              atual[0], atual[1], (unsigned)strlen(atual[2]), atual[3]);
        return;
    }

    static const bool centraliza[LCD_ROWS] = {true, false, false, false};
    for (uint8_t i = 0; i < LCD_ROWS; i++)
        if (forcar || mudou[i])
            escreverLinha(i, atual[i], centraliza[i]);
}

void display_loop()
{
    if (!lcdOK)
        return;

    static unsigned long ultimo = 0;
    unsigned long agora = millis();
    if (agora - ultimo < DISPLAY_REFRESH_MS)
        return;
    ultimo = agora;

    // O PCF8574 pode continuar respondendo mesmo com o HD44780 zerado
    // (pico de consumo do relé). Se o barramento falhar, reconfigura;
    // nos dois casos reescreve tudo e reafirma o backlight.
    Wire.beginTransmission(LCD_I2C_ADDRESS);
    if (Wire.endTransmission() != 0)
    {
        lcd.init();
        tlogln("LCD: sem ACK no I2C, reinicializado.");
    }
    // Reafirma display ON + backlight: um comando corrompido no pico do relé
    // pode ter desligado o painel mesmo com o controlador ainda respondendo.
    lcd.display();
    lcd.backlight();
    atualizarDisplay(true);
}

void mostrarNoLCD(const String &status, const String &mensagem,
                  const String &teclado, const String &rodape)
{
    definirLinha(atual[0], status);
    definirLinha(atual[1], mensagem);
    definirLinha(atual[2], teclado);
    definirLinha(atual[3], rodape);
    atualizarDisplay(false);
}

void telaAguardando()
{
    const char *titulo = wifiConectado ? "Bem vindo!" : "WiFi OFF!";
    if (senhas_total() > 0)
        mostrarNoLCD(titulo, "Digite a senha:", "",
                    wifiConectado ? "Confirme com #" : "Sem rede!");
    else
        mostrarNoLCD(titulo, "Sem chaves!", "",
                    wifiConectado ? "Solicite o envio" : "Sem rede!");
}

void display_set_mascara(unsigned int n)
{
    if (n > LCD_COLUMNS)
        n = LCD_COLUMNS;
    memset(atual[2], '*', n);
    atual[2][n] = '\0';
    atualizarDisplay(false);
}

void display_limpar_teclado()
{
    atual[2][0] = '\0';
    atualizarDisplay(false);
}
