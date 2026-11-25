// telnet_log.cpp — Servidor telnet mínimo (1 cliente) + log dual.
// Sem alocação de String por chamada: literais/formatos vão direto para
// Serial e WiFiClient, que aceitam const char*.
#include "secrets.h" // TELNET_PORT
#include "telnet_log.h"
#include <WiFi.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

static WiFiServer telnetServer(TELNET_PORT);
static WiFiClient telnetClient;

// Buffer do comando digitado (fixo: até 64 chars, sem heap).
static char telnetLinha[65];
static uint8_t telnetLen = 0;

bool telnet_conectado()
{
    return telnetClient && telnetClient.connected();
}

void tlog_raw(const String &s)
{
    Serial.print(s);
    if (telnet_conectado())
        telnetClient.print(s);
}

void tlog_raw(const char *s)
{
    Serial.print(s);
    if (telnet_conectado())
        telnetClient.print(s);
}

void tlog(const String &s)
{
    tlog_raw(s);
}

void tlog(const char *s)
{
    tlog_raw(s);
}

void tlogln(const String &s)
{
    Serial.println(s);
    if (telnet_conectado())
        telnetClient.println(s);
}

void tlogln(const char *s)
{
    Serial.println(s);
    if (telnet_conectado())
        telnetClient.println(s);
}

void tlogf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Serial.print(buf);
    if (telnet_conectado())
        telnetClient.print(buf);
}

static void telnet_banner()
{
    telnetClient.println();
    telnetClient.println("=== ESP32 trancaNDAE — log remoto ===");
    telnetClient.println("Comandos: help | status | reboot");
    telnetClient.println("------------------------------------");
}

// Status sem puxar firebase_client.h (evita dependência circular).
static void telnet_status()
{
    telnetClient.print("IP: ");
    telnetClient.println(WiFi.localIP().toString());
    telnetClient.print("RSSI: ");
    telnetClient.print(WiFi.RSSI());
    telnetClient.println(" dBm");
    telnetClient.print("Uptime: ");
    telnetClient.print(millis() / 1000);
    telnetClient.println(" s");
    telnetClient.print("Heap livre: ");
    telnetClient.print(ESP.getFreeHeap());
    telnetClient.println(" bytes");
    telnetClient.print("Telnet: conectado na porta ");
    telnetClient.println(TELNET_PORT);
}

// Compara o comando já normalizado (minúsculo, sem espaços nas pontas).
static void telnet_exec(const char *cmd)
{
    char c[sizeof(telnetLinha)];
    size_t ini = 0, fim = strlen(cmd);
    while (ini < fim && isspace((unsigned char)cmd[ini]))
        ini++;
    while (fim > ini && isspace((unsigned char)cmd[fim - 1]))
        fim--;

    size_t n = 0;
    for (size_t i = ini; i < fim && n < sizeof(c) - 1; i++)
        c[n++] = (char)tolower((unsigned char)cmd[i]);
    c[n] = '\0';
    if (n == 0)
        return;

    if (!strcmp(c, "help") || !strcmp(c, "?"))
    {
        telnetClient.println("Comandos:");
        telnetClient.println("  status - IP, uptime, heap, RSSI, WiFi");
        telnetClient.println("  reboot - reinicia o ESP32");
        telnetClient.println("  help   - esta ajuda");
    }
    else if (!strcmp(c, "status"))
    {
        telnet_status();
    }
    else if (!strcmp(c, "reboot"))
    {
        telnetClient.println("Reiniciando...");
        delay(300);
        ESP.restart();
    }
    else
    {
        telnetClient.print("Comando desconhecido: ");
        telnetClient.println(cmd);
    }
}

static void telnet_limpar_linha()
{
    telnetLen = 0;
    telnetLinha[0] = '\0';
}

void telnet_setup()
{
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    Serial.print("Telnet pronto na porta ");
    Serial.print(TELNET_PORT);
    Serial.print(" (");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.print(TELNET_PORT);
    Serial.println(")");
}

void telnet_loop()
{
    // Novo cliente? Derruba o antigo (1 slot só, economiza RAM).
    if (telnetServer.hasClient())
    {
        WiFiClient novo = telnetServer.accept();
        if (telnet_conectado())
        {
            // Já ocupado: recusa educadamente.
            novo.println("Servidor ocupado (1 cliente por vez). Tente depois.");
            novo.stop();
        }
        else
        {
            telnetClient = novo;
            telnet_limpar_linha();
            telnet_banner();
            Serial.println("Cliente telnet conectado.");
        }
    }

    if (!telnet_conectado())
    {
        if (telnetClient) // socket morreu: libera o slot e o buffer
        {
            telnetClient.stop();
            telnet_limpar_linha();
            Serial.println("Cliente telnet desconectado.");
        }
        return;
    }

    // Lê comandos (linha a linha).
    while (telnetClient.available())
    {
        char ch = (char)telnetClient.read();
        if (ch == '\r')
            continue;
        if (ch == '\n')
        {
            telnet_exec(telnetLinha);
            telnet_limpar_linha();
        }
        else if (ch == 0x08 || ch == 0x7F) // backspace
        {
            if (telnetLen > 0)
                telnetLinha[--telnetLen] = '\0';
        }
        else if (telnetLen < sizeof(telnetLinha) - 1 && ch >= 32 && ch < 127)
        {
            telnetLinha[telnetLen++] = ch;
            telnetLinha[telnetLen] = '\0';
        }
    }
}
