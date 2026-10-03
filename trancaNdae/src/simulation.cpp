// simulation.cpp
#include "simulation.h"
#include "pins.h"
#include "firebase_client.h"

#ifdef WOKWI_SIM

// ================= DEPENDÊNCIAS EXTERNAS (definidas no main.ino) =================
// Declaradas como extern para evitar duplicação
extern void mostrarNoLCD(const String &status, const String &mensagem,
                         const String &teclado, const String &rodape);

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
    // --- 1. Botão TEST grava código plain de teste e salva na NVS ---
    bool estadoAtual = digitalRead(PINO_BOTAO_TESTE);

    if (ultimoEstadoBotao == HIGH && estadoAtual == LOW) // borda de descida
    {
        // Código de teste "1234" (sem cripto, igual ao fluxo da web)
        salvar_token_nvs("1234");

        Serial.println("[WOKWI] Código de teste gravado: 1234");
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