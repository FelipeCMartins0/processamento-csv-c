#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
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
    "sigla_tribunal,procedimento,ramo_justica,sigla_grau,uf_oj,municipio_oj,"
    "id_ultimo_oj,nome,mesano_cnm1,mesano_sent,casos_novos_2026,julgados_2026,"
    "prim_sent2026,suspensos_2026,dessobrestados_2026,cumprimento_meta1,"
    "distm2_a,julgm2_a,suspm2_a,cumprimento_meta2a,distm2_ant,julgm2_ant,"
    "suspm2_ant,desom2_ant,cumprimento_meta2ant,distm4_a,julgm4_a,suspm4_a,"
    "cumprimento_meta4a,distm4_b,julgm4_b,suspm4_b,cumprimento_meta4b\n";

static int eh_csv(const char *nome) {
    size_t n = strlen(nome);
    if (n < 4) return 0;
    const char *ext = nome + n - 4;
    return ext[0] == '.' &&
           tolower((unsigned char)ext[1]) == 'c' &&
           tolower((unsigned char)ext[2]) == 's' &&
           tolower((unsigned char)ext[3]) == 'v';
}

void concatenar_arquivos(const char *pasta, const char *saida) {
    const char *dir = (pasta && pasta[0] != '\0') ? pasta : ".";

    DIR *d = opendir(dir);
    if (!d) { fprintf(stderr, "Erro: nao foi possivel abrir a pasta '%s'\n", dir); return; }

    FILE *fout = fopen(saida, "w");
    if (!fout) { fprintf(stderr, "Erro ao criar '%s'\n", saida); closedir(d); return; }
    fputs(CABECALHO, fout);

    int total = 0, arquivos = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!eh_csv(ent->d_name)) continue;
        if (strcmp(ent->d_name, saida) == 0) continue;

        char caminho[768];
        snprintf(caminho, sizeof(caminho), "%s/%s", dir, ent->d_name);

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
        }
        fclose(fin);
        total += linhas;
        arquivos++;
        printf("  -> %d linhas de '%s'\n", linhas, caminho);
    }

    closedir(d);
    fclose(fout);

    if (arquivos == 0)
        printf("Nenhum CSV encontrado em '%s'\n", dir);
    else
        printf("Concatenacao concluida: %d arquivo(s), %d linhas em '%s'\n", arquivos, total, saida);
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
    Resumo *ant = NULL, *r = *lista;
    while (r && strcmp(r->sigla, sigla) < 0) {
        ant = r;
        r = r->prox;
    }
    if (r && strcmp(r->sigla, sigla) == 0) return r;

    Resumo *novo = (Resumo *)calloc(1, sizeof(Resumo));
    if (!novo) return NULL;
    strncpy(novo->sigla, sigla, MAX_CAMPO - 1);
    novo->prox = r;
    if (ant) ant->prox = novo;
    else     *lista = novo;
    return novo;
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

static void normalizar(const char *src, char *dst, size_t tam) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < tam - 1; i++) {
        unsigned char c = (unsigned char)src[i];

        if (c == 0xC3 && src[i+1] != '\0') {
            unsigned char b = (unsigned char)src[++i];
            if (b >= 0xA0) b -= 0x20;
            char base;
            if      (b >= 0x80 && b <= 0x85) base = 'A';
            else if (b == 0x87)              base = 'C';
            else if (b >= 0x88 && b <= 0x8B) base = 'E';
            else if (b >= 0x8C && b <= 0x8F) base = 'I';
            else if (b == 0x91)              base = 'N';
            else if (b >= 0x92 && b <= 0x96) base = 'O';
            else if (b >= 0x99 && b <= 0x9C) base = 'U';
            else continue;
            dst[j++] = base;
        } else {
            dst[j++] = (char)toupper(c);
        }
    }
    dst[j] = '\0';
}

void filtrar_municipio(Lista *l, const char *municipio) {
    char mun_upper[MAX_CAMPO];
    normalizar(municipio, mun_upper, sizeof(mun_upper));

    char nome_arq[MAX_CAMPO + 4];
    snprintf(nome_arq, sizeof(nome_arq), "%s.csv", mun_upper);

    int encontrados = 0;
    No *no = l->inicio;
    char campo[MAX_CAMPO], mun_no[MAX_CAMPO];
    while (no) {
        get_campo(no->linha, 5, campo);
        normalizar(campo, mun_no, sizeof(mun_no));
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
        get_campo(no->linha, 5, campo);
        normalizar(campo, mun_no, sizeof(mun_no));
        if (strcmp(mun_no, mun_upper) == 0) {
            fputs(no->linha, fp);
            fputc('\n', fp);
        }
        no = no->prox;
    }

    fclose(fp);
    printf("%d registro(s) gravado(s) em '%s'\n", encontrados, nome_arq);
}