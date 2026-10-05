// Model (config): apenas dados de conexão. Sem DOM, sem regra de negócio.
// apiKey de projeto web é pública por design (proteção real está nas Rules).
export const firebaseConfig = {
  apiKey: "AIzaSyByBylcrbdPD6el0C2UbBeAdFvtFzWhFr4",
  authDomain: "trancandae-cd59b.firebaseapp.com",
  databaseURL: "https://trancandae-cd59b-default-rtdb.firebaseio.com",
  projectId: "trancandae-cd59b",
};

// PIN de 4 dígitos (igual a include/senhas_store.h: PIN_LEN).
export const PIN_LEN = 4;

export const DEVICE_DEFAULT = "dispositivo1";

// Comando volátil para o ESP (ele consome e apaga o nó):
//   comandos/{device} = "PIN:nome" | "RENOVAR:nome:novo"
//                     | "BLOQ:nome" | "LIB:nome" | "DEL:nome" | "LIMPAR"
export const caminhoComando = (device) => `comandos/${device}`;
export const COMANDO_LIMPAR = "LIMPAR";

// Metadados das chaves (nunca o PIN), geridos pela página:
//   pessoas/{device}/{nome} = { nome, criadaEm, validadeH, ativa }
export const caminhoPessoas = (device) => `pessoas/${device}`;
