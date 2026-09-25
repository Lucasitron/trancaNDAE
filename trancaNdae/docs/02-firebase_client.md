# 🔥 Módulo Firebase Client

Integração com **Firebase Realtime Database** em modo assíncrono, com persistência de token em **NVS**.

## API

### `void iniciarFirebase()`
Inicializa a autenticação, o app e o streaming no nó `/comandos/dispositivo1`. Recupera token pendente da NVS no boot.

```cpp
iniciarFirebase();
```

### `void processarFirebase()`
Mantém o app assíncrono rodando. Chamada **não-bloqueante** no `loop()`.

```cpp
void loop() {
    processarFirebase();
}
```

### `void salvar_token_nvs(const String &token)`
Persiste o token HMAC recebido do Firebase na NVS.

### `String ler_token_nvs()`
Recupera o token salvo. Retorna `""` se vazio.

### `void limpar_token_nvs()`
Remove o token da NVS. **Chame após validação bem-sucedida** (anti-replay).

## Callback

### `void processData(AsyncResult &aResult)`
Callback unificado para eventos, erros, debug e dados. Quando uma **string** chega no nó monitorado, chama `salvar_token_nvs()`.

## Formato Esperado no Firebase

### Comando único legado

O PC deve publicar uma **string** no nó `/comandos/dispositivo1`:

```json
{
  "comandos": {
    "dispositivo1": "a3f5b8c9...e7d2"
  }
}
```

### Gerenciador de senhas (Casa do Estudante)

A página `web/` mantém duas chaves por dispositivo. O ESP32 lê **somente**
`lista` (uma string CSV, sem JSON — a lib não tem parser e a RAM é limitada):

```json
{
  "senhas": {
    "dispositivo1": {
      "lista": "tok1hex32,tok2hex32",
      "itens": {
        "-Oa1b2c": { "nome": "Maria Q12", "token": "tok1hex32", "ativa": true, "expiraEm": 0, "criadaEm": 1758760000000 }
      }
    }
  }
}
```

- `lista`: só tokens de senhas **ativas e não expiradas** (máx. 20, ver
  `include/senhas_store.h: MAX_SENHAS`). Bloquear/excluir/expirar = sumir da
  lista = slot liberado no ESP32. Não há blocklist que cresce.
- `itens`: metadados só para a web (o ESP32 ignora).
- Lista vazia/nula = zero chaves (fail-closed). Expiração é aplicada pela web
  ao recompor `lista`; sem NTP no ESP32, um token expirado ainda vale offline
  até a próxima sincronização.

## Dependências

- `FirebaseClient` (mobizt)
- `Preferences.h` (core ESP32)
- `WiFiClientSecure.h` (core ESP32)
- `secrets.h` (credenciais Firebase)

## Segurança

- ✅ TLS via `WiFiClientSecure` (`setInsecure()` para testes)
- ✅ Autenticação por e-mail/senha (`UserAuth`)
- ✅ Renovação automática de token (3000s)
- ⚠️ Em produção, use certificado raiz real em vez de `setInsecure()`

## Exemplo de Uso

```cpp
#include "firebase_client.h"

void setup() {
    iniciarFirebase();
}

void loop() {
    processarFirebase();
}
```