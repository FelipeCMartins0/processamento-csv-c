#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lista.h"
#ifdef _WIN32
#include <windows.h>
#endif


static void limpar_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


static void menu_concatenar(void) {
    char pasta[256];
    char saida[256];

    printf("\n--- CONCATENAR ARQUIVOS ---\n");
    printf("Pasta onde estao os arquivos (deixe vazio para diretorio atual): ");
    fgets(pasta, sizeof(pasta), stdin);
    pasta[strcspn(pasta, "\r\n")] = '\0';

    printf("Nome do arquivo de saida (ex: base_completa.csv): ");
    fgets(saida, sizeof(saida), stdin);
    saida[strcspn(saida, "\r\n")] = '\0';
    if (saida[0] == '\0') strcpy(saida, "base_completa.csv");

    concatenar_arquivos(pasta, saida);
}


static void menu_resumo(Lista *l) {
    char saida[256];

    printf("\n--- GERAR RESUMO POR TRIBUNAL ---\n");

    if (l->tamanho == 0) {
        printf("A lista esta vazia. Carregue um arquivo CSV primeiro (opcao 4).\n");
        return;
    }

    printf("Nome do arquivo de saida (ex: resumo.csv): ");
    fgets(saida, sizeof(saida), stdin);
    saida[strcspn(saida, "\r\n")] = '\0';
    if (saida[0] == '\0') strcpy(saida, "resumo.csv");

    gerar_resumo(l, saida);
}


static void menu_municipio(Lista *l) {
    char municipio[256];

    printf("\n--- FILTRAR POR MUNICIPIO ---\n");

    if (l->tamanho == 0) {
        printf("A lista esta vazia. Carregue um arquivo CSV primeiro (opcao 4).\n");
        return;
    }

    printf("Nome do municipio: ");
    fgets(municipio, sizeof(municipio), stdin);
    municipio[strcspn(municipio, "\r\n")] = '\0';

    if (municipio[0] == '\0') {
        printf("Nome de municipio invalido.\n");
        return;
    }

    filtrar_municipio(l, municipio);
}


static void menu_carregar(Lista *l) {
    char arquivo[512];

    printf("\n--- CARREGAR ARQUIVO CSV NA LISTA ---\n");
    printf("Nome do arquivo CSV: ");
    fgets(arquivo, sizeof(arquivo), stdin);
    arquivo[strcspn(arquivo, "\r\n")] = '\0';

    if (arquivo[0] == '\0') {
        printf("Nome invalido.\n");
        return;
    }

    
    lista_destruir(l);

    int n = carregar_csv(arquivo, l, 1 );
    printf("%d registro(s) carregado(s) na lista.\n", n);
}


int main(void) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    Lista l;
    lista_inicializar(&l);

    int opcao;

    printf("=========================================\n");
    printf("  TP01  Manipulacao de Arquivos CSV     \n");
    printf("  UCB  Algoritmos e Prog. Estruturada   \n");
    printf("=========================================\n");

    do {
        printf("\n--- MENU ---\n");
        printf("1. Concatenar arquivos CSV\n");
        printf("2. Gerar resumo por tribunal\n");
        printf("3. Filtrar por municipio\n");
        printf("4. Carregar arquivo CSV na lista\n");
        printf("0. Sair\n");
        printf("Opcao: ");

        if (scanf("%d", &opcao) != 1) opcao = -1;
        limpar_buffer();

        switch (opcao) {
            case 1: menu_concatenar();    break;
            case 2: menu_resumo(&l);      break;
            case 3: menu_municipio(&l);   break;
            case 4: menu_carregar(&l);    break;
            case 0: printf("Encerrando...\n"); break;
            default: printf("Opcao invalida.\n");
        }
    } while (opcao != 0);

    lista_destruir(&l);
    return 0;
}