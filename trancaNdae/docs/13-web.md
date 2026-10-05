# Página Web (`web/`)

Controle da tranca **sem login** (auth removida). Metadados das chaves ficam
no Firebase; o **PIN nunca é armazenado nem devolvido**.

## Fluxo

1. Enviar: grava metadados em `pessoas/{device}/{nome}`
   (`nome`, `criadaEm`, `validadeH`, `ativa`) + comando `PIN:nome` em
   `comandos/{device}` (um por vez — last-write-wins).
2. O ESP consome e **apaga o comando**; a página confirma observando o nó
   virar `null` (timeout 45s apaga à força).
3. A tabela lê `pessoas/` (Nome | Cadastrada em | Validade | Status) e
   **bloqueia as expiradas** ao carregar (`BLOQ`).

## Ações por linha

*Renovar* (pede novo PIN, mantém nome/data nova), *Bloquear/Liberar*,
*Excluir*. *Apagar chaves do ESP* = `LIMPAR`. Menu recolhível **Wi-Fi**:
salva secundário (`WIFI2:ssid:senha`) ou desativa (`WIFI2OFF`).

## Arquivos (MVC enxuto)

| Arquivo | Papel |
| :--- | :--- |
| `js/app.js` | Bootstrap (`ComandoController` + `ComandoView`) |
| `js/config.js` | Endpoints/paths (`comandos/`, `pessoas/`, `PIN_LEN`) |
| `js/models/ComandoModel.js` | Firebase: comandos, metadados, higiene, confirmação |
| `js/views/ComandoView.js` | Só DOM (form, tabela, status) |
| `js/controllers/ComandoController.js` | Liga View ↔ Model |
| `database.rules.json` | Rules públicas `comandos`/`pessoas` (hospede restrito!) |
| `index.html` | Tela única + `<details>` (Como funciona, Wi-Fi) |

Validações: PIN 4 dígitos (não `0000`), nome 1–24 sem `. # $ / [ ] : " \`,
SSID 1–32, senha vazia ou 8–63 (sem `:`).

## Deploy

```bash
firebase deploy --only database   # Rules
firebase deploy --only hosting    # página (+ Ctrl+Shift+R no navegador)
```
