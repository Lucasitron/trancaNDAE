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

O PC deve publicar uma **string** no nó `/comandos/dispositivo1`:

```json
{
  "comandos": {
    "dispositivo1": "a3f5b8c9...e7d2"
  }
}
```

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