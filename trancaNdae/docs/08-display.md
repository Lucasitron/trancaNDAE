# Módulo Display (`display.h/.cpp`)

LCD 20x4 I2C (PCF8574, endereço `LCD_I2C_ADDRESS`) em layout de 4 zonas:
status | mensagem | teclado (máscara) | rodapé.

## Robustez I2C (tela em branco)

O acionamento do relé pode derrubar/reiniciar o controlador do LCD
(pico de consumo no 5V, EMI) **sem** derrubar o ESP. O PCF8574 continua
respondendo no barramento, mas o HD44780 fica apagado — e como o driver
só escrevia quando o texto mudava, o painel ficava em branco até reiniciar.

Correção em duas camadas:

- `atualizarDisplay()` compara linha a linha e escreve **só o que mudou**
  (reduz o tráfego I2C a cada tecla).
- `display_loop()` (chamado todo `loop()`) reafirma **display ON +
  backlight** e reescreve **as 4 linhas a cada `DISPLAY_REFRESH_MS` (1 s)**.
  Um comando I2C corrompido no pico do relé pode ter desligado o painel
  (`0x08`) ou apagado o backlight; isso restaura sem reiniciar o ESP.
  Antes, verifica o ACK do endereço: se o controlador não responder, chama
  `lcd.init()` e reconfigura tudo. A tela volta sozinha em até 1 s.

`display_init()` faz `Wire.begin()` + **scan do barramento**; só ativa
(`display_ok()==true`) se o endereço responder. Sem display, o sistema
segue (conteúdo estrutural, sem segredos, espelhado no log `[LCD]`).

## Sem heap no caminho de render

As 4 zonas ficam em buffers fixos `char[LCD_ROWS][LCD_COLUMNS + 1]`
(RAM estática, ~170 bytes) — nenhuma `String` temporária por tecla.
`escreverLinha()` monta as 20 colunas em um buffer de pilha já preenchido
com espaços, o que apaga restos da linha anterior sem `clear()` (que
pisca a tela e custa mais I2C).

## API

| Função | Descrição |
| :--- | :--- |
| `display_init()` / `display_ok()` | Auto-detecção no boot |
| `display_loop()` | Vigia/reescreve o LCD a cada 1 s (recupera tela em branco) |
| `mostrarNoLCD(status, msg, teclado="", rodape="")` | 4 zonas (trunca em 20 colunas) |
| `telaAguardando()` | `Bem vindo!` (ou `WiFi OFF!` offline) + dica; sem contadores |
| `display_set_mascara(n)` | N asteriscos na zona do teclado — PIN nunca em claro |
| `display_limpar_teclado()` | Limpa a zona de digitação |

Sem acentos em nenhuma string (o HD44780 não tem `ã/ç`).
