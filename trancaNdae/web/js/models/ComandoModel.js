// Model: comandos voláteis + leitura do resumo. Sem DOM, sem cripto.
// Escrita (web -> ESP, apagada após consumo). O NOME é a chave de gerência
// (único por tabela):
//   comandos/{device} = "PIN:nome[:epoch]" | "RENOVAR:nome:novo[:epoch]"
//                     | "DEL:nome" | "LIMPAR"
// Leitura (ESP -> web, SEM pin — só nome+data):
//   resumo/{device} = {"total":N,"chaves":[{"nome":"..","criadaEm":E}]}
// Last-write-wins no comando: envie um por vez e aguarde confirmar.
import { getApps, initializeApp } from "firebase/app";
import { getAuth } from "firebase/auth";
import { getDatabase, onValue, ref, set } from "firebase/database";
import {
  COMANDO_LIMPAR,
  caminhoComando,
  caminhoResumo,
  firebaseConfig,
  PIN_LEN,
} from "../config.js";

const app = getApps().length ? getApps()[0] : initializeApp(firebaseConfig);
const auth = getAuth(app);
const db = getDatabase(app);

export function validarPin(pin) {
  return new RegExp(`^[0-9]{${PIN_LEN}}$`).test(pin ?? "");
}

export function validarNome(nome) {
  const n = (nome ?? "").trim();
  return n.length > 0 && n.length <= 24 && !/[:",\\\n\r]/.test(n);
}

function exigirAuth() {
  if (!auth.currentUser) throw new Error("Faça login primeiro.");
}

function epochAgora() {
  return Math.floor(Date.now() / 1000);
}

async function enviar(device, valor) {
  if (!device) throw new Error("Informe o dispositivo.");
  exigirAuth();
  await set(ref(db, caminhoComando(device)), valor);
}

export const enviarCodigo = (device, pin, nome) => {
  if (!validarPin(pin)) throw new Error(`PIN inválido: use ${PIN_LEN} dígitos.`);
  if (!validarNome(nome)) throw new Error("Nome inválido (1–24 letras, sem : \" ,).");
  return enviar(device, `${pin}:${nome.trim()}:${epochAgora()}`);
};

export const renovarCodigo = (device, nome, pinNovo) => {
  if (!nome) throw new Error("Nome inválido.");
  if (!validarPin(pinNovo)) throw new Error(`Novo PIN inválido: use ${PIN_LEN} dígitos.`);
  return enviar(device, `RENOVAR:${nome}:${pinNovo}:${epochAgora()}`);
};

export const excluirCodigo = (device, nome) => {
  if (!nome) throw new Error("Nome inválido.");
  return enviar(device, `DEL:${nome}`);
};

export const limparChaves = (device) => enviar(device, COMANDO_LIMPAR);

// Resumo publicado pelo ESP (nomes+datas, nunca o PIN). O ESP grava como
// string JSON; aqui normalizamos para objeto (aceita os dois formatos).
// Retorna unsubscribe.
export const assinarResumo = (device, cb) =>
  onValue(ref(db, caminhoResumo(device)), (snap) => cb(normalizarResumo(snap.val())));

function normalizarResumo(v) {
  if (typeof v === "string") {
    try {
      v = JSON.parse(v);
    } catch {
      return null;
    }
  }
  if (!v || typeof v !== "object") return null;
  const chaves = Array.isArray(v.chaves)
    ? v.chaves.filter((c) => c && typeof c.nome === "string")
    : [];
  return { total: chaves.length, chaves };
}

export const formatarData = (epoch) =>
  epoch > 0 ? new Date(epoch * 1000).toLocaleString("pt-BR") : "—";
