#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

#include "wifi_manager.h"
#include "firebase_client.h"
#include "secrets.h"
#include "telnet_log.h"

// ================= CREDENCIAIS =================
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

volatile bool wifiConectado = false;
String wifiPerfil = "";

// Wi-Fi secundário (NVS "wifi": ssid/pass/on). Configurado pela página.
static Preferences wifiPrefs;

bool wifi2_ativo()
{
    wifiPrefs.begin("wifi", true);
    bool on = wifiPrefs.isKey("on") && wifiPrefs.getUChar("on", 0) != 0 &&
              wifiPrefs.isKey("ssid") && wifiPrefs.getString("ssid", "").length() > 0;
    wifiPrefs.end();
    return on;
}

static bool wifi2_valido(const String &ssid, const String &pass)
{
    if (ssid.length() < 1 || ssid.length() > 32)
        return false;
    if (pass.length() > 63)
        return false;
    for (unsigned int i = 0; i < ssid.length(); i++)
    {
        char c = ssid[i];
        if (c == ':' || c < 32 || c > 126)
            return false;
    }
    for (unsigned int i = 0; i < pass.length(); i++)
    {
        char c = pass[i];
        if (c == ':' || c < 32 || c > 126)
            return false;
    }
    return true;
}

bool wifi2_salvar(const String &ssid, const String &pass)
{
    if (!wifi2_valido(ssid, pass))
        return false;
    wifiPrefs.begin("wifi", false);
    wifiPrefs.putString("ssid", ssid);
    wifiPrefs.putString("pass", pass);
    wifiPrefs.putUChar("on", 1);
    wifiPrefs.end();
    tlogln("Wi-Fi secundário salvo e ATIVO.");
    return true;
}

void wifi2_desativar()
{
    wifiPrefs.begin("wifi", false);
    wifiPrefs.putUChar("on", 0);
    wifiPrefs.end();
    tlogln("Wi-Fi secundário DESATIVADO (só padrão).");
}

// ================= IP FIXO MULTI-REDE =================
// Estratégia: conecta por DHCP primeiro (pega gateway/DNS corretos de
// QUALQUER rede), descobre em qual sub-rede estamos e, se ela casar com
// um perfil, reaplica aquele IP fixo (mesmo gateway/DNS do DHCP).
// Assim nunca associamos com um IP de outra rede.
static bool mesmo24(IPAddress a, IPAddress b)
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

// Conecta via DHCP com as credenciais dadas. Retorna true se WL_CONNECTED.
static bool conectarDHCP(const char *rede, const char *senha, unsigned long timeoutMs)
{
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(rede, senha);
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
static bool aplicarFixo(const char *rede, const char *senha,
                        IPAddress ip, IPAddress gateway, IPAddress dns, IPAddress subnet)
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
    WiFi.begin(rede, senha);
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

// Tenta UMA rede (DHCP para descobrir + fixo se a sub-rede casar).
// fixoRef: IP de referência do perfil (STATIC_IP / STATIC_IP2).
static bool tentarRede(const char *rotulo, const char *rede, const char *senha,
                       IPAddress fixoRef)
{
    tlog("Rede ");
    tlog_raw(rotulo);
    tlog_raw(" (");
    tlog_raw(rede);
    tlogln("):");

    if (!conectarDHCP(rede, senha, 10000))
        return false;

#if USE_STATIC_IP
    IPAddress lease = WiFi.localIP();
    if (mesmo24(lease, fixoRef))
    {
        IPAddress gw = WiFi.gatewayIP();
        IPAddress dns = WiFi.dnsIP();
        IPAddress subnet = WiFi.subnetMask();
        tlog("Sub-rede do perfil. Aplicando fixo: ");
        tlogln(fixoRef.toString());
        if (!aplicarFixo(rede, senha, fixoRef, gw, dns, subnet))
        {
            tlogln("Falha no IP fixo; voltando para DHCP.");
            if (!conectarDHCP(rede, senha, 8000))
                return false;
        }
    }
    else
    {
        tlogln("Fora do perfil fixo; mantendo DHCP.");
    }
#endif
    return true;
}

// ================= CONEXÃO WI-FI =================
void conectarWiFi()
{
    tlogln("\nIniciando conexão Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    wifiPerfil = "";

    // 1) Padrão (secrets.h) — sempre primeiro.
    bool ok = tentarRede("padrao", ssid, password, IPAddress(STATIC_IP));

    // 2) Secundário (NVS) — só se ativo e o padrão falhou.
    if (!ok && wifi2_ativo())
    {
        wifiPrefs.begin("wifi", true);
        String s2 = wifiPrefs.getString("ssid", "");
        String p2 = wifiPrefs.getString("pass", "");
        wifiPrefs.end();
        if (tentarRede("secundaria", s2.c_str(), p2.c_str(), IPAddress(STATIC_IP2)))
        {
            ok = true;
            wifiPerfil = "secundario";
        }
    }
    if (ok && wifiPerfil.length() == 0)
        wifiPerfil = "padrao";

    if (ok)
    {
        tlogln("\nWi-Fi Conectado com sucesso!");
        tlog("Perfil: ");
        tlogln(wifiPerfil);
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

// Reconecta (padrão -> secundário) e rearma OTA. Para retry offline.
void wifi_reconectar()
{
    tlogln("Tentando reconectar Wi-Fi...");
    conectarWiFi();
    if (wifiConectado)
        iniciarOTA(); // rearma o UDP após a queda
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
        wifiPerfil = "";
        firebase_reset_stream(); // stream SSE morreu: recria ao voltar
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