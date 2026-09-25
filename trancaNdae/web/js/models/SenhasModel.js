// Model: CRUD das senhas + publicação da lista compacta. Não toca no DOM.
//
// Esquema no RTDB (ver js/config.js):
//   senhas/{device}/itens/{pushId} = { nome, token, ativa, expiraEm, criadaEm }
//   senhas/{device}/lista          = "tok1,tok2,..." (só ativas e não expiradas)
//
// O ESP32 faz stream SOMENTE de `lista` (string, sem JSON) e o PIN nunca
// trafega na rede: a web publica só o HMAC("ABRIR", PIN), igual ao firmware.
import { getApps, initializeApp } from "firebase/app";
import {
  getAuth,
  signInWithEmailAndPassword,
  signOut,
  onAuthStateChanged,
} from "firebase/auth";
import {
  get,
  getDatabase,
  onValue,
  push,
  ref,
  remove,
  set,
} from "firebase/database";
import { firebaseConfig, MAX_SENHAS, caminhoItens, caminhoLista } from "../config.js";
import { gerarToken, validarPin } from "./TokenModel.js";

const app = getApps().length ? getApps()[0] : initializeApp(firebaseConfig);
const auth = getAuth(app);
const db = getDatabase(app);

export const isExpirada = (item, agora = Date.now()) =>
  !!item?.expiraEm && item.expiraEm <= agora;

export const statusDe = (item, agora = Date.now()) => {
  if (!item?.ativa) return "bloqueada";
  if (isExpirada(item, agora)) return "expirada";
  return "ativa";
};

// Itens que o ESP32 aceita: ativas, não expiradas, no máximo MAX_SENHAS.
export function tokensPublicaveis(itensObj) {
  const agora = Date.now();
  return Object.values(itensObj ?? {})
    .filter((it) => it?.ativa && !isExpirada(it, agora) && typeof it?.token === "string")
    .sort((a, b) => (a.criadaEm ?? 0) - (b.criadaEm ?? 0))
    .slice(0, MAX_SENHAS)
    .map((it) => it.token);
}

async function lerItens(device) {
  const snap = await get(ref(db, caminhoItens(device)));
  return snap.val() ?? {};
}

// Recompõe `lista` a partir de `itens` (fonte da verdade).
// Chamado após toda mutação: converte por last-write-wins, pois todo
// escritor recalcula a partir dos mesmos `itens`.
export async function publicarLista(device) {
  const tokens = tokensPublicaveis(await lerItens(device));
  await set(ref(db, caminhoLista(device)), tokens.join(","));
  return tokens.length;
}

export function gerarPinAleatorio(n = 6) {
  const buf = new Uint32Array(n);
  crypto.getRandomValues(buf);
  return [...buf].map((v) => String(v % 10)).join("");
}

export async function criarSenha(device, { nome, pin, validadeHoras }) {
  nome = (nome ?? "").trim().slice(0, 40);
  if (!nome) throw new Error("Informe o nome do morador/visitante.");
  if (!validarPin(pin)) throw new Error("PIN inválido: use 4–16 dígitos.");

  const itens = await lerItens(device);
  if (tokensPublicaveis(itens).length >= MAX_SENHAS)
    throw new Error(`Limite de ${MAX_SENHAS} senhas ativas atingido.`);

  const token = await gerarToken(pin);
  if (Object.values(itens).some((it) => it?.token === token))
    throw new Error("Este PIN já está cadastrado.");

  const agora = Date.now();
  const expiraEm =
    validadeHoras > 0 ? agora + validadeHoras * 3600_000 : 0; // 0 = sem expiração
  const novo = await push(ref(db, caminhoItens(device)), {
    nome,
    token,
    ativa: true,
    expiraEm,
    criadaEm: agora,
  });
  await publicarLista(device);
  return { id: novo.key, pin }; // pin retornado UMA vez p/ exibição
}

export async function setAtiva(device, id, ativa) {
  const itemRef = ref(db, `${caminhoItens(device)}/${id}`);
  const snap = await get(itemRef);
  if (!snap.exists()) throw new Error("Senha não encontrada.");
  await set(itemRef, { ...snap.val(), ativa });
  await publicarLista(device);
}

export async function removerSenha(device, id) {
  await remove(ref(db, `${caminhoItens(device)}/${id}`));
  await publicarLista(device);
}

// Remove itens expirados (higiene: libera slots e mantém `lista` curta).
export async function limparExpiradas(device) {
  const itens = await lerItens(device);
  const agora = Date.now();
  let removidas = 0;
  for (const [id, it] of Object.entries(itens)) {
    if (isExpirada(it, agora)) {
      await remove(ref(db, `${caminhoItens(device)}/${id}`));
      removidas++;
    }
  }
  if (removidas) await publicarLista(device);
  return removidas;
}

export const login = (email, password) =>
  signInWithEmailAndPassword(auth, email, password);

export const logout = () => signOut(auth);

export const onAuth = (cb) => onAuthStateChanged(auth, cb);

// Assinatura realtime dos `itens` p/ renderizar a tabela. Retorna unsubscribe.
export const assinarItens = (device, cb) =>
  onValue(ref(db, caminhoItens(device)), (snap) => cb(snap.val() ?? {}));
