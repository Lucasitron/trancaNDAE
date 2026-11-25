# Memória e robustez

Auditoria do firmware com foco em estouro/vazamento e em custo de heap.
Resumo: **não havia estouro de memória**; o "display apaga ao abrir" era o
controlador do LCD perdendo o estado no acionamento do relé (ver
`docs/08-display.md`). Ainda assim, o caminho quente (tecla → validação →
abertura → display) foi reescrito para não alocar.

## Por que não era estouro de memória

- O ESP continua rodando depois da abertura: o uptime visto no modo
  status continua crescendo e a tranca volta a responder normalmente.
- Um estouro real reiniciaria/`panic`aria o ESP (não apenas apagaria o
  LCD). Nesse caso o display voltaria ao normal sozinho após o reboot.
- O único pico de corrente coincide com `digitalWrite(PINO_RELE, ...)`.

Se um dia houver suspeita de heap, meça antes de mudar código:
modo status → **Sistema** (heap livre) ou telnet (`status`). Queda
contínua entre leituras indica vazamento; um valor baixo e estável não.

## O que foi feito

| Área | Antes | Agora |
| :--- | :--- | :--- |
| Display | 4 `String` + `limparLinha`/`centralizar` por tecla; `clear()` no LCD | 4 buffers `char[21]` fixos; escreve 20 colunas de uma vez e **só as linhas que mudaram** |
| Máscara do PIN | `mascarar()` criava uma `String` a cada dígito | `display_set_mascara(n)` escreve `n` asteriscos direto no buffer |
| Buffer do PIN | `String` crescendo sob demanda | `pinDigitado.reserve(PIN_MAX_LENGTH)` no `setup()` |
| Validação | `String codigo = chave; codigo.trim();` | usa o próprio `const String &` (o teclado só acumula dígitos) |
| Comando Firebase | `valor.startsWith(String(ADMIN_PASSWORD) + ":")` | `memcmp` + teste do `:` (zero alocação) |
| Split `CMD:a:b` | loop com 2 `String` descartáveis extras | helper único `dividirCampos()` |
| Nome duplicado | `indice_nome(String(nomeLimpo))` | overload `indice_nome(const char*)` com `strcmp` |
| Sanitizar nome | 11 comparações de caractere | `strchr()` em um conjunto de proibidos |
| Log Telnet | `tlog_raw(const char*)` criava `String`; buffer de comando era `String` | escrita direta em `Serial`/`WiFiClient`; buffer fixo `char[65]` |
| Telnet | `telnet_only*`, checagem de conexão duplicada | removidos (código morto) |
| Menus | `display_mostrar4` (duplicata) + linhas montadas 2× | `mostrarNoLCD` + contagem de linhas sem remontar |
| Tabela de chaves | 20 chaves, PIN em ASCII (5 B) e 3 entradas NVS por chave | 30 chaves, PIN `uint16` (2 B) e **1 blob NVS** (840 B); formato antigo migrado no boot |

Nada disso mudou a leitura do teclado (`src/teclado.cpp` continua igual).

## Pegada estática

| Item | Bytes |
| :--- | ---: |
| Tabela de chaves (30 × 28: PIN `uint16` + nome + flag) | 840 |
| Display (atual + último escrito) | 168 |
| Buffer do comando telnet | 65 |
| Buffer do PIN (`String` reservada) | ~32 |

Não há `new`/`malloc`/VLA no código do projeto. `String` sobrevive apenas
nas bordas (NVS e stream do Firebase), onde o custo é esporádico.

Consumo no build `esp32dev`: **RAM global 18,4%** (~60 KB de 320 KB) e
**flash 91,2%** (~1,19 MB de 1,25 MB). O heap em execução é dominado pelos
3 clientes TLS do Firebase (auth + stream + manutenção), mantidos de
propósito — reduzi-los exigiria reabrir a conexão a cada comando.

> **Atenção:** com 91% da partição de app ocupada, a próxima feature pode
> não caber. Se precisar de folga, troque `board_build.partitions` para
> `huge_app.csv` (sem OTA) ou remova o modo simulação do build.

## Regras para não regredir

1. Nada de `String` concatenada dentro de `loop()`/callbacks.
2. Buffers de texto com tamanho fixo + `snprintf`.
3. `reserve()` em qualquer `String` que cresça com entrada do usuário.
4. Ao acionar carga indutiva (relé), assuma que o LCD pode reiniciar:
   `display_loop()` já cobre isso — não remova a reescrita periódica.
