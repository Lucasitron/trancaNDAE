# Módulo Simulation (Wokwi)

Isola a lógica de simulação do **Wokwi** do código de produção.

## Como funciona

- Com `WOKWI_SIM` definido, o corpo das funções é compilado; sem ele,
  são **no-op** (não ocupam espaço no binário).
- Usa `display.h` (`mostrarNoLCD`) e `senhas_store.h` — sem `extern`
  manual e sem HMAC/buzzer.

## API

### `void simulation_setup()`
Botão TEST (`INPUT_PULLUP`) + LED de status. Chamada no `setup()`.

### `void simulation_loop()`
1. **Botão TEST** (borda de descida) → `senhas_adicionar("1234", "Teste Wokwi")`
   e avisa no display.
2. **LED de status** → espelha o estado do relé.

## Pinos (`pins.h`, só com `WOKWI_SIM`)

| Define | GPIO | Função |
| :--- | :--- | :--- |
| `PINO_BOTAO_TESTE` | 5 | Simula recebimento de comando/chave |
| `PINO_LED_STATUS` | 2 | Espelha o relé |
