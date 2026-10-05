// fechadura.cpp — implementação do relé, validação e reset.
#include "fechadura.h"
#include "display.h"
#include "senhas_store.h"
#include "menu_status.h"
#include "pins.h"
#include "secrets.h"
#include "telnet_log.h"

static bool portaAberta = false;
static unsigned long momentoAbertura = 0;

void fechadura_init()
{
    pinMode(PINO_RELE, OUTPUT);
    digitalWrite(PINO_RELE, RELE_FECHADO);
}

void abrirPorta()
{
    digitalWrite(PINO_RELE, RELE_ABERTO);
    portaAberta = true;
    momentoAbertura = millis();
    mostrarNoLCD("PORTA ABERTA", "Empurre a porta", "", "Fechando sozinha...");
    registrarEvento("Porta ABERTA pelo teclado.");
}

void fecharPorta()
{
    digitalWrite(PINO_RELE, RELE_FECHADO);
    portaAberta = false;
    registrarEvento("Porta FECHADA (timeout).");
    if (!emModoStatus())
        telaAguardando();
}

bool fechadura_aberta()
{
    return portaAberta;
}

void fechadura_loop()
{
    if (portaAberta && (millis() - momentoAbertura >= TEMPO_PORTA_ABERTA_MS))
        fecharPorta();
}

void validarChave(const String &chave)
{
    String codigo = chave;
    codigo.trim();

    // O código de reinício e a senha de admin NUNCA abrem a porta.
    if (codigo == ADMIN_PASSWORD || codigo == CODIGO_RESET)
    {
        mostrarNoLCD("!! INVALIDO !!", "Uso restrito", "", "Tente novamente");
        delay(1500);
        telaAguardando();
        return;
    }

    if (senhas_contem(codigo))
    {
        abrirPorta();
        return;
    }

    if (senhas_total() == 0)
        mostrarNoLCD("Sem chaves", "Nenhuma cadastrada", "", "Envie pela pagina");
    else
        mostrarNoLCD("!! INVALIDO !!", "Chave incorreta", "", "Tente novamente");
    delay(1500);
    telaAguardando();
}

void executarReinicio()
{
    // Garante a tranca FECHADA antes de reiniciar (não deixa destrancada).
    digitalWrite(PINO_RELE, RELE_FECHADO);
    portaAberta = false;
    mostrarNoLCD("Reiniciando...", "Aguarde!", "", "");
    tlogln("Reinicio via teclado (0000). Chaves preservadas.");
    delay(1200);
    ESP.restart();
}
