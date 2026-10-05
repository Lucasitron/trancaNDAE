#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "wifi_manager.h"
#include "secrets.h"
#include "telnet_log.h"

// ================= CREDENCIAIS =================
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

volatile bool wifiConectado = false;

// ================= IP FIXO MULTI-REDE =================
// Estratégia: conecta por DHCP primeiro (pega gateway/DNS corretos de
// QUALQUER rede), descobre em qual sub-rede estamos e, se ela casar com
// um perfil, reaplica aquele IP fixo (mesmo gateway/DNS do DHCP).
// Assim nunca associamos com um IP de outra rede.
static bool mesmo24(IPAddress a, IPAddress b)
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

// Conecta via DHCP. Retorna true se WL_CONNECTED dentro do timeout.
static bool conectarDHCP(unsigned long timeoutMs)
{
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(ssid, password);
    tlog("DHCP: conectando");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0 < timeoutMs))
    {
        delay(300);
        tlog_raw(".");
    }
    return WiFi.status() == WL_CONNECTED;
}

// Reaplica IP fixo (mesmo gateway/DNS) e reconecta. true se conectou.
static bool aplicarFixo(IPAddress ip, IPAddress gateway, IPAddress dns, IPAddress subnet)
{
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    if (!WiFi.config(ip, gateway, subnet, dns))
    {
        tlogln("Falha WiFi.config; mantendo DHCP.");
        return false;
    }
    WiFi.begin(ssid, password);
    tlog("IP fixo: reconectando em ");
    tlogln(ip.toString());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0 < 8000))
    {
        delay(300);
        tlog_raw(".");
    }
    return WiFi.status() == WL_CONNECTED;
}

// ================= CONEXÃO WI-FI =================
void conectarWiFi()
{
    tlogln("\nIniciando conexão Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);

    // 1) DHCP: descobre a rede real (gateway/DNS corretos).
    bool ok = conectarDHCP(12000);

#if USE_STATIC_IP
    if (ok)
    {
        IPAddress lease = WiFi.localIP();
        IPAddress gw = WiFi.gatewayIP();
        IPAddress dns = WiFi.dnsIP();
        IPAddress subnet = WiFi.subnetMask();
        IPAddress alvo;

        if (mesmo24(lease, IPAddress(STATIC_IP)))
            alvo = IPAddress(STATIC_IP); // perfil A (10.0.0.x)
#if defined(STATIC_IP2)
        else if (mesmo24(lease, IPAddress(STATIC_IP2)))
            alvo = IPAddress(STATIC_IP2); // perfil B (192.168.1.x)
#endif
        else
        {
            tlogln("Rede fora dos perfis fixos; mantendo DHCP.");
            alvo = IPAddress(0, 0, 0, 0);
        }

        if (alvo != IPAddress(0, 0, 0, 0))
        {
            tlog("Sub-rede detectada. Aplicando IP fixo do perfil: ");
            tlogln(alvo.toString());
            if (!aplicarFixo(alvo, gw, dns, subnet))
            {
                tlogln("Falha no IP fixo; voltando para DHCP.");
                ok = conectarDHCP(10000);
            }
        }
    }
#endif

    if (ok)
    {
        tlogln("\nWi-Fi Conectado com sucesso!");
        tlog("Endereço IP: ");
        tlogln(WiFi.localIP().toString());
        tlog("Gateway: ");
        tlogln(WiFi.gatewayIP().toString());
    }
    else
    {
        tlogln("\nFalha ao conectar no Wi-Fi. Verifique credenciais/rede.");
    }
}

// ================= CALLBACK DE EVENTOS WI-FI =================
void eventoWiFi(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        tlogln("📡 Wi-Fi Conectado ao roteador!");
        break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        tlog("✅ IP Obtido: ");
        tlogln(WiFi.localIP().toString());
        wifiConectado = true;
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        tlogln("❌ Wi-Fi Desconectado!");
        wifiConectado = false;
        break;

    default:
        break;
    }
}

// ================= OTA =================
void iniciarOTA()
{
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    // Também é possível restringir a porta ou MDNS:
    // ArduinoOTA.setPort(3232);
    // ArduinoOTA.setMdnsEnabled(true);

    ArduinoOTA.onStart([]()
                       {
                            String tipo = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
                            tlogln("\nOTA iniciado - atualizando ");
                            tlogln(tipo);
                           // Se quiser, desligue o relé por segurança durante a atualização
                           digitalWrite(PINO_RELE, LOW);
                       });

    ArduinoOTA.onEnd([]()
                     { tlogln("\nOTA finalizado com sucesso. Reiniciando..."); });

    ArduinoOTA.onProgress([](unsigned int progresso, unsigned int total)
                          {
        // Imprime o progresso a cada 10%
        static unsigned int ultimoPct = 0;
        unsigned int pct = (progresso * 100) / total;
        if (pct / 10 != ultimoPct / 10) {
            tlogf("Progresso OTA: %u%%\n", pct);
            ultimoPct = pct;
        } });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        tlogf("Erro OTA [%u]: ", error);
        if (error == OTA_AUTH_ERROR)         tlogln("Falha de autenticação");
        else if (error == OTA_BEGIN_ERROR)   tlogln("Falha no início");
        else if (error == OTA_CONNECT_ERROR) tlogln("Falha na conexão");
        else if (error == OTA_RECEIVE_ERROR) tlogln("Falha no recebimento");
        else if (error == OTA_END_ERROR)     tlogln("Falha no final"); });

    ArduinoOTA.begin();
    tlog("OTA pronto. Hostname: ");
    tlog_raw(OTA_HOSTNAME);
    tlog_raw(".local | IP: ");
    tlogln(WiFi.localIP().toString());
}

// Chame isso dentro do loop() principal para processar OTA
void processarOTA()
{
    ArduinoOTA.handle();
}