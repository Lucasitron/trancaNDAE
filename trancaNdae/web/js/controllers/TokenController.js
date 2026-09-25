// Controller: liga View <-> Model. Sem querySelector, sem fetch direto.
import { enviarToken } from "../models/TokenModel.js";

export class TokenController {
  constructor(view) {
    this.view = view;
    this.view.onSubmit(() => this.enviar());
  }

  async enviar() {
    const dados = this.view.getDados();
    if (!dados.devicePath) {
      this.view.erro(new Error("Informe o nó do dispositivo."));
      return;
    }
    this.view.carregando(true);
    this.view.info("Gerando HMAC e enviando…");
    try {
      const token = await enviarToken(dados);
      this.view.ok(token, dados.devicePath);
    } catch (e) {
      this.view.erro(e);
    } finally {
      this.view.carregando(false);
    }
  }
}
