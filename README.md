# Processamento de Dados da Justiça Eleitoral em C

Programa em C que une os arquivos CSV de metas dos 27 Tribunais Regionais Eleitorais (TREs), carrega os registros em uma lista encadeada e gera um resumo de indicadores por tribunal e filtros por município.

Trabalho prático da disciplina de **Estrutura de Dados** — Engenharia de Software, Universidade Católica de Brasília (2026/1).

## Funcionalidades

| Opção | O que faz |
|---|---|
| 1. Concatenar arquivos CSV | Lê todos os `.csv` de uma pasta e junta em um único arquivo, com um só cabeçalho |
| 2. Gerar resumo por tribunal | Soma os dados de cada tribunal e calcula as Metas 1, 2A, 2Ant, 4A e 4B, salvando em `resumo.csv`, em ordem alfabética |
| 3. Filtrar por município | Gera um CSV só com os registros do município informado, ignorando acentos e maiúsculas (`brasilia`, `Brasília` e `BRASÍLIA` geram `BRASILIA.csv`) |
| 4. Carregar arquivo CSV na lista | Lê o arquivo para a memória, em uma lista encadeada |

## Base de dados

- 27 arquivos CSV, um por TRE, na pasta `Base de Dados/`
- **205.122 registros** no total, com 33 colunas cada
- `TratamentoNULL.R`: script em R usado antes do programa para trocar valores vazios (NA) das colunas numéricas por 0

## Como compilar e rodar

Requisito: um compilador C (GCC ou MinGW no Windows).

```bash
gcc main.c lista.c -o tp1
./tp1
```

No Windows, rode `tp1.exe`.

**Fluxo de uso:**

1. Opção **1** para gerar a base completa (ex.: `base_completa.csv`)
2. Opção **4** para carregar a base completa na lista
3. Opções **2** e **3** para gerar o resumo e os filtros

## Como funciona

- **Lista encadeada com alocação dinâmica:** cada nó guarda uma linha do CSV. O nó é alocado com o tamanho exato da linha (`malloc(sizeof(No) + tamanho)`), sem desperdiçar memória com buffers fixos.
- **Inserção em O(1):** a lista guarda ponteiros para o início e o fim, então inserir no final não exige percorrer a lista.
- **Leitura em blocos de 1 MB:** o arquivo é lido com `fread` em blocos, e não linha a linha, o que deixa o carregamento da base inteira rápido (poucos segundos).
- **Resumo por tribunal:** uma segunda lista encadeada acumula os valores de cada tribunal. Quando aparece uma sigla nova, o nó é inserido já na posição certa, mantendo a lista em ordem alfabética.
- **Busca sem acento:** o filtro converte o texto para maiúsculas e troca as letras acentuadas em UTF-8 (2 bytes) pela letra sem acento antes de comparar.
- **Portável:** a leitura da pasta usa `dirent.h`, que funciona no Linux, no macOS e no Windows (MinGW).

## Estrutura do projeto

```
├── Base de Dados/     # CSVs dos 27 TREs + script de tratamento em R
├── lista.h            # structs e declarações das funções
├── lista.c            # lista encadeada, leitura dos CSVs, resumo e filtro
└── main.c             # menu
```
