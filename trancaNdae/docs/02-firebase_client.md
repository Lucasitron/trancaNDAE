# Firebase Client — comandos voláteis (nada persiste na nuvem)

Integração com **Firebase Realtime Database** em modo assíncrono. As senhas
vivem **somente no ESP** (RAM+NVS via `senhas_store`); o RTDB transporta
apenas comandos que o ESP **consome e apaga**.

## API

### `void iniciarFirebase()`
App e preparo do stream (idempotente). Por padrão usa **`NoAuth`** (as Rules
são públicas), então `app.ready()` fica `true` de imediato e o stream não
depende de login. O stream inicia em `processarFirebase()` quando
`app.ready()`. Para Rules com `auth != null`, defina `FIREBASE_USE_USER_AUTH`
em `firebase_client.cpp` e preencha `USER_EMAIL`/`USER_PASSWORD`.

> **Importante:** `iniciarFirebase()` só sobe com Wi-Fi. O `main.cpp` chama os
> serviços online (OTA/telnet/Firebase) quando `wifiConectado` fica `true`,
> seja no boot ou numa reconexão posterior.

### `void processarFirebase()`
Mantém o app assíncrono rodando + inicia o stream UMA vez após auth.
Chamada **não-bloqueante** no `loop()`.

### `bool firebase_pronto()`
`true` quando o app foi iniciado e está pronto (`NoAuth` → imediato). Usado
pelo modo status do display.

### `void processData(AsyncResult &aResult)`
Callback unificado (eventos/debug/erros/dados). **Nunca** chama `app.loop()`
dentro (recursão estoura a `loopTask`) e **nunca inicia tarefa async dentro
dele** (reentrância = reboot): só RAM/NVS aqui. `Database.remove` roda no
`processarFirebase()`, via flag. Para strings no stream:
- `"LIMPAR"` → `senhas_limpar()`; senha de admin → recusado;
- `"PIN:nome"` → adicionar; `"RENOVAR:nome:novo"`, `"BLOQ:nome"`/`"LIB:nome"`,
  `"DEL:nome"`, `"WIFI2:ssid:senha"` (salva secundário e reconecta),
  `"WIFI2OFF"` (só padrão).
O PIN é gravado só na RAM/NVS local e o nó é apagado no loop seguinte.

## Formato no Firebase

Comando volátil (web escreve, ESP apaga), string:

```json
{ "comandos": { "dispositivo1": "4829:Maria" } }
```

Metadados (gravados e lidos pela PÁGINA, nunca o PIN):

```json
{ "pessoas": { "dispositivo1": {
  "Maria": { "nome": "Maria", "criadaEm": 1758760000,
             "validadeH": 12, "ativa": true }
} } }
```

O ESP **não** publica nem lê metadados: só recebe comandos e guarda os PINs.
A expiração é aplicada pela página (bloqueia via `BLOQ` ao carregar).

- `"4829"` → cadastra o PIN na tabela local (máx. 30).
- `"LIMPAR"` → zera a tabela local.
- `null`/ausente → estado normal (nó já consumido/apagado).

## Dependências

- `FirebaseClient` (mobizt), `WiFiClientSecure.h`, `secrets.h`

## Segurança

- TLS (`setInsecure()` p/ testes; certificado real em produção)
- **Sem login por padrão (`NoAuth`)**, pois `database.rules.json` é público
  (`true`) — o auth era um ponto de falha sem ganho de segurança. Ao endurecer
  as Rules (`auth != null`), ligue `FIREBASE_USE_USER_AUTH` (renovação 3000s).
- PINs nunca trafegam além do comando volátil (apagado após leitura)
- Resiliência: o stream é recriado quando `app.ready()` cai ou quando o SSE
  reporta erro; o comando é reprocessado ao voltar a conexão.
