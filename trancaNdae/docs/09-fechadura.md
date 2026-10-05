# Módulo Fechadura (`fechadura.h/.cpp`)

Relé (GPIO 4, sem strapping), validação e reinício. Relé energizado =
porta **ABERTA** (módulo ativo em LOW: inverta `RELE_ABERTO/RELE_FECHADO`).

## API

| Função | Descrição |
| :--- | :--- |
| `fechadura_init()` | Relé como saída, começa **FECHADO** |
| `abrirPorta()` | Energiza, arma auto-fechamento (`TEMPO_PORTA_ABERTA_MS` = 5s) |
| `fecharPorta()` | Trava na hora (+ restaura espera fora do modo status) |
| `fechadura_aberta()` | Estado (teclado pausa com porta aberta) |
| `fechadura_loop()` | Fecha sozinha no timeout — chame todo `loop()` |
| `validarChave(chave)` | **SOMENTE com `#`**: abre se está na tabela e ativa; `ADMIN_PASSWORD` e `CODIGO_RESET` nunca abrem |
| `executarReinicio()` | **Só reboot** (chaves preservadas): trava o relé, avisa e `ESP.restart()` |

## Reinício por teclado

Digite `0000` + `*` → reinicia. Nunca cadastre `0000` como chave
(barrado no ESP e na página).
