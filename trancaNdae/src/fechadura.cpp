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

    // A senha de admin NUNCA abre a porta.
    if (codigo == ADMIN_PASSWORD)
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

void executarResetFabrica()
{
    mostrarNoLCD("RESET FABRICA", "Apagando chaves...", "", "Reiniciando...");
    senhas_limpar();
    tlogln("RESET de fabrica via teclado: chaves apagadas. Reiniciando...");
    delay(1500);
    ESP.restart();
}
