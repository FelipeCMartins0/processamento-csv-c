#ifndef LISTA_H
#define LISTA_H

#define MAX_CAMPO  256
#define MAX_COLS    33
#define MAX_LINHA  2048

typedef struct No {
    struct No *prox;
    char linha[1];
} No;

typedef struct {
    No *inicio;
    No *fim;
    int tamanho;
} Lista;

void lista_inicializar(Lista *l);
int  lista_inserir_linha(Lista *l, const char *linha);
void lista_destruir(Lista *l);

void concatenar_arquivos(const char *pasta, const char *saida);
void gerar_resumo(Lista *l, const char *saida);
void filtrar_municipio(Lista *l, const char *municipio);

int  carregar_csv(const char *arquivo, Lista *l, int pular_cabecalho);
void parsear_linha(const char *linha, char campos[MAX_COLS][MAX_CAMPO]);
void get_campo(const char *linha, int col, char *dest);

#endif
