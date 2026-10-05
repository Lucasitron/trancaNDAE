# Operação da fechadura

## Teclado

- Digite o PIN (4 dígitos, máscara `*` no display) e pressione **`#`**.
  A abertura acontece **somente** com `#` (sem autovalidação).
- `*` com buffer vazio apenas limpa. `*` após a **senha de admin**
  (ver `ADMIN_PASSWORD` em `include/secrets.h`) entra no modo status.

## Modo status (display)

Entrada: `<senha-admin>` + `*`. Navegação:

| Tecla | Ação |
| :--- | :--- |
| `2` / `8` | rola linhas da tela |
| `4` / `6` | tela anterior / próxima |
| `*` | sai do modo (volta à espera) |

Telas: **Wi-Fi** (rede, IP, RSSI), **Firebase** (rede/auth/stream),
**Chaves** (só contadores — PINs nunca aparecem), **Sistema**
(uptime, heap, estado do relé). A senha de admin **nunca abre a porta**.

## Cadastrar chaves (página web, sem login)

A página usa auth anônima automática (vale nas Rules `auth != null`).
Sem login, quem abre a página pode enviar comandos — mantenha-a em
rede/hospedagem restrita.

1. Dispositivo + **nome** (etiqueta única) + PIN de 4 dígitos → **Enviar**.
3. Um comando por vez: aguarde o display confirmar antes do próximo
   (last-write-wins no nó `comandos`).
4. A tabela **Carrega do ESP** sozinha (resumo: nomes + datas, nunca o PIN).
   *Renovar* troca o PIN mantendo o nome; *Excluir* remove uma;
   **Apagar chaves do ESP** zera tudo (`LIMPAR`).

Nada de senha fica no database: o ESP apaga o comando após consumir.
Sem Wi-Fi, a tranca opera com as chaves em cache (NVS).

## Telnet e OTA

- Logs remotos: `telnet <IP> 23` (`status`, `reboot`, `help`). O IP aparece
  no LCD/Serial no boot (muda conforme a rede).
- Upload sem cabo: `upload_protocol = espota` no `platformio.ini`
  (primeiras 2 gravações via USB). Senha em `OTA_PASSWORD`.

## Rede (IP fixo multi-perfil)

O ESP tenta nesta ordem: fixo `10.0.0.150` (gateway `.1`), fixo
`192.168.1.150` (gateway `.1`), e DHCP como reserva. Perfis em
`include/secrets.h` (`STATIC_IP/GATEWAY`, `STATIC_IP2/GATEWAY2`,
`IP_FALLBACK_DHCP`). Use IPs fora do range DHCP do roteador.

## Credenciais (`include/secrets.h`, gitignored)

Copie `include/secrets.example.h` → `include/secrets.h` e preencha:
Wi-Fi, Firebase (API key, login, URL), `ADMIN_PASSWORD`, OTA, Telnet.
