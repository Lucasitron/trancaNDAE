// display.cpp — implementação do LCD com auto-detecção e anti-spam.
#include "display.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "pins.h"
#include "senhas_store.h"
#include "telnet_log.h"

static LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
static bool lcdOK = false;

static String linhaStatus = "";
static String linhaMensagem = "";
static String linhaTeclado = "";
static String linhaRodape = "";

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

static void limparLinha(uint8_t linha)
{
    lcd.setCursor(0, linha);
    for (uint8_t i = 0; i < LCD_COLUMNS; i++)
        lcd.print(" ");
}

static String centralizar(const String &texto)
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

static void atualizarDisplay()
{
    static String ultStatus = "\x01";
    static String ultMsg = "\x01";
    static String ultTeclado = "\x01";
    static String ultRodape = "\x01";
    if (linhaStatus == ultStatus && linhaMensagem == ultMsg &&
        linhaTeclado == ultTeclado && linhaRodape == ultRodape)
        return; // sem mudança: poupa o I2C
    ultStatus = linhaStatus;
    ultMsg = linhaMensagem;
    ultTeclado = linhaTeclado;
    ultRodape = linhaRodape;

    if (!lcdOK)
    {
        // Sem display físico: espelha estrutura (SEM segredos) no log.
        tlogf("[LCD] %s | %s | %d dig. | %s\n",
              linhaStatus.c_str(), linhaMensagem.c_str(),
              linhaTeclado.length(), linhaRodape.c_str());
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
                  const String &teclado, const String &rodape)
{
    linhaStatus = status;
    linhaMensagem = mensagem;
    linhaTeclado = teclado;
    linhaRodape = rodape;
    atualizarDisplay();
}

void display_mostrar4(const String &l0, const String &l1,
                      const String &l2, const String &l3)
{
    linhaStatus = l0;
    linhaMensagem = l1;
    linhaTeclado = l2;
    linhaRodape = l3;
    atualizarDisplay();
}

void telaAguardando()
{
    if (senhas_total() > 0)
    {
        char rodape[21];
        snprintf(rodape, sizeof(rodape), "%d chave(s) ativa(s)", senhas_total());
        mostrarNoLCD("Aguardando...", "Digite a senha + #", "", rodape);
    }
    else
    {
        mostrarNoLCD("Aguardando...", "Sem chaves", "", "Envie pela pagina");
    }
}

String mascarar(const String &s)
{
    String m = "";
    m.reserve(s.length());
    for (unsigned int i = 0; i < s.length(); i++)
        m += '*';
    return m;
}

void display_set_teclado(const String &mascara)
{
    linhaTeclado = mascara;
    atualizarDisplay();
}

void display_limpar_teclado()
{
    linhaTeclado = "";
    atualizarDisplay();
}
