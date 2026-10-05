// simulation.cpp
#include "simulation.h"
#include "pins.h"
#include "senhas_store.h"
#include "display.h" // mostrarNoLCD (módulo display)

#ifdef WOKWI_SIM

// ================= ESTADO INTERNO =================
static bool ultimoEstadoBotao = HIGH;

// ================= SETUP =================
void simulation_setup()
{
    pinMode(PINO_BOTAO_TESTE, INPUT_PULLUP);
    pinMode(PINO_LED_STATUS, OUTPUT);
    digitalWrite(PINO_LED_STATUS, LOW);
    Serial.println("[WOKWI] Modo simulacao ativo");
}

// ================= LOOP =================
void simulation_loop()
{
    // --- 1. Botão TEST cadastra o PIN de teste na tabela local ---
    bool estadoAtual = digitalRead(PINO_BOTAO_TESTE);

    if (ultimoEstadoBotao == HIGH && estadoAtual == LOW) // borda de descida
    {
        senhas_adicionar("1234", "Teste Wokwi", 0);

        Serial.println("[WOKWI] PIN de teste cadastrado: 1234");
        mostrarNoLCD("Comando Pendente", "Digite a chave", "", "Pressione # para OK");
    }
    ultimoEstadoBotao = estadoAtual;

    // --- 2. LED vermelho espelha o estado do relé ---
    digitalWrite(PINO_LED_STATUS, digitalRead(PINO_RELE));
}

#else

// ================= VERSÃO VAZIA (produção) =================
// Sem a flag WOKWI_SIM, essas funções são "no-op" para não pesar no binário final
void simulation_setup() {}
void simulation_loop() {}

#endif