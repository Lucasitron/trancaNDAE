# Modo Status (`menu_status.h/.cpp`)

Diagnóstico local no display. Entrada: **senha de admin** (`ADMIN_PASSWORD`,
em `secrets.h`) + `*`. A senha de admin nunca abre a porta.

## Navegação

| Tecla | Ação |
| :--- | :--- |
| `2` / `8` | rola linhas da tela |
| `4` / `6` | tela anterior / próxima |
| `*` | sai (volta à espera) |

Demais teclas são ignoradas.

## Telas

1. **Wi-Fi**: rede, IP, RSSI.
2. **Firebase**: rede conectada/offline, auth OK/pendente.
3. **Chaves**: `Ativas: N/20`, abertura só com `#`, último evento.
4. **Sistema** (rolável, 6 linhas): uptime, heap, relé, lembretes
   (`Reinicia: 0000+*`, `Status: admin + *`).

Só contadores/estado — **PINs nunca aparecem**.

## API

| Função | Descrição |
| :--- | :--- |
| `registrarEvento(ev)` | Último evento (log + tela Chaves) |
| `emModoStatus()` | Teclado normal pausado quando `true` |
| `entrarStatus()` / `sairStatus()` | Liga/desliga |
| `processarTeclaStatus(tecla)` | Navegação (sempre consome) |
