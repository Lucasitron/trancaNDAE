// Model: metadados (Firebase, geridos pela página) + comandos (ESP).
// O PIN nunca é armazenado: só trafega no comando volátil, que o ESP
// consome e apaga. Os metadados ficam em pessoas/{device}/{nome}.
import { getApps, initializeApp } from "firebase/app";
import { getDatabase, onValue, ref, remove, set, update } from "firebase/database";
import {
  COMANDO_LIMPAR,
  caminhoComando,
  caminhoPessoas,
  firebaseConfig,
  PIN_LEN,
} from "../config.js";

const app = getApps().length ? getApps()[0] : initializeApp(firebaseConfig);
const db = getDatabase(app);

export function validarPin(pin) {
  // "0000" é reservado ao reinício da tranca (0000 + '*').
  return new RegExp(`^[0-9]{${PIN_LEN}}$`).test(pin ?? "") && pin !== "0000";
}

// Nome = chave no RTDB: sem . # $ / [ ] : " \ e sem controle.
export function validarNome(nome) {
  const n = (nome ?? "").trim();
  return n.length > 0 && n.length <= 24 && !/[.#$/\[\]:"\\\n\r]/.test(n);
}

export function validarWifi(ssid, senha) {
  const s = (ssid ?? "").trim();
  if (s.length < 1 || s.length > 32 || /[:\n\r]/.test(s)) return false;
  const p = senha ?? "";
  if (p.length > 63 || /[:\n\r]/.test(p)) return false;
  if (p.length > 0 && p.length < 8) return false; // WPA mínimo 8 (vazio = aberta)
  for (const c of s + p) {
    const o = c.codePointAt(0);
    if (o < 32 || o > 126) return false;
  }
  return true;
}

const epochAgora = () => Math.floor(Date.now() / 1000);

// ---- comandos voláteis (ESP) ----
async function enviar(device, valor) {
  if (!device) throw new Error("Informe o dispositivo.");
  await set(ref(db, caminhoComando(device)), valor);
}

// ---- metadados (Firebase) ----
export const assinarPessoas = (device, cb) =>
  onValue(ref(db, caminhoPessoas(device)), (snap) => cb(normalizarPessoas(snap.val())));

function normalizarPessoas(v) {
  if (!v || typeof v !== "object") return { total: 0, chaves: [] };
  const chaves = Object.entries(v)
    .filter(([, p]) => p && typeof p.nome === "string")
    .map(([id, p]) => ({
      id,
      nome: p.nome,
      criadaEm: p.criadaEm > 0 ? p.criadaEm : 0,
      validadeH: p.validadeH > 0 ? p.validadeH : 0,
      ativa: p.ativa !== false,
    }));
  return { total: chaves.length, chaves };
}

export const isExpirada = (item, agora = epochAgora()) =>
  !!item && item.ativa && item.validadeH > 0 && item.criadaEm > 0 &&
  agora > item.criadaEm + item.validadeH * 3600;

export const statusDe = (item, agora) => {
  if (!item?.ativa) return "bloqueada";
  if (isExpirada(item, agora)) return "expirada";
  return "ativa";
};

export const formatarData = (epoch) =>
  epoch > 0 ? new Date(epoch * 1000).toLocaleString("pt-BR") : "—";

export const formatarValidade = (h) => {
  if (!(h > 0)) return "sem expiração";
  if (h < 24) return `${h}h`;
  return `${Math.round(h / 24)} dias`;
};

// ---- operações (metadados + comando) ----
async function gravarMeta(device, nome, meta) {
  await set(ref(db, `${caminhoPessoas(device)}/${nome}`), { nome, ...meta });
}

export async function cadastrar(device, pin, nome, validadeH) {
  if (!validarPin(pin)) throw new Error(`PIN inválido: ${PIN_LEN} dígitos (0000 é reservado).`);
  if (!validarNome(nome)) throw new Error('Nome inválido (1–24, sem . # $ / [ ] : " ,).');
  const n = nome.trim();
  const vh = Math.max(0, parseInt(validadeH, 10) || 0);
  await gravarMeta(device, n, { criadaEm: epochAgora(), validadeH: vh, ativa: true });
  await enviar(device, `${pin}:${n}`);
}

export async function renovar(device, nome, pinNovo) {
  if (!validarPin(pinNovo)) throw new Error(`Novo PIN inválido: ${PIN_LEN} dígitos (0000 é reservado).`);
  await update(ref(db, `${caminhoPessoas(device)}/${nome}`), { criadaEm: epochAgora() });
  await enviar(device, `RENOVAR:${nome}:${pinNovo}`);
}

export async function alternarBloqueio(device, nome, bloquear) {
  await update(ref(db, `${caminhoPessoas(device)}/${nome}`), { ativa: !bloquear });
  await enviar(device, `${bloquear ? "BLOQ" : "LIB"}:${nome}`);
}

export async function excluir(device, nome) {
  await remove(ref(db, `${caminhoPessoas(device)}/${nome}`));
  await enviar(device, `DEL:${nome}`);
}

export async function limpar(device) {
  await remove(ref(db, caminhoPessoas(device)));
  await enviar(device, COMANDO_LIMPAR);
}

export async function salvarWifi2(device, ssid, senha) {
  if (!validarWifi(ssid, senha))
    throw new Error("Wi-Fi inválido: SSID 1–32, senha vazia ou 8–63 (sem ':').");
  await enviar(device, `WIFI2:${ssid.trim()}:${senha}`);
}

export async function desativarWifi2(device) {
  await enviar(device, "WIFI2OFF");
}

// Bloqueia no ESP as expiradas (a página aplica a expiração ao carregar).
export async function bloquearExpiradas(device, chaves, agora = epochAgora()) {
  for (const c of chaves) {
    if (isExpirada(c, agora)) await alternarBloqueio(device, c.nome, true);
  }
}

// Aguarda o ESP consumir (o nó comandos/{device} vira null). Em timeout,
// apaga o comando à força — o PIN não permanece no database.
export const aguardarConsumo = (device, timeoutMs = 45000) =>
  new Promise((resolve) => {
    let feito = false;
    const finalizar = (r) => {
      if (feito) return;
      feito = true;
      clearTimeout(timer);
      unsub();
      if (r === "timeout") remove(ref(db, caminhoComando(device))).catch(() => {});
      resolve(r);
    };
    const timer = setTimeout(() => finalizar("timeout"), timeoutMs);
    const unsub = onValue(
      ref(db, caminhoComando(device)),
      (snap) => {
        if (snap.val() === null) finalizar("ok");
      },
      () => finalizar("timeout"),
    );
  });
