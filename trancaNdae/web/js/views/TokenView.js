// View: só DOM. Sem Firebase, sem HMAC.
export class TokenView {
  constructor() {
    this.form = document.getElementById("token-form");
    this.email = document.getElementById("email");
    this.password = document.getElementById("password");
    this.pin = document.getElementById("pin");
    this.devicePath = document.getElementById("device-path");
    this.btn = document.getElementById("btn-enviar");
    this.status = document.getElementById("status");
  }

  getDados() {
    return {
      email: this.email.value.trim(),
      password: this.password.value,
      pin: this.pin.value.trim(),
      devicePath: this.devicePath.value.trim().replace(/^\/+|\/+$/g, ""),
    };
  }

  carregando(on) {
    this.btn.disabled = on;
    this.btn.textContent = on ? "Enviando…" : "Gerar e enviar comando";
  }

  info(msg) {
    this.status.className = "status";
    this.status.textContent = msg;
  }

  ok(token, path) {
    this.status.className = "status ok";
    this.status.textContent = `✅ Token enviado para /${path}: ${token}`;
  }

  erro(e) {
    this.status.className = "status erro";
    this.status.textContent = `❌ ${e?.code ?? ""} ${e?.message ?? e}`.trim();
  }

  onSubmit(handler) {
    this.form.addEventListener("submit", (ev) => {
      ev.preventDefault();
      handler();
    });
  }
}
