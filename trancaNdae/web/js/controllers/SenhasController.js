// Controller: liga View <-> Model. Sem querySelector, sem Firebase direto.
import {
  assinarItens,
  criarSenha,
  gerarPinAleatorio,
  limparExpiradas,
  login,
  logout,
  onAuth,
  removerSenha,
  setAtiva,
} from "../models/SenhasModel.js";

export class SenhasController {
  constructor(view) {
    this.view = view;
    this.unsubItens = null;

    this.view.onLogin((email, pass) => this.doLogin(email, pass));
    this.view.onLogout(() => logout().catch((e) => this.view.erro(e)));
    this.view.onGerarPin(() => gerarPinAleatorio(6));
    this.view.onCriar((dados) => this.criar(dados));
    this.view.onAcao((acao, id, ativa) => this.acao(acao, id, ativa));
    this.view.onLimparExpiradas(() => this.limpar());
    this.view.onDeviceChange((device) => this.assinar(device));

    onAuth((user) => {
      this.view.mostrarManager(user);
      if (user) this.assinar(this.view.deviceAtual());
      else this.desassinar();
    });
  }

  assinar(device) {
    this.desassinar();
    this.view.info(`Ouvindo senhas de "${device}"…`);
    this.unsubItens = assinarItens(device, (itens) => this.view.renderLista(itens));
  }

  desassinar() {
    this.unsubItens?.();
    this.unsubItens = null;
  }

  async doLogin(email, password) {
    try {
      await login(email, password);
      this.view.ok("Login OK.");
    } catch (e) {
      this.view.erro(e);
    }
  }

  async criar({ nome, pin, validadeHoras, device }) {
    if (!device) return this.view.erro(new Error("Informe o dispositivo."));
    this.view.info("Gerando HMAC e publicando…");
    try {
      const { pin: pinCriado } = await criarSenha(device, { nome, pin, validadeHoras });
      this.view.mostrarPinUmaVez(nome.trim(), pinCriado);
      this.view.limparFormCriar();
      this.view.ok("Senha publicada. O ESP32 sincroniza em segundos.");
    } catch (e) {
      this.view.erro(e);
    }
  }

  async acao(acao, id, ativa) {
    const device = this.view.deviceAtual();
    try {
      if (acao === "toggle") {
        await setAtiva(device, id, !ativa);
        this.view.info(ativa ? "Senha bloqueada." : "Senha liberada.");
      } else if (acao === "excluir") {
        if (!confirm("Excluir esta senha?")) return;
        await removerSenha(device, id);
        this.view.info("Senha excluída.");
      }
    } catch (e) {
      this.view.erro(e);
    }
  }

  async limpar() {
    try {
      const n = await limparExpiradas(this.view.deviceAtual());
      this.view.ok(n ? `${n} expirada(s) removida(s).` : "Nada expirado.");
    } catch (e) {
      this.view.erro(e);
    }
  }
}
