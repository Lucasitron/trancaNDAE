# Tabela Local de Chaves (`senhas_store.h/.cpp`)

Única fonte de chaves: **só existe no ESP** (RAM estática + NVS).
Capacidade: 20 × (PIN 4 + nome 24 + flag) ≈ 620 bytes, sem heap.

## Registro

`PIN` (4 dígitos, `0000` reservado) + `nome` (etiqueta única, 24,
sem `.:#$ /[]:"` e controle — igual à validação da página) + `ativa`.

## API

| Função | Descrição |
| :--- | :--- |
| `senhas_init()` | NVS → RAM (migra entradas antigas como ativas) |
| `senhas_adicionar(pin, nome)` | Rejeita inválido/duplicado/lotado/`0000` |
| `senhas_remover_nome(nome)` | Remove uma |
| `senhas_renovar(nome, pinNovo)` | Troca o PIN mantendo o nome |
| `senhas_bloquear(nome, bloquear)` | Bloqueio sem apagar |
| `senhas_limpar()` | Zera tudo (comando `LIMPAR`) |
| `senhas_total()` / `senhas_total_ativas()` | Contadores |
| `senhas_contem(pin)` | `true` só se existe **e ativa** (porta) |
| `senhas_consumirMudanca()` | Flag p/ atualizar o display |

NVS (`senhas`): `n`, `t%02d` (PIN), `n%02d` (nome), `a%02d` (ativa).
`isKey()` antes de ler: sem spam `NOT_FOUND` com NVS vazia.
