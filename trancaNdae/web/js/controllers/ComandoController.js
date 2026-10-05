// Controller: liga View <-> Model. Sem querySelector, sem Firebase direto.
// Sem login: auth anônima automática; assina o resumo ao abrir.
import { garantirAuth } from "../models/AuthModel.js";
import {
  assinarResumo,
  enviarCodigo,
  excluirCodigo,
  formatarData,
  limparChaves,
  renovarCodigo,
} from "../models/ComandoModel.js";

export class ComandoController {
  constructor(view) {
    this.view = view;
    this.unsubResumo = null;

    this.view.onEnviar((device, pin, nome) => this.enviar(device, pin, nome));
    this.view.onLimparTudo(() => this.limpar());
    this.view.onAcaoTabela((acao, nome) => this.acao(acao, nome));
    this.view.onDeviceChange((device) => this.assinar(device));

    garantirAuth()
      .then(() => this.assinar(this.view.deviceAtual()))
      .catch((e) => this.view.erro(e));
  }

  assinar(device) {
    this.desassinar();
    if (!device) return;
    this.view.info(`Lendo chaves de "${device}" direto do ESP…`);
    this.unsubResumo = assinarResumo(device, (resumo) =>
      this.view.renderResumo(resumo, formatarData),
    );
  }

  desassinar() {
    this.unsubResumo?.();
    this.unsubResumo = null;
  }

  async enviar(device, pin, nome) {
    try {
      const codigo = await enviarCodigo(device, pin, nome);
      this.view.aposEnvio(codigo, nome);
    } catch (e) {
      this.view.erro(e);
    }
  }

  async acao(acao, nome) {
    const device = this.view.deviceAtual();
    try {
      if (acao === "renovar") {
        const novo = prompt(`Novo PIN de 4 dígitos para "${nome}":`);
        if (novo === null) return;
        await renovarCodigo(device, nome, novo.trim());
        this.view.ok(`Renovação de "${nome}" enviada. Aguarde o ESP confirmar.`);
      } else if (acao === "excluir") {
        if (!confirm(`Excluir "${nome}" do ESP?`)) return;
        await excluirCodigo(device, nome);
        this.view.ok(`Exclusão de "${nome}" enviada. Aguarde o ESP confirmar.`);
      }
    } catch (e) {
      this.view.erro(e);
    }
  }

  async limpar() {
    const device = this.view.deviceAtual();
    if (!device) return this.view.erro(new Error("Informe o dispositivo."));
    if (!confirm(`Apagar TODAS as chaves do ESP em "${device}"?`)) return;
    try {
      await limparChaves(device);
      this.view.ok("Comando LIMPAR enviado. O ESP zera a tabela.");
    } catch (e) {
      this.view.erro(e);
    }
  }
}
