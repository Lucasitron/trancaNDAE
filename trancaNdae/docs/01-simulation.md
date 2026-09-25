# 🧪 Módulo Simulation

Encapsula toda a lógica de simulação do **Wokwi**, isolando-a do código de produção.

## Como Funciona

- Se `WOKWI_SIM` estiver definido no `build_flags`, o corpo das funções é compilado.
- Caso contrário, as funções são **no-op** (vazias) e não ocupam espaço no binário final.
- O `main.ino` chama `simulation_setup()` e `simulation_loop()` incondicionalmente.

## API

### `void simulation_setup()`
Inicializa o botão TEST (`INPUT_PULLUP`) e o LED de status. Chamada no `setup()` principal.

### `void simulation_loop()`
Processa:
1. **Botão TEST** → gera token HMAC de `"ABRIR"` com chave `"12345678"` e salva na NVS.
2. **LED de status** → espelha o estado do relé.

## Pinos (definidos em `pins.h`)

| Pino | GPIO | Função |
| :--- | :--- | :--- |
| `PINO_BOTAO_TESTE` | 5 | Botão que simula o recebimento do token |
| `PINO_LED_STATUS` | 2 | LED que espelha o relé |

## Dependências Externas

O módulo usa `extern` para referenciar funções do `main.ino`:

```cpp
extern String hmacSha256(const String &mensagem, const String &chave);
extern void mostrarNoLCD(const String &status, const String &mensagem,
                         const String &teclado, const String &rodape);
```

> ⚠️ **Importante:** essas funções **não podem ser `static`** no `main.ino`.

## Exemplo de Uso

```cpp
#include "simulation.h"

void setup() {
    simulation_setup();  // no-op em produção
}

void loop() {
    simulation_loop();   // no-op em produção
}
```

## Chave de Teste

| Parâmetro | Valor |
| :--- | :--- |
| Palavra-comando | `ABRIR` |
| PIN de teste | `12345678` |
| Token gerado | `HMAC-SHA256("ABRIR", "12345678")` |

Para alterar, edite a linha em `simulation.cpp`:

```cpp
String tokenTeste = hmacSha256("ABRIR", "12345678");
```