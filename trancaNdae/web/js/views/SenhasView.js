// View: só DOM. Sem Firebase, sem HMAC.
import { isExpirada, statusDe } from "../models/SenhasModel.js";
import { MAX_SENHAS } from "../config.js";

const fmtData = (ms) =>
  !ms ? "—" : new Date(ms).toLocaleString("pt-BR", { dateStyle: "short", timeStyle: "short" });

export class SenhasView {
  constructor() {
    const $ = (id) => document.getElementById(id);
    // Login
    this.loginSection = $("login-section");
    this.loginForm = $("login-form");
    this.loginEmail = $("login-email");
    this.loginPass = $("login-password");
    // Manager
    this.managerSection = $("manager-section");
    this.btnLogout = $("btn-logout");
    this.device = $("device");
    this.formCriar = $("form-criar");
    this.nome = $("nome");
    this.pin = $("pin");
    this.btnGerarPin = $("btn-gerar-pin");
    this.validade = $("validade");
    this.tbody = $("senhas-tbody");
    this.count = $("count");
    this.btnLimpar = $("btn-limpar-expiradas");
    this.pinUmaVez = $("pin-uma-vez");
    this.status = $("status");
  }

  // ---- auth ----
  onLogin(handler) {
    this.loginForm.addEventListener("submit", (ev) => {
      ev.preventDefault();
      handler(this.loginEmail.value.trim(), this.loginPass.value);
    });
  }

  onLogout(handler) {
    this.btnLogout.addEventListener("click", handler);
  }

  mostrarManager(user) {
    const logado = !!user;
    this.loginSection.hidden = logado;
    this.managerSection.hidden = !logado;
    if (logado) this.info(`Logado como ${user.email}.`);
  }

  // ---- criar ----
  onGerarPin(handler) {
    this.btnGerarPin.addEventListener("click", () => {
      this.pin.value = handler();
    });
  }

  onCriar(handler) {
    this.formCriar.addEventListener("submit", (ev) => {
      ev.preventDefault();
      handler({
        nome: this.nome.value,
        pin: this.pin.value.trim(),
        validadeHoras: Number(this.validade.value),
        device: this.device.value.trim().replace(/^\/+|\/+$/g, "") || "dispositivo1",
      });
    });
  }

  limparFormCriar() {
    this.nome.value = "";
    this.pin.value = "";
  }

  mostrarPinUmaVez(nome, pin) {
    this.pinUmaVez.hidden = false;
    this.pinUmaVez.textContent = `✅ "${nome}" cadastrado. PIN (anote agora, não fica salvo): ${pin}`;
  }

  esconderPinUmaVez() {
    this.pinUmaVez.hidden = true;
  }

  // ---- lista ----
  onDeviceChange(handler) {
    this.device.addEventListener("change", () => handler(this.deviceAtual()));
  }

  deviceAtual() {
    return this.device.value.trim().replace(/^\/+|\/+$/g, "") || "dispositivo1";
  }

  renderLista(itensObj) {
    const entries = Object.entries(itensObj ?? {}).sort(
      (a, b) => (b[1].criadaEm ?? 0) - (a[1].criadaEm ?? 0),
    );
    const agora = Date.now();
    const ativas = entries.filter(([, it]) => statusDe(it, agora) === "ativa").length;
    this.count.textContent = `${ativas}/${MAX_SENHAS} ativas · ${entries.length} cadastradas`;

    if (!entries.length) {
      this.tbody.innerHTML = `<tr><td colspan="5" class="vazio">Nenhuma senha cadastrada.</td></tr>`;
      return;
    }
    this.tbody.innerHTML = entries
      .map(([id, it]) => {
        const st = statusDe(it, agora);
        const selo = st === "ativa" ? "ok" : st === "bloqueada" ? "bloq" : "exp";
        const acao = it.ativa ? "Bloquear" : "Liberar";
        return `<tr>
          <td>${this.esc(it.nome ?? "—")}</td>
          <td>${fmtData(it.criadaEm)}</td>
          <td>${it.expiraEm ? fmtData(it.expiraEm) : "sem expiração"}${
            isExpirada(it, agora) ? " (expirou)" : ""
          }</td>
          <td><span class="selo ${selo}">${st}</span></td>
          <td class="acoes">
            <button data-acao="toggle" data-id="${id}" data-ativa="${it.ativa ? 1 : 0}">${acao}</button>
            <button data-acao="excluir" data-id="${id}" class="perigo">Excluir</button>
          </td>
        </tr>`;
      })
      .join("");
  }

  onAcao(handler) {
    this.tbody.addEventListener("click", (ev) => {
      const btn = ev.target.closest("button[data-acao]");
      if (!btn) return;
      handler(btn.dataset.acao, btn.dataset.id, btn.dataset.ativa === "1");
    });
  }

  onLimparExpiradas(handler) {
    this.btnLimpar.addEventListener("click", handler);
  }

  esc(s) {
    return String(s).replace(/[&<>"']/g, (c) => ({
      "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;",
    })[c]);
  }

  // ---- status ----
  info(msg) {
    this.status.className = "status";
    this.status.textContent = msg;
  }

  ok(msg) {
    this.status.className = "status ok";
    this.status.textContent = msg;
  }

  erro(e) {
    this.status.className = "status erro";
    this.status.textContent = `❌ ${e?.code ?? ""} ${e?.message ?? e}`.trim();
  }
}
