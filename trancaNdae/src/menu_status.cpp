// menu_status.cpp — implementação das telas de diagnóstico.
#include "menu_status.h"
#include <WiFi.h>
#include "display.h"
#include "fechadura.h"
#include "wifi_manager.h"
#include "firebase_client.h"
#include "senhas_store.h"
#include "secrets.h"
#include "telnet_log.h"

static bool modoStatus = false;
static uint8_t telaStatus = 0;
static uint8_t scrollStatus = 0;
static const uint8_t N_TELAS_STATUS = 4;
static String ultimoEvento = "boot";

void registrarEvento(const String &ev)
{
    ultimoEvento = ev;
    tlogln(ev);
}

bool emModoStatus()
{
    return modoStatus;
}

// Monta as linhas da tela (máx. 6; telas curtas usam 4).
static void linhasStatus(uint8_t tela, String out[6], uint8_t &nLinhas)
{
    char buf[21];
    switch (tela)
    {
    case 0: // Wi-Fi
        nLinhas = 4;
        out[0] = "== Wi-Fi ==";
        out[1] = "Rede: " + String(WIFI_SSID);
        out[2] = "IP: " + WiFi.localIP().toString();
        snprintf(buf, sizeof(buf), "RSSI: %d dBm", WiFi.RSSI());
        out[3] = String(buf);
        break;
    case 1: // Firebase
        nLinhas = 4;
        out[0] = "== Firebase ==";
        out[1] = wifiConectado ? "Rede: conectada" : "Rede: OFFLINE";
        out[2] = firebase_pronto() ? "Auth: OK (stream)" : "Auth: pendente";
        out[3] = "Cmd: consome+apaga";
        break;
    case 2: // Chaves (só contadores — PINs nunca no display)
        nLinhas = 4;
        out[0] = "== Chaves ==";
        snprintf(buf, sizeof(buf), "Ativas: %d/%d", senhas_total_ativas(), MAX_SENHAS);
        out[1] = String(buf);
        out[2] = "Abertura: so com #";
        out[3] = "Ult: " + ultimoEvento;
        break;
    default: // Sistema (rolável: 6 linhas)
        nLinhas = 6;
        out[0] = "== Sistema ==";
        snprintf(buf, sizeof(buf), "Uptime: %lus", millis() / 1000);
        out[1] = String(buf);
        snprintf(buf, sizeof(buf), "Heap: %u", ESP.getFreeHeap());
        out[2] = String(buf);
        out[3] = fechadura_aberta() ? "Rele: ABERTO" : "Rele: fechado";
        out[4] = "Reset: 0000 + *";
        out[5] = "Status: admin + *";
        break;
    }
}

static void desenharStatus()
{
    String linhas[6];
    uint8_t n = 4;
    linhasStatus(telaStatus, linhas, n);
    if (scrollStatus + 4 > n)
        scrollStatus = (n > 4) ? n - 4 : 0;
    String rodape = (n > 4) ? "2/8 rola 4/6 tela *=sai" : "*=sai 4/6=tela";
    display_mostrar4(linhas[scrollStatus + 0],
                     (n > (scrollStatus + 1)) ? linhas[scrollStatus + 1] : "",
                     (n > (scrollStatus + 2)) ? linhas[scrollStatus + 2] : "",
                     rodape);
}

void entrarStatus()
{
    modoStatus = true;
    telaStatus = 0;
    scrollStatus = 0;
    desenharStatus();
    tlogln("Modo status ativado.");
}

void sairStatus()
{
    modoStatus = false;
    tlogln("Modo status encerrado.");
    telaAguardando();
}

bool processarTeclaStatus(char tecla)
{
    String linhas[6];
    uint8_t n = 4;
    linhasStatus(telaStatus, linhas, n);
    uint8_t maxScroll = (n > 4) ? n - 4 : 0;

    switch (tecla)
    {
    case '*':
        sairStatus();
        break;
    case '2': // rola para cima
        scrollStatus = (scrollStatus == 0) ? maxScroll : scrollStatus - 1;
        desenharStatus();
        break;
    case '8': // rola para baixo
        scrollStatus = (scrollStatus >= maxScroll) ? 0 : scrollStatus + 1;
        desenharStatus();
        break;
    case '4': // tela anterior
        telaStatus = (telaStatus == 0) ? N_TELAS_STATUS - 1 : telaStatus - 1;
        scrollStatus = 0;
        desenharStatus();
        break;
    case '6': // próxima tela
        telaStatus = (telaStatus + 1) % N_TELAS_STATUS;
        scrollStatus = 0;
        desenharStatus();
        break;
    default:
        break; // demais teclas ignoradas no modo status
    }
    return true; // sempre consome
}
