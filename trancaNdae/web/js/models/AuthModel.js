// Model: Auth anônima automática. Sem tela de login, sem DOM.
// Cada visita ganha uma identidade anônima (vale nas Rules `auth != null`).
// ATENÇÃO: sem login, quem abre a página pode enviar comandos — mantenha
// a página em rede/hospedagem restrita (ou volte o login + Rules por UID).
import { getApps, initializeApp } from "firebase/app";
import { getAuth, signInAnonymously } from "firebase/auth";
import { firebaseConfig } from "../config.js";

const app = getApps().length ? getApps()[0] : initializeApp(firebaseConfig);
const auth = getAuth(app);

export const garantirAuth = async () => {
  if (!auth.currentUser) await signInAnonymously(auth);
  return auth.currentUser;
};
