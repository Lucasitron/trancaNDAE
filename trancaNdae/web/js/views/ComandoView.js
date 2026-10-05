// View: só DOM. Sem Firebase, sem regra de negócio (sem login).
const $ = (id) => document.getElementById(id);

export class ComandoView {
  constructor() {
    this.device = $("device");
    this.formEnviar = $("form-enviar");
    this.nome = $("nome-enviar");
    this.pin = $("pin-enviar");
    this.validade = $("validade-enviar");
    this.btnLimpar = $("btn-limpar-tudo");
    this.tbody = $("chaves-tbody");
    this.count = $("count");
    this.atualizado = $("resumo-info");
    this.status = $("status");
  }

  deviceAtual() {
    return (this.device.value ?? "").trim();
  }

  onDeviceChange(handler) {
    this.device.addEventListener("change", () => handler(this.deviceAtual()));
  }

  onEnviar(handler) {
    this.formEnviar.addEventListener("submit", (ev) => {
      ev.preventDefault();
      handler(
        this.deviceAtual(),
        this.pin.value.trim(),
        this.nome.value.trim(),
        this.validade.value,
      );
    });
  }

  onLimparTudo(handler) {
    this.btnLimpar.addEventListener("click", handler);
  }

  onAcaoTabela(handler) {
    this.tbody.addEventListener("click", (ev) => {
      const btn = ev.target.closest("button[data-acao]");
      if (btn)
        handler(btn.dataset.acao, btn.dataset.nome, btn.dataset.bloqueada === "1");
    });
  }

  limparPin() {
    this.pin.value = "";
  }

  // Renderiza o resumo vindo do ESP (metadados; o PIN nunca vem).
  // Ações usam o NOME como chave (PINs não trafegam de volta).
  // statusFn/relogio vêm do controller (regra de expiração).
  renderResumo(resumo, formatarData, formatarValidade, statusDe) {
    const chaves = resumo?.chaves ?? [];
    const ativas = chaves.filter((c) => statusDe(c) === "ativa").length;
    this.count.textContent = `(${ativas}/${chaves.length} ativas no ESP)`;
    this.atualizado.textContent = chaves.length
      ? "Lista carregada do ESP (PINs nunca trafegam de volta)."
      : "Nenhuma chave no ESP. Envie a primeira acima.";
    this.tbody.innerHTML = "";
    chaves.forEach((c) => {
      const st = statusDe(c);
      const tr = document.createElement("tr");
      const tdNome = document.createElement("td");
      tdNome.textContent = c.nome || "(sem nome)";
      const tdData = document.createElement("td");
      tdData.textContent = formatarData(c.criadaEm);
      const tdVal = document.createElement("td");
      tdVal.textContent = formatarValidade(c.validadeH);
      const tdStatus = document.createElement("td");
      tdStatus.textContent = st;
      const tdAcoes = document.createElement("td");
      const btnR = document.createElement("button");
      btnR.textContent = "Renovar";
      btnR.title = "Trocar o PIN mantendo o nome";
      btnR.dataset.acao = "renovar";
      btnR.dataset.nome = c.nome;
      const btnB = document.createElement("button");
      const bloqueada = st === "bloqueada";
      btnB.textContent = bloqueada ? "Liberar" : "Bloquear";
      btnB.className = "secundario";
      btnB.dataset.acao = "bloquear";
      btnB.dataset.nome = c.nome;
      btnB.dataset.bloqueada = bloqueada ? "1" : "";
      const btnE = document.createElement("button");
      btnE.textContent = "Excluir";
      btnE.className = "secundario";
      btnE.dataset.acao = "excluir";
      btnE.dataset.nome = c.nome;
      tdAcoes.append(
        btnR,
        document.createTextNode(" "),
        btnB,
        document.createTextNode(" "),
        btnE,
      );
      tr.append(tdNome, tdData, tdVal, tdStatus, tdAcoes);
      this.tbody.append(tr);
    });
  }

  info(msg) {
    this.status.className = "status";
    this.status.textContent = msg;
  }

  ok(msg) {
    this.status.className = "status ok";
    this.status.textContent = `✅ ${msg}`;
  }

  erro(e) {
    this.status.className = "status erro";
    this.status.textContent = `❌ ${e?.code ?? ""} ${e?.message ?? e}`.trim();
  }
}
