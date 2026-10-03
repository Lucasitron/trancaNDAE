// Model: comando único + Firebase. Não toca no DOM.
import { initializeApp } from "firebase/app";
import { getAuth, signInWithEmailAndPassword, signOut } from "firebase/auth";
import { getDatabase, ref, set } from "firebase/database";
import { firebaseConfig, PALAVRA_COMANDO, TOKEN_HEX_LEN, PIN_LEN } from "../config.js";

const app = initializeApp(firebaseConfig);
const auth = getAuth(app);
const db = getDatabase(app);

// MODO PLAIN (sem cripto, por hora): publica o PIN em claro; o ESP32
// compara direto com o digitado. gerarToken() mantido p/ retorno futuro.
// Réplica futura do firmware seria: HMAC-SHA256(msg="ABRIR", key=PIN)
// com mbedtls, truncado nos primeiros TOKEN_HEX_LEN/2 bytes.
export async function gerarToken(pin) {
  const key = await crypto.subtle.importKey(
    "raw",
    new TextEncoder().encode(pin),
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign"],
  );
  const sig = await crypto.subtle.sign(
    "HMAC",
    key,
    new TextEncoder().encode(PALAVRA_COMANDO),
  );
  const bytes = new Uint8Array(sig).slice(0, TOKEN_HEX_LEN / 2);
  return [...bytes].map((b) => b.toString(16).padStart(2, "0")).join("");
}

export function validarPin(pin) {
  return new RegExp(`^[0-9]{${PIN_LEN}}$`).test(pin ?? "");
}

export async function enviarToken({ email, password, devicePath, pin }) {
  if (!validarPin(pin)) throw new Error(`PIN inválido: use ${PIN_LEN} dígitos.`);
  await signInWithEmailAndPassword(auth, email, password);
  try {
    await set(ref(db, devicePath), pin);
  } finally {
    await signOut(auth).catch(() => {});
  }
  return pin;
}
