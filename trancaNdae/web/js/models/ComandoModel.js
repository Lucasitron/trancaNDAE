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
import { getDatabase, onValue, ref, remove, set } from "firebase/database";
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

export const enviarCodigo = (device, pin, nome, validadeH = 0) => {
  if (!validarPin(pin)) throw new Error(`PIN inválido: use ${PIN_LEN} dígitos.`);
  if (!validarNome(nome)) throw new Error("Nome inválido (1–24 letras, sem : \" ,).");
  const vh = Math.max(0, parseInt(validadeH, 10) || 0);
  return enviar(device, `${pin}:${nome.trim()}:${epochAgora()}:${vh}`);
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

export const alternarBloqueio = (device, nome, bloquear) => {
  if (!nome) throw new Error("Nome inválido.");
  return enviar(device, `${bloquear ? "BLOQ" : "LIB"}:${nome}`);
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
    ? v.chaves
        .filter((c) => c && typeof c.nome === "string")
        .map((c) => ({
          nome: c.nome,
          criadaEm: c.criadaEm > 0 ? c.criadaEm : 0,
          validadeH: c.validadeH > 0 ? c.validadeH : 0,
          ativa: c.ativa !== 0,
        }))
    : [];
  return { total: chaves.length, chaves };
}

// true se expirada (só computável com data de cadastro + validade).
export const isExpirada = (item, agora = Math.floor(Date.now() / 1000)) =>
  !!item && item.ativa && item.validadeH > 0 && item.criadaEm > 0 &&
  agora > item.criadaEm + item.validadeH * 3600;

export const statusDe = (item, agora) => {
  if (!item?.ativa) return "bloqueada";
  if (isExpirada(item, agora)) return "expirada";
  return "ativa";
};

export const formatarValidade = (h) => {
  if (!(h > 0)) return "sem expiração";
  if (h < 24) return `${h}h`;
  return `${Math.round(h / 24)} dias`;
};

// Aguarda o ESP reagir (resumo mudar) e APAGA o comando —
// PIN não permanece no database nem em caso de falha (timeout 45s).
// Retorna "ok" (confirmado) ou "timeout" (apagado sem confirmação).
export const aguardarReacaoEApagar = (device, resumoAnterior, timeoutMs = 45000) =>
  new Promise((resolve) => {
    const antes = JSON.stringify(resumoAnterior ?? null);
    let feito = false;
    const finalizar = (r) => {
      if (feito) return;
      feito = true;
      clearTimeout(timer);
      unsub();
      remove(ref(db, caminhoComando(device))).catch(() => {});
      resolve(r);
    };
    const timer = setTimeout(() => finalizar("timeout"), timeoutMs);
    const unsub = onValue(
      ref(db, caminhoResumo(device)),
      (snap) => {
        const atual = normalizarResumo(snap.val());
        if (JSON.stringify(atual) !== antes) finalizar("ok");
      },
      () => finalizar("timeout"),
    );
  });

export const formatarData = (epoch) =>
  epoch > 0 ? new Date(epoch * 1000).toLocaleString("pt-BR") : "—";
