# Tabela Local de Chaves (`senhas_store.h/.cpp`)

Única fonte de chaves: **só existe no ESP** (RAM estática + NVS).
Capacidade: **30 chaves**, sem heap.

## Registro e formato compacto

Cada chave é um `struct Registro` de **28 bytes**:

| Campo | Tipo | Bytes | Observação |
| :--- | :--- | ---: | :--- |
| `pin` | `uint16_t` | 2 | 0–9999; `0` = `0000` reservado |
| `ativa` | `uint8_t` | 1 | 0/1 (bloqueio) |
| `nome` | `char[25]` | 25 | etiqueta única, ASCII imprimível |

O PIN **não** é guardado como texto. Um PIN de 4 dígitos cabe em 14 bits,
então `uint16_t` é o menor tipo nativo:

| Formato | Bytes por PIN | Total (30) |
| :--- | ---: | ---: |
| ASCII `"1234\0"` (antes) | 5 | 150 |
| Hexa textual `"04D2"` | 4 | 120 (e exige conversão) |
| `uint16_t` (atual) | **2** | **60** |

Hexa textual não compensa: são 4 caracteres, quase o mesmo espaço do
ASCII, e ainda exige `strtol`. O `uint16_t` é comparado direto (`==`) e
formatado com `%04u` quando necessário — conversão só na borda (comando
recebido), nunca no caminho de abertura.

RAM total: `30 × 28 = 840 bytes`. O nome (25 bytes) domina; compactá-lo
para 7 bits/caractere economizaria ~120 bytes ao custo de bit-packing —
não vale a complexidade. A flag `ativa` também fica em 1 byte por chave
(um bitset salvaria 26 bytes, mas com código bem menos legível).

## NVS

Antes: 3 entradas por chave (`t00` PIN, `n00` nome, `a00` ativa) + `n`
contador (~90 entradas com 20 chaves).

Agora: **uma única entrada**:

- `senhas/v` — versão do formato (`1`);
- `senhas/n` — contador de chaves;
- `senhas/regs` — blob com `n × 28` bytes.

Menos entradas = menos overhead do NVS (cada entrada tem metadados
próprios), gravação mais rápida e código de persistência sem o laço de
`isKey`/`remove` por índice. A gravação é precedida da versão e sucedida
do contador; na leitura o contador é limitado ao tamanho real do blob e
cada registro é validado (PIN ≤ 9999, flag 0/1, nome terminado), então
uma escrita interrompida não corrompe a tabela.

**Migração:** no primeiro boot sem a chave `v`, o formato antigo
(`t00/n00/a00`) é lido, convertido para `uint16` e regravado no blob; as
chaves antigas são removidas. Se o `struct` mudar no futuro, incremente
`NVS_VERSAO_ATUAL` (formato desconhecido é ignorado, não interpretado
errado).

## API

| Função | Descrição |
| :--- | :--- |
| `senhas_init()` | NVS → RAM (migra o formato antigo, se houver) |
| `senhas_adicionar(pin, nome)` | Rejeita inválido/duplicado/lotado/`0000` |
| `senhas_remover_nome(nome)` | Remove uma |
| `senhas_renovar(nome, pinNovo)` | Troca o PIN mantendo o nome |
| `senhas_bloquear(nome, bloquear)` | Bloqueio sem apagar |
| `senhas_limpar()` | Zera tudo (comando `LIMPAR`) |
| `senhas_total()` / `senhas_total_ativas()` | Contadores |
| `senhas_contem(pin)` | `true` só se existe **e ativa** (porta) |
| `senhas_consumirMudanca()` | Flag p/ atualizar o display |

`nome` é sanitizado na entrada (1–24, sem `.:#$ /[]:"` e controle — igual
à validação da página). Comparações de nome usam a cópia limpa, sem
alocar `String`.
