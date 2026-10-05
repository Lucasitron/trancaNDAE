// Model (config): apenas dados de conexão. Sem DOM, sem regra de negócio.
// apiKey de projeto web é pública por design (proteção real está nas
// Rules + Auth + App Check). Nenhuma senha é guardada aqui nem no RTDB:
// a página só envia comandos voláteis que o ESP consome e apaga.
export const firebaseConfig = {
  apiKey: "AIzaSyByBylcrbdPD6el0C2UbBeAdFvtFzWhFr4",
  authDomain: "trancandae-cd59b.firebaseapp.com",
  databaseURL: "https://trancandae-cd59b-default-rtdb.firebaseio.com",
  projectId: "trancandae-cd59b",
};

// PIN de 4 dígitos (igual a include/senhas_store.h: PIN_LEN).
export const PIN_LEN = 4;

export const DEVICE_DEFAULT = "dispositivo1";

// Nó de comandos VOLÁTEIS (a web escreve, o ESP consome e apaga):
//   comandos/{device} = "PIN:nome[:epoch]" | "RENOVAR:o:n[:e]"
//                     | "DEL:pin" | "LIMPAR"
// Nó de leitura (ESP -> web, SEM pin — só nome+data):
//   resumo/{device} = {"total":N,"chaves":[{"nome":"..","criadaEm":E}]}
export const caminhoComando = (device) => `comandos/${device}`;
export const caminhoResumo = (device) => `resumo/${device}`;
export const COMANDO_LIMPAR = "LIMPAR";
