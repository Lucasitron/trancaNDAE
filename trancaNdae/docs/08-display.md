# Módulo Display (`display.h/.cpp`)

LCD 20x4 I2C (PCF8574, endereço `LCD_I2C_ADDRESS`) em layout de 4 zonas:
status | mensagem | teclado (máscara) | rodapé.

## Robustez I2C

- `display_init()`: `Wire.begin()` + **scan do barramento**; só ativa
  (`display_ok()==true`) se o endereço responder. Sem display, o sistema
  segue (conteúdo estrutural, sem segredos, espelhado no log `[LCD]`).
- `atualizarDisplay()` escreve **só quando algo muda** e `centralizar()`
  sempre completa 20 colunas (sem restos da linha anterior).

## API

| Função | Descrição |
| :--- | :--- |
| `display_init()` / `display_ok()` | Auto-detecção no boot |
| `mostrarNoLCD(status, msg, teclado="", rodape="")` | 4 zonas |
| `display_mostrar4(l0..l3)` | 4 linhas cruas (menus) |
| `telaAguardando()` | `Bem vindo!` (ou `WiFi OFF!` offline) + dica; sem contadores |
| `mascarar(s)` | `"1234"` → `"****"` — PIN nunca em claro |
| `display_set_teclado()` / `display_limpar_teclado()` | Zona de digitação |

Sem acentos em nenhuma string (o HD44747 não tem `ã/ç`).
