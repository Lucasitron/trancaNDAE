# Firebase Client — comandos voláteis (nada persiste na nuvem)

Integração com **Firebase Realtime Database** em modo assíncrono. As senhas
vivem **somente no ESP** (RAM+NVS via `senhas_store`); o RTDB transporta
apenas comandos que o ESP **consome e apaga**.

## API

### `void iniciarFirebase()`
Autenticação (`UserAuth`), app e preparo do stream. O stream inicia em
`processarFirebase()` quando `app.ready()`.

### `void processarFirebase()`
Mantém o app assíncrono rodando + inicia o stream UMA vez após auth.
Chamada **não-bloqueante** no `loop()`.

### `bool firebase_pronto()`
`true` quando autenticado (usado pelo modo status do display).

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

- `"4829"` → cadastra o PIN na tabela local (máx. 20).
- `"LIMPAR"` → zera a tabela local.
- `null`/ausente → estado normal (nó já consumido/apagado).

## Dependências

- `FirebaseClient` (mobizt), `WiFiClientSecure.h`, `secrets.h`

## Segurança

- TLS (`setInsecure()` p/ testes; certificado real em produção)
- Auth e-mail/senha com renovação (3000s); Rules exigem `auth != null`
- PINs nunca trafegam além do comando volátil (apagado após leitura)
