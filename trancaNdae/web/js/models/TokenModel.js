// Model: HMAC + Firebase. Não toca no DOM.
import { initializeApp } from "firebase/app";
import { getAuth, signInWithEmailAndPassword, signOut } from "firebase/auth";
import { getDatabase, ref, set } from "firebase/database";
import { firebaseConfig, PALAVRA_COMANDO } from "../config.js";

const app = initializeApp(firebaseConfig);
const auth = getAuth(app);
const db = getDatabase(app);

// Replica exata do firmware (src/main.cpp: hmacSha256):
// HMAC-SHA256(msg="ABRIR", key=PIN) com mbedtls, truncado nos 16
// primeiros bytes -> 32 chars hex minúsculos.
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
  const bytes = new Uint8Array(sig).slice(0, 16);
  return [...bytes].map((b) => b.toString(16).padStart(2, "0")).join("");
}

export function validarPin(pin) {
  return /^[0-9*#]{4,16}$/.test(pin ?? "");
}

export async function enviarToken({ email, password, devicePath, pin }) {
  if (!validarPin(pin)) throw new Error("PIN inválido: use 4–16 dígitos.");
  const token = await gerarToken(pin);
  await signInWithEmailAndPassword(auth, email, password);
  try {
    await set(ref(db, devicePath), token);
  } finally {
    await signOut(auth).catch(() => {});
  }
  return token;
}
