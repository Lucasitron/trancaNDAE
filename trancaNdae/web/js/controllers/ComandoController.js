// Controller: liga View <-> Model. Sem querySelector, sem Firebase direto.
// Sem login. Metadados vêm do Firebase (pessoas), não do ESP.
import {
  aguardarConsumo,
  alternarBloqueio,
  assinarPessoas,
  bloquearExpiradas,
  cadastrar,
  excluir,
  formatarData,
  formatarValidade,
  limpar,
  renovar,
  statusDe,
} from "../models/ComandoModel.js";

export class ComandoController {
  constructor(view) {
    this.view = view;
    this.unsubPessoas = null;
    this.higieneFeita = false;

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

    this.assinar(this.view.deviceAtual());
  }

  assinar(device) {
    this.desassinar();
    if (!device) return;
    this.view.info(`Carregando chaves de "${device}"…`);
    this.unsubPessoas = assinarPessoas(device, (dados) => {
      this.view.renderChaves(dados, formatarData, formatarValidade, statusDe);
      this.higiene(device, dados);
    });
  }

  desassinar() {
    this.unsubPessoas?.();
    this.unsubPessoas = null;
  }

  // Bloqueia no ESP as expiradas (1x por dispositivo). O ESP não tem
  // relógio: a expiração é aplicada pela página ao carregar.
  async higiene(device, dados) {
    if (this.higieneFeita) return;
    this.higieneFeita = true;
    try {
      await bloquearExpiradas(device, dados.chaves);
    } catch (e) {
      this.view.erro(e);
    }
  }

  // Executa a operação no ESP e aguarda o consumo do comando.
  async executar(device, rotulo, fn) {
    try {
      await fn();
      this.view.info(`${rotulo} Aguardando o ESP consumir…`);
      const r = await aguardarConsumo(device);
      this.view.ok(
        r === "ok"
          ? `${rotulo} Confirmado (comando apagado).`
          : `${rotulo} Comando apagado sem confirmação do ESP.`,
      );
    } catch (e) {
      this.view.erro(e);
    }
  }

  async enviar(device, pin, nome, validade) {
    await this.executar(device, `"${nome.trim()}" enviada.`, () =>
      cadastrar(device, pin, nome, validade),
    );
    this.view.limparPin();
  }

  async acao(acao, nome, bloqueada) {
    const device = this.view.deviceAtual();
    if (acao === "renovar") {
      const novo = prompt(`Novo PIN de 4 dígitos para "${nome}":`);
      if (novo === null) return;
      await this.executar(device, `Renovação de "${nome}".`, () =>
        renovar(device, nome, novo.trim()),
      );
    } else if (acao === "bloquear") {
      await this.executar(device, `"${nome}" ${bloqueada ? "liberada" : "bloqueada"}.`, () =>
        alternarBloqueio(device, nome, !bloqueada),
      );
    } else if (acao === "excluir") {
      if (!confirm(`Excluir "${nome}"?`)) return;
      await this.executar(device, `Exclusão de "${nome}".`, () =>
        excluir(device, nome),
      );
    }
  }

  async limpar() {
    const device = this.view.deviceAtual();
    if (!device) return this.view.erro(new Error("Informe o dispositivo."));
    if (!confirm(`Apagar TODAS as chaves em "${device}"?`)) return;
    await this.executar(device, "Limpeza total.", () => limpar(device));
  }
}
