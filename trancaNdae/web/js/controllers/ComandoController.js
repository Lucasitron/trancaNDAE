// Controller: liga View <-> Model. Sem querySelector, sem Firebase direto.
// Sem login: auth anônima automática; assina o resumo ao abrir.
import { garantirAuth } from "../models/AuthModel.js";
import {
  aguardarReacaoEApagar,
  alternarBloqueio,
  assinarResumo,
  enviarCodigo,
  excluirCodigo,
  formatarData,
  formatarValidade,
  isExpirada,
  limparChaves,
  renovarCodigo,
  statusDe,
} from "../models/ComandoModel.js";

export class ComandoController {
  constructor(view) {
    this.view = view;
    this.unsubResumo = null;
    this.higieneFeita = false;
    this.ultimoResumo = null;

    this.view.onEnviar((device, pin, nome, validade) =>
      this.enviar(device, pin, nome, validade),
    );
    this.view.onLimparTudo(() => this.limpar());
    this.view.onAcaoTabela((acao, nome, bloqueada) =>
      this.acao(acao, nome, bloqueada),
    );
    this.view.onDeviceChange((device) => {
      this.higieneFeita = false;
      this.assinar(device);
    });

    garantirAuth()
      .then(() => this.assinar(this.view.deviceAtual()))
      .catch((e) => this.view.erro(e));
  }

  assinar(device) {
    this.desassinar();
    if (!device) return;
    this.view.info(`Lendo chaves de "${device}" direto do ESP…`);
    this.unsubResumo = assinarResumo(device, (resumo) => {
      this.ultimoResumo = resumo;
      this.view.renderResumo(resumo, formatarData, formatarValidade, statusDe);
      this.higiene(device, resumo);
    });
  }

  desassinar() {
    this.unsubResumo?.();
    this.unsubResumo = null;
  }

  // Bloqueia no ESP as expiradas ainda ativas (higiene, 1x por dispositivo).
  // O ESP não tem relógio: a web aplica a expiração ao carregar a lista.
  async higiene(device, resumo) {
    if (this.higieneFeita) return;
    this.higieneFeita = true;
    const agora = Math.floor(Date.now() / 1000);
    for (const c of resumo?.chaves ?? []) {
      if (isExpirada(c, agora)) {
        try {
          await alternarBloqueio(device, c.nome, true);
        } catch (e) {
          this.view.erro(e);
          return;
        }
      }
    }
  }

  // Envia e, ao confirmar reação do ESP (ou timeout), apaga o comando:
  // o PIN não permanece no database.
  async enviarConfirmando(device, fnEnvio, msgOk) {
    const antes = this.ultimoResumo;
    try {
      await fnEnvio();
      this.view.info(`${msgOk} Aguardando o ESP consumir…`);
      const r = await aguardarReacaoEApagar(device, antes);
      this.view.ok(
        r === "ok"
          ? `${msgOk} Confirmado e apagado do database.`
          : `${msgOk} Comando apagado (sem confirmação do ESP).`,
      );
    } catch (e) {
      this.view.erro(e);
    }
  }

  async enviar(device, pin, nome, validade) {
    await this.enviarConfirmando(
      device,
      () => enviarCodigo(device, pin, nome, validade),
      `"${nome}" enviado.`,
    );
    this.view.limparPin();
  }

  async acao(acao, nome, bloqueada) {
    const device = this.view.deviceAtual();
    if (acao === "renovar") {
      const novo = prompt(`Novo PIN de 4 dígitos para "${nome}":`);
      if (novo === null) return;
      await this.enviarConfirmando(
        device,
        () => renovarCodigo(device, nome, novo.trim()),
        `Renovação de "${nome}" enviada.`,
      );
    } else if (acao === "bloquear") {
      await this.enviarConfirmando(
        device,
        () => alternarBloqueio(device, nome, !bloqueada),
        `"${nome}" ${bloqueada ? "liberada" : "bloqueada"}.`,
      );
    } else if (acao === "excluir") {
      if (!confirm(`Excluir "${nome}" do ESP?`)) return;
      await this.enviarConfirmando(
        device,
        () => excluirCodigo(device, nome),
        `Exclusão de "${nome}" enviada.`,
      );
    }
  }

  async limpar() {
    const device = this.view.deviceAtual();
    if (!device) return this.view.erro(new Error("Informe o dispositivo."));
    if (!confirm(`Apagar TODAS as chaves do ESP em "${device}"?`)) return;
    await this.enviarConfirmando(device, () => limparChaves(device), "LIMPAR enviado.");
  }
}
