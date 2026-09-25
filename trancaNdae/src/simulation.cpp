// simulation.cpp
#include "simulation.h"
#include "pins.h"
#include "firebase_client.h"

#ifdef WOKWI_SIM

// ================= DEPENDÊNCIAS EXTERNAS (definidas no main.ino) =================
// Declaradas como extern para evitar duplicação
extern String hmacSha256(const String &mensagem, const String &chave);
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
    // --- 1. Botão TEST gera token HMAC e salva na NVS ---
    bool estadoAtual = digitalRead(PINO_BOTAO_TESTE);

    if (ultimoEstadoBotao == HIGH && estadoAtual == LOW) // borda de descida
    {
        // Gera o HMAC de "ABRIR" com a chave de teste "1234" (PIN_LEN dígitos)
        String tokenTeste = hmacSha256("ABRIR", "1234");
        salvar_token_nvs(tokenTeste);

        Serial.println("[WOKWI] Token de teste gravado: " + tokenTeste);
        mostrarNoLCD("Comando Pendente", "Digite a chave", "", "Pressione # para OK");
        tone(PINO_BUZZER, 1500, 150);
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