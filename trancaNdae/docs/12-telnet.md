# Log Remoto (`telnet_log.h/.cpp`)

Servidor TCP na porta `TELNET_PORT` (1 cliente por vez) + escrita dupla:
toda função `tlog*` escreve no **Serial e no Telnet** (sem cliente, custo
zero de rede). Conecte: `telnet <IP> 23` (mesma rede do ESP).

## API

| Função | Descrição |
| :--- | :--- |
| `telnet_setup()` / `telnet_loop()` | Sobe o servidor; chame `loop()` todo ciclo (não bloqueia) |
| `tlog_raw()` / `tlog()` / `tlogln()` / `tlogf()` | Espelho Serial+Telnet |
| `telnet_conectado()` | Há cliente agora? |

## Comandos (digite no telnet)

| Comando | Ação |
| :--- | :--- |
| `help` | Lista comandos |
| `status` | IP, RSSI, uptime, heap |
| `reboot` | Reinicia o ESP |

Regra de segurança: **nunca logar PIN** — só posição física (`@R/C`),
tamanhos e contadores.
