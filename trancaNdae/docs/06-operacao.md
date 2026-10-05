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

A página não usa login; os nós `comandos`/`pessoas` estão com Rules
públicas (`true`) para funcionar sem configuração. **Risco:** quem tiver
a URL controla a tranca — hospede a página em local restrito ou volte o
login + Rules por UID/`auth != null`.

1. Dispositivo + **nome** (etiqueta única) + PIN de 4 dígitos + validade.
2. Ao enviar, a página grava os **metadados no Firebase**
   (`pessoas/{device}/{nome}`: nome, data, validade, status) e o comando
   `PIN:nome` em `comandos/{device}`. O ESP consome e **apaga o comando**
   (confirmado quando o nó vira `null`, ou após 45s) — o PIN nunca fica
   armazenado.
3. A tabela lê os metadados do Firebase e **bloqueia as expiradas** ao
   carregar. *Renovar* troca o PIN e atualiza a data; *Bloquear/Liberar*
   alterna o status; *Excluir* remove; **Apagar chaves** zera tudo.

Nada de senha fica no database: o ESP apaga o comando após consumir.
Sem Wi-Fi, a tranca opera com as chaves em cache (NVS).

## Telnet e OTA

- Logs remotos: `telnet <IP> 23` (`status`, `reboot`, `help`). O IP aparece
  no LCD/Serial no boot (muda conforme a rede).
- Upload sem cabo: `upload_protocol = espota` no `platformio.ini`
  (primeiras 2 gravações via USB). Senha em `OTA_PASSWORD`.

## Rede (padrão + secundário)

- O ESP usa o **Wi-Fi padrão** gravado nele (`secrets.h`).
- No menu recolhível **Wi-Fi do ESP** da página dá para salvar um
  **secundário** (SSID + senha): se o padrão falhar no boot, ele tenta o
  secundário sozinho; desativando, volta a usar só o padrão.
- IP: tenta fixo por sub-rede (`10.0.0.x` ou `192.168.1.x`) com os
  gateway/DNS do DHCP; fora disso mantém DHCP. Offline, tenta de novo a
  cada 60s.
- **Sem nenhuma rede ativa o display mostra `WiFi OFF!`** no lugar do
  `Bem vindo!` (a fechadura segue operando com as chaves em cache).

## Credenciais (`include/secrets.h`, gitignored)

Copie `include/secrets.example.h` → `include/secrets.h` e preencha:
Wi-Fi, Firebase (API key, login, URL), `ADMIN_PASSWORD`, OTA, Telnet.
