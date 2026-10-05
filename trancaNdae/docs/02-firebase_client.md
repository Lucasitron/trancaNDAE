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
dele** (reentrância = reboot): só RAM/NVS aqui. `Database.remove` e os `set`
rodam no `processarFirebase()`, via flags. Para strings no stream:
1. marca `apagarComandoPendente` e interpreta: `"LIMPAR"` → `senhas_limpar()`;
   senha de admin → recusado; `"PIN:nome[:epoch[:validadeH]]"` → adicionar;
   `"RENOVAR:nome:novo[:epoch]"`, `"BLOQ:nome"`/`"LIB:nome"`, `"DEL:nome"`;
2. mutação arma `resumoPendente`; o loop publica e apaga o comando.

## Formato no Firebase

Comando volátil (web escreve, ESP apaga no loop seguinte), string:

```json
{ "comandos": { "dispositivo1": "4829:Maria:1758760000:12" } }
```

Resumo de leitura (ESP escreve, web lê — nunca contém o PIN):

```json
{ "resumo": { "dispositivo1": {
  "total": 1,
  "chaves": [{ "nome": "Maria", "criadaEm": 1758760000,
                "validadeH": 12, "ativa": 1 }]
} } }
```

Expiração: a web bloqueia (`BLOQ`) as vencidas ao carregar a lista
(o ESP não tem relógio); `validadeH: 0` = sem expiração.

- `"4829"` → cadastra o PIN na tabela local (máx. 20).
- `"LIMPAR"` → zera a tabela local.
- `null`/ausente → estado normal (nó já consumido/apagado).

## Dependências

- `FirebaseClient` (mobizt), `WiFiClientSecure.h`, `secrets.h`

## Segurança

- TLS (`setInsecure()` p/ testes; certificado real em produção)
- Auth e-mail/senha com renovação (3000s); Rules exigem `auth != null`
- PINs nunca trafegam além do comando volátil (apagado após leitura)
