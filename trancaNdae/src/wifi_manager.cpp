#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "wifi_manager.h"
#include "secrets.h"

// ================= CREDENCIAIS =================
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

volatile bool wifiConectado = false;

// ================= IP FIXO =================
void configurarIPFixo()
{
#if USE_STATIC_IP
    IPAddress ip(STATIC_IP);
    IPAddress gateway(STATIC_GATEWAY);
    IPAddress subnet(STATIC_SUBNET);
    IPAddress dns(STATIC_DNS);

    // WiFi.config deve ser chamado ANTES do WiFi.begin
    if (!WiFi.config(ip, gateway, subnet, dns))
    {
        Serial.println("⚠️ Falha ao configurar IP estático. Usando DHCP.");
    }
    else
    {
        Serial.print("🔧 IP fixo configurado: ");
        Serial.println(ip);
    }
#else
    Serial.println("🔧 Modo DHCP ativo (IP dinâmico).");
#endif
}

// ================= CONEXÃO WI-FI =================
void conectarWiFi()
{
    Serial.println("\nIniciando conexão Wi-Fi...");

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME); // Define hostname (útil para mDNS/OTA)
    configurarIPFixo();             // Aplica IP fixo antes do begin

    WiFi.begin(ssid, password);

    unsigned long tempoInicio = millis();
    const unsigned long timeoutConexao = 10000;

    while (WiFi.status() != WL_CONNECTED && (millis() - tempoInicio < timeoutConexao))
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n✅ Wi-Fi Conectado com sucesso!");
        Serial.print("Endereço IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\n❌ Falha ao conectar no Wi-Fi. Verifique as credenciais.");
    }
}

// ================= CALLBACK DE EVENTOS WI-FI =================
void eventoWiFi(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("📡 Wi-Fi Conectado ao roteador!");
        break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("✅ IP Obtido: ");
        Serial.println(WiFi.localIP());
        wifiConectado = true;
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("❌ Wi-Fi Desconectado!");
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
                           Serial.println("\n🔄 OTA iniciado — atualizando ");
                           Serial.println(tipo);
                           // Se quiser, desligue o relé por segurança durante a atualização
                           digitalWrite(PINO_RELE, LOW);
                       });

    ArduinoOTA.onEnd([]()
                     { Serial.println("\n✅ OTA finalizado com sucesso. Reiniciando..."); });

    ArduinoOTA.onProgress([](unsigned int progresso, unsigned int total)
                          {
        // Imprime o progresso a cada 10%
        static unsigned int ultimoPct = 0;
        unsigned int pct = (progresso * 100) / total;
        if (pct / 10 != ultimoPct / 10) {
            Serial.printf("⏳ Progresso: %u%%\n", pct);
            ultimoPct = pct;
        } });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        Serial.printf("❌ Erro OTA [%u]: ", error);
        if (error == OTA_AUTH_ERROR)         Serial.println("Falha de autenticação");
        else if (error == OTA_BEGIN_ERROR)   Serial.println("Falha no início");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na conexão");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha no recebimento");
        else if (error == OTA_END_ERROR)     Serial.println("Falha no final"); });

    ArduinoOTA.begin();
    Serial.print("🚀 OTA pronto. Hostname: ");
    Serial.print(OTA_HOSTNAME);
    Serial.print(".local | IP: ");
    Serial.println(WiFi.localIP());
}

// Chame isso dentro do loop() principal para processar OTA
void processarOTA()
{
    ArduinoOTA.handle();
}