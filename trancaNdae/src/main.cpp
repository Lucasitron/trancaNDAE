// main.cpp — Fechadura eletrônica (orquestração).
// Especialistas: teclado.* (varredura) | display.* (LCD) |
// fechadura.* (relé/validação/reset) | menu_status.* (diagnóstico) |
// wifi_manager / firebase_client / senhas_store / telnet_log (serviços).
#include <Arduino.h>

#include "secrets.h" // TELNET_PORT, ADMIN_PASSWORD
#include "wifi_manager.h"
#include "firebase_client.h"
#include "senhas_store.h"
#include "telnet_log.h"
#include "teclado.h"
#include "display.h"
#include "fechadura.h"
#include "menu_status.h"

// Buffer do PIN (apenas dígitos; valida SOMENTE com '#').
static String pinDigitado = "";
static const int PIN_MAX_LENGTH = 16;

// ================= SETUP =================
void setup()
{
    Serial.begin(115200);
    delay(500);
    tlogln("\n=== Iniciando Fechadura ===");

    senhas_init();    // chaves locais (NVS -> RAM; offline OK)
    fechadura_init(); // relé começa FECHADO
    teclado_init();   // linhas saída, colunas pull-up
    display_init();   // LCD com auto-detecção (não trava sem display)

    mostrarNoLCD("Sistema Iniciado", "Bem-vindo!", "", "Versao 5.0");
    delay(800);

    WiFi.onEvent(eventoWiFi);
    conectarWiFi();

    if (wifiConectado)
    {
        iniciarOTA();
        telnet_setup(); // log remoto: telnet <IP> 23
        iniciarFirebase();
        mostrarNoLCD("WiFi OK", "Sistema Online", "", "Aguardando comando");
    }
    else
    {
        mostrarNoLCD("WiFi OFF", "Modo offline", "", "Chaves em cache OK");
    }

    telaAguardando();

    tlogln("=== Sistema pronto (fechadura) ===");
    tlog("Telnet: telnet ");
    tlog_raw(WiFi.localIP().toString());
    tlogf(" %d\n", TELNET_PORT);
}

// ================= LOOP =================
void loop()
{
    // Serviços (não-bloqueantes)
    processarOTA();
    processarFirebase();
    telnet_loop();
    fechadura_loop(); // auto-fechamento por timeout

    // Mudança na tabela -> atualiza espera (fora de porta aberta/status)
    static unsigned long ultimoCheck = 0;
    if (millis() - ultimoCheck > 2000)
    {
        ultimoCheck = millis();
        if (senhas_consumirMudanca() && !fechadura_aberta() && !emModoStatus())
            telaAguardando();
    }

    // Retry Wi-Fi a cada 60s offline (fora de porta aberta/status).
    // Se voltar, o display sai do alerta sozinho na próxima mudança.
    static unsigned long ultimoRetry = 0;
    if (!wifiConectado && !fechadura_aberta() && !emModoStatus() &&
        (millis() - ultimoRetry > 60000))
    {
        ultimoRetry = millis();
        wifi_reconectar();
        telaAguardando();
    }

    // Teclado pausado com a porta aberta
    if (fechadura_aberta())
        return;

    byte idxR = 0, idxC = 0;
    char tecla = teclado_ler(&idxR, &idxC);
    if (!tecla)
        return;

    // Log sem segredo: só posição física + tamanho do buffer.
    tlogf("Tecla @R%dC%d (%d dig.)\n", idxR, idxC, pinDigitado.length());

    // Modo status consome a navegação
    if (emModoStatus())
    {
        processarTeclaStatus(tecla);
        return;
    }

    if (tecla == '*')
    {
        if (pinDigitado == CODIGO_RESET) // 0000 + '*' = reset de fábrica
        {
            pinDigitado = "";
            executarResetFabrica(); // não retorna (reinicia)
            return;
        }
        if (pinDigitado == ADMIN_PASSWORD) // admin + '*' = modo status
        {
            pinDigitado = "";
            entrarStatus();
            return;
        }
        pinDigitado = "";
        display_limpar_teclado();
        tlogln("Buffer limpo.");
        return;
    }

    if (tecla == '#')
    {
        tlogf("Validando (%d dig.)...\n", pinDigitado.length());
        validarChave(pinDigitado);
        pinDigitado = "";
        return;
    }

    // Dígito: acumula e exibe SÓ máscara (PIN nunca em claro).
    if (pinDigitado.length() < (unsigned)PIN_MAX_LENGTH)
    {
        pinDigitado += tecla;
        display_set_teclado(mascarar(pinDigitado));
    }
}
