// telnet_log.cpp — Servidor telnet mínimo (1 cliente) + log dual.
#include "telnet_log.h"
#include <WiFi.h>
#include <stdarg.h>

static WiFiServer telnetServer(TELNET_PORT);
static WiFiClient telnetClient;
static String telnetLinha; // buffer do comando digitado

bool telnet_conectado()
{
    return telnetClient && telnetClient.connected();
}

// Envia para o telnet somente (sem Serial).
static void telnet_only(const String &s)
{
    if (telnet_conectado())
        telnetClient.print(s);
}

static void telnet_only_f(const char *fmt, va_list ap)
{
    if (!telnet_conectado())
        return;
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    telnetClient.print(buf);
}

void tlog_raw(const String &s)
{
    Serial.print(s);
    telnet_only(s);
}

void tlog_raw(const char *s)
{
    Serial.print(s);
    telnet_only(String(s));
}

void tlog(const String &s)
{
    tlog_raw(s);
}

void tlogln(const String &s)
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
    telnet_only(String(buf));
}

static void telnet_banner()
{
    telnetClient.println();
    telnetClient.println("=== ESP32 trancaNDAE — log remoto ===");
    telnetClient.println("Comandos: help | status | reboot");
    telnetClient.println("------------------------------------");
}

static void telnet_status();

static void telnet_exec(const String &cmd)
{
    String c = cmd;
    c.trim();
    c.toLowerCase();
    if (c.length() == 0)
        return;
    if (c == "help" || c == "?")
    {
        telnetClient.println("Comandos:");
        telnetClient.println("  status - IP, uptime, heap, RSSI, WiFi");
        telnetClient.println("  reboot - reinicia o ESP32");
        telnetClient.println("  help   - esta ajuda");
    }
    else if (c == "status")
    {
        telnet_status();
    }
    else if (c == "reboot")
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

void telnet_setup()
{
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    Serial.print("📡 Telnet pronto na porta ");
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
            telnetLinha = "";
            telnet_banner();
            // Avisa no USB também.
            Serial.println("📡 Cliente telnet conectado.");
        }
    }

    if (!telnet_conectado())
        return;

    if (!telnetClient.connected())
    {
        telnetClient.stop();
        Serial.println("📡 Cliente telnet desconectado.");
        return;
    }

    // Lê comandos (linha a linha, eco simples).
    while (telnetClient.available())
    {
        char ch = (char)telnetClient.read();
        if (ch == '\r')
            continue;
        if (ch == '\n')
        {
            telnet_exec(telnetLinha);
            telnetLinha = "";
        }
        else if (ch == 0x08 || ch == 0x7F) // backspace
        {
            if (telnetLinha.length() > 0)
                telnetLinha.remove(telnetLinha.length() - 1);
        }
        else if (telnetLinha.length() < 64 && ch >= 32 && ch < 127)
        {
            telnetLinha += ch;
        }
    }
}
