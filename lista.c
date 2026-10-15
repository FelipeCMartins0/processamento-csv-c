#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "lista.h"

void lista_inicializar(Lista *l) {
    l->inicio  = NULL;
    l->fim     = NULL;
    l->tamanho = 0;
}

int lista_inserir_linha(Lista *l, const char *linha) {
    size_t len = strlen(linha);
    No *novo = (No *)malloc(sizeof(No) + len);
    if (!novo) return 0;
    memcpy(novo->linha, linha, len + 1);
    novo->prox = NULL;
    if (!l->fim) {
        l->inicio = novo;
    } else {
        l->fim->prox = novo;
    }
    l->fim = novo;
    l->tamanho++;
    return 1;
}

void lista_destruir(Lista *l) {
    No *aux = l->inicio;
    while (aux) {
        No *p = aux->prox;
        free(aux);
        aux = p;
    }
    l->inicio  = NULL;
    l->fim     = NULL;
    l->tamanho = 0;
}

static void remover_aspas(char *s) {
    size_t len = strlen(s);
    if (len >= 2 && s[0] == '"' && s[len-1] == '"') {
        memmove(s, s+1, len-2);
        s[len-2] = '\0';
    }
}

void get_campo(const char *linha, int col, char *dest) {
    char buf[MAX_LINHA];
    strncpy(buf, linha, MAX_LINHA - 1);
    buf[MAX_LINHA - 1] = '\0';
    char delim = strchr(buf, ';') ? ';' : ',';
    int c = 0;
    char *ptr = buf;
    while (1) {
        char *sep = strchr(ptr, delim);
        if (c == col) {
            if (sep) *sep = '\0';
            strncpy(dest, ptr, MAX_CAMPO - 1);
            dest[MAX_CAMPO - 1] = '\0';
            remover_aspas(dest);
            return;
        }
        if (!sep) break;
        ptr = sep + 1;
        c++;
    }
    dest[0] = '\0';
}

void parsear_linha(const char *linha, char campos[MAX_COLS][MAX_CAMPO]) {
    int i;
    for (i = 0; i < MAX_COLS; i++)
        get_campo(linha, i, campos[i]);
}

static double safe_atof(const char *s) {
    if (!s || s[0] == '\0') return 0.0;
    return atof(s);
}

#define BUFFER_LEITURA (1 << 20)

int carregar_csv(const char *arquivo, Lista *l, int pular_cabecalho) {
    FILE *fp = fopen(arquivo, "r");
    if (!fp) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", arquivo);
        return 0;
    }

    char *bloco = (char *)malloc(BUFFER_LEITURA + 1);
    char *linha  = (char *)malloc(MAX_LINHA);
    if (!bloco || !linha) {
        free(bloco);
        free(linha);
        fclose(fp);
        return 0;
    }

    int inseridos = 0;
    int cabecalho_pulado = !pular_cabecalho;
    size_t sobra = 0;

    size_t lido;
    while ((lido = fread(bloco + sobra, 1, BUFFER_LEITURA - sobra, fp)) > 0) {
        size_t total = sobra + lido;
        bloco[total] = '\0';

        char *ptr = bloco;
        char *fim;

        while ((fim = memchr(ptr, '\n', (size_t)(bloco + total - ptr))) != NULL) {
            size_t len = (size_t)(fim - ptr);
            if (len > 0 && ptr[len - 1] == '\r') len--;

            if (!cabecalho_pulado) {
                cabecalho_pulado = 1;
                ptr = fim + 1;
                continue;
            }

            if (len > 0 && len < MAX_LINHA) {
                memcpy(linha, ptr, len);
                linha[len] = '\0';
                if (lista_inserir_linha(l, linha)) inseridos++;
            }
            ptr = fim + 1;
        }

        sobra = (size_t)(bloco + total - ptr);
        if (sobra > 0) memmove(bloco, ptr, sobra);
    }

    if (sobra > 0 && cabecalho_pulado) {
        bloco[sobra] = '\0';
        if (sobra > 0 && bloco[sobra-1] == '\r') bloco[--sobra] = '\0';
        if (sobra > 0 && sobra < MAX_LINHA) {
            if (lista_inserir_linha(l, bloco)) inseridos++;
        }
    }

    free(bloco);
    free(linha);
    fclose(fp);
    return inseridos;
}

static const char *CABECALHO =
    "sigla_tribunal;procedimento;ramo_justica;sigla_grau;uf_oj;municipio_oj;"
    "id_ultimo_oj;nome;mesano_cnm1;mesano_sent;casos_novos_2026;julgados_2026;"
    "prim_sent2026;suspensos_2026;dessobrestados_2026;cumprimento_meta1;"
    "distm2_a;julgm2_a;suspm2_a;cumprimento_meta2a;distm2_ant;julgm2_ant;"
    "suspm2_ant;desom2_ant;cumprimento_meta2ant;distm4_a;julgm4_a;suspm4_a;"
    "cumprimento_meta4a;distm4_b;julgm4_b;suspm4_b;cumprimento_meta4b\n";

void concatenar_arquivos(const char *pasta, const char *saida) {
#ifdef _WIN32
    FILE *fout = fopen(saida, "w");
    if (!fout) { fprintf(stderr, "Erro ao criar '%s'\n", saida); return; }
    fputs(CABECALHO, fout);

    char padrao[768];
    if (pasta && pasta[0] != '\0')
        snprintf(padrao, sizeof(padrao), "%s\\*.csv", pasta);
    else
        strncpy(padrao, "*.csv", sizeof(padrao) - 1);

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(padrao, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Nenhum CSV encontrado em '%s'\n", pasta ? pasta : ".");
        fclose(fout);
        return;
    }

    int total = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (strcmp(fd.cFileName, saida) == 0) continue;

        char caminho[768];
        if (pasta && pasta[0] != '\0')
            snprintf(caminho, sizeof(caminho), "%s\\%s", pasta, fd.cFileName);
        else
            strncpy(caminho, fd.cFileName, sizeof(caminho) - 1);

        FILE *fin = fopen(caminho, "r");
        if (!fin) { fprintf(stderr, "Aviso: nao abriu '%s'\n", caminho); continue; }

        char linha[MAX_LINHA];
        if (fgets(linha, sizeof(linha), fin) == NULL) { fclose(fin); continue; }

        int linhas = 0;
        while (fgets(linha, sizeof(linha), fin)) {
            if (linha[0] == '\n' || linha[0] == '\r') continue;
            fputs(linha, fout);
            size_t len = strlen(linha);
            if (len > 0 && linha[len-1] != '\n') fputc('\n', fout);
            linhas++;
            total++;
        }
        fclose(fin);
        printf("  -> %d linhas de '%s'\n", linhas, caminho);
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
    fclose(fout);
    printf("Concatenacao concluida: %d linhas em '%s'\n", total, saida);
#else
    FILE *fout = fopen(saida, "w");
    if (!fout) { fprintf(stderr, "Erro ao criar '%s'\n", saida); return; }
    fputs(CABECALHO, fout);
    printf("Digite os nomes dos arquivos CSV (linha vazia para terminar):\n");
    char nome[512], caminho[768];
    int total = 0;
    while (1) {
        printf("Arquivo: ");
        if (!fgets(nome, sizeof(nome), stdin)) break;
        nome[strcspn(nome, "\r\n")] = '\0';
        if (nome[0] == '\0') break;
        if (pasta && pasta[0] != '\0')
            snprintf(caminho, sizeof(caminho), "%s/%s", pasta, nome);
        else
            strncpy(caminho, nome, sizeof(caminho) - 1);
        FILE *fin = fopen(caminho, "r");
        if (!fin) { fprintf(stderr, "Aviso: nao abriu '%s'\n", caminho); continue; }
        char linha[MAX_LINHA];
        fgets(linha, sizeof(linha), fin);
        int linhas = 0;
        while (fgets(linha, sizeof(linha), fin)) {
            if (linha[0] == '\n' || linha[0] == '\r') continue;
            fputs(linha, fout);
            linhas++;
            total++;
        }
        fclose(fin);
        printf("  -> %d linhas de '%s'\n", linhas, caminho);
    }
    fclose(fout);
    printf("Concatenacao concluida: %d linhas em '%s'\n", total, saida);
#endif
}

#define COL_SIGLA        0
#define COL_CASOS_NOVOS 10
#define COL_JULGADOS    11
#define COL_SUSPENSOS   13
#define COL_DESSOBR     14
#define COL_DISTM2_A    16
#define COL_JULGM2_A    17
#define COL_SUSPM2_A    18
#define COL_DISTM2_ANT  20
#define COL_JULGM2_ANT  21
#define COL_SUSPM2_ANT  22
#define COL_DESOM2_ANT  23
#define COL_DISTM4_A    25
#define COL_JULGM4_A    26
#define COL_SUSPM4_A    27
#define COL_DISTM4_B    29
#define COL_JULGM4_B    30
#define COL_SUSPM4_B    31

typedef struct Resumo {
    char   sigla[MAX_CAMPO];
    double casos_novos, julgados, suspensos, dessobr;
    double distm2_a, julgm2_a, suspm2_a;
    double distm2_ant, julgm2_ant, suspm2_ant, desom2_ant;
    double distm4_a, julgm4_a, suspm4_a;
    double distm4_b, julgm4_b, suspm4_b;
    struct Resumo *prox;
} Resumo;

static Resumo *buscar_ou_criar(Resumo **lista, const char *sigla) {
    Resumo *r = *lista;
    while (r) {
        if (strcmp(r->sigla, sigla) == 0) return r;
        r = r->prox;
    }
    r = (Resumo *)calloc(1, sizeof(Resumo));
    if (!r) return NULL;
    strncpy(r->sigla, sigla, MAX_CAMPO - 1);
    r->prox = *lista;
    *lista = r;
    return r;
}

static void liberar_resumos(Resumo *r) {
    while (r) {
        Resumo *p = r->prox;
        free(r);
        r = p;
    }
}

void gerar_resumo(Lista *l, const char *saida) {
    Resumo *resumos = NULL;
    char tmp[MAX_CAMPO];

    No *no = l->inicio;
    while (no) {
        get_campo(no->linha, COL_SIGLA, tmp);
        Resumo *r = buscar_ou_criar(&resumos, tmp);
        if (!r) { no = no->prox; continue; }

        get_campo(no->linha, COL_CASOS_NOVOS, tmp); r->casos_novos += safe_atof(tmp);
        get_campo(no->linha, COL_JULGADOS,    tmp); r->julgados    += safe_atof(tmp);
        get_campo(no->linha, COL_SUSPENSOS,   tmp); r->suspensos   += safe_atof(tmp);
        get_campo(no->linha, COL_DESSOBR,     tmp); r->dessobr     += safe_atof(tmp);
        get_campo(no->linha, COL_DISTM2_A,    tmp); r->distm2_a    += safe_atof(tmp);
        get_campo(no->linha, COL_JULGM2_A,    tmp); r->julgm2_a    += safe_atof(tmp);
        get_campo(no->linha, COL_SUSPM2_A,    tmp); r->suspm2_a    += safe_atof(tmp);
        get_campo(no->linha, COL_DISTM2_ANT,  tmp); r->distm2_ant  += safe_atof(tmp);
        get_campo(no->linha, COL_JULGM2_ANT,  tmp); r->julgm2_ant  += safe_atof(tmp);
        get_campo(no->linha, COL_SUSPM2_ANT,  tmp); r->suspm2_ant  += safe_atof(tmp);
        get_campo(no->linha, COL_DESOM2_ANT,  tmp); r->desom2_ant  += safe_atof(tmp);
        get_campo(no->linha, COL_DISTM4_A,    tmp); r->distm4_a    += safe_atof(tmp);
        get_campo(no->linha, COL_JULGM4_A,    tmp); r->julgm4_a    += safe_atof(tmp);
        get_campo(no->linha, COL_SUSPM4_A,    tmp); r->suspm4_a    += safe_atof(tmp);
        get_campo(no->linha, COL_DISTM4_B,    tmp); r->distm4_b    += safe_atof(tmp);
        get_campo(no->linha, COL_JULGM4_B,    tmp); r->julgm4_b    += safe_atof(tmp);
        get_campo(no->linha, COL_SUSPM4_B,    tmp); r->suspm4_b    += safe_atof(tmp);
        no = no->prox;
    }

    FILE *fp = fopen(saida, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao criar '%s'\n", saida);
        liberar_resumos(resumos);
        return;
    }

    fprintf(fp, "sigla_tribunal;total_julgados_2026;Meta1;Meta2A;Meta2Ant;Meta4A;Meta4B\n");

    Resumo *r = resumos;
    while (r) {
        double den, meta1, meta2a, meta2ant, meta4a, meta4b;

        den   = r->casos_novos + r->dessobr - r->suspensos;
        meta1 = den != 0.0 ? (r->julgados / den) * 100.0 : 0.0;

        den    = r->distm2_a - r->suspm2_a;
        meta2a = den != 0.0 ? (r->julgm2_a / den) * (1000.0/7.0) : 0.0;

        den      = r->distm2_ant - r->suspm2_ant - r->desom2_ant;
        meta2ant = den != 0.0 ? (r->julgm2_ant / den) * 100.0 : 0.0;

        den    = r->distm4_a - r->suspm4_a;
        meta4a = den != 0.0 ? (r->julgm4_a / den) * 100.0 : 0.0;

        den    = r->distm4_b - r->suspm4_b;
        meta4b = den != 0.0 ? (r->julgm4_b / den) * 100.0 : 0.0;

        fprintf(fp, "%s;%.0f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
                r->sigla, r->julgados, meta1, meta2a, meta2ant, meta4a, meta4b);
        r = r->prox;
    }

    fclose(fp);
    printf("Resumo gerado em '%s'\n", saida);
    liberar_resumos(resumos);
}

static void to_upper(char *s) {
    while (*s) {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

void filtrar_municipio(Lista *l, const char *municipio) {
    char mun_upper[MAX_CAMPO];
    strncpy(mun_upper, municipio, MAX_CAMPO - 1);
    mun_upper[MAX_CAMPO - 1] = '\0';
    to_upper(mun_upper);

    char nome_arq[MAX_CAMPO + 4];
    snprintf(nome_arq, sizeof(nome_arq), "%s.csv", mun_upper);

    int encontrados = 0;
    No *no = l->inicio;
    char mun_no[MAX_CAMPO];
    while (no) {
        get_campo(no->linha, 5, mun_no);
        to_upper(mun_no);
        if (strcmp(mun_no, mun_upper) == 0) encontrados++;
        no = no->prox;
    }

    if (encontrados == 0) {
        printf("Nenhum registro encontrado para '%s'\n", mun_upper);
        return;
    }

    FILE *fp = fopen(nome_arq, "w");
    if (!fp) { fprintf(stderr, "Erro ao criar '%s'\n", nome_arq); return; }

    fputs(CABECALHO, fp);

    no = l->inicio;
    while (no) {
        get_campo(no->linha, 5, mun_no);
        to_upper(mun_no);
        if (strcmp(mun_no, mun_upper) == 0) {
            fputs(no->linha, fp);
            fputc('\n', fp);
        }
        no = no->prox;
    }

    fclose(fp);
    printf("%d registro(s) gravado(s) em '%s'\n", encontrados, nome_arq);
}
