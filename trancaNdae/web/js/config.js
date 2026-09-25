// Model (config): apenas dados de conexão. Sem DOM, sem regra de negócio.
// Preencha com os mesmos valores de include/secrets.h.
// apiKey de projeto web é pública por design (proteção real está nas Rules + Auth).
export const firebaseConfig = {
  apiKey: "AIzaSyByBylcrbdPD6el0C2UbBeAdFvtFzWhFr4",
  authDomain: "trancandae-cd59b.firebaseapp.com",
  databaseURL: "https://trancandae-cd59b-default-rtdb.firebaseio.com",
  projectId: "trancandae-cd59b",
};

// Palavra-comando esperada pelo firmware (src/main.cpp: PALAVRA_ESPERADA).
export const PALAVRA_COMANDO = "ABRIR";

// Capacidade máxima da tabela no ESP32 (include/senhas_store.h: MAX_SENHAS).
// O ESP32 ignora tokens além deste limite; a web publica no máximo isso.
export const MAX_SENHAS = 20;

// Token = HMAC-SHA256("ABRIR", PIN) truncado (4 hex = 2 bytes) e PIN de
// 4 dígitos. Espelha include/senhas_store.h (TOKEN_HEX_LEN, PIN_LEN).
export const TOKEN_HEX_LEN = 4;
export const PIN_LEN = 4;

export const DEVICE_DEFAULT = "dispositivo1";

// Layout no RTDB (o ESP32 lê SOMENTE `lista`; `itens` é só p/ a web):
//   senhas/{device}/lista        -> string CSV de tokens (32 hex)
//   senhas/{device}/itens/{id}   -> { nome, token, ativa, expiraEm, criadaEm }
export const caminhoItens = (device) => `senhas/${device}/itens`;
export const caminhoLista = (device) => `senhas/${device}/lista`;
