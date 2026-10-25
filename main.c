#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#define MAX_REGISTROS   10000   /* Capacidade maxima de registros (2024 tem ~8712) */
#define TAM_ID          20      /* Tamanho do campo id_bdq                         */
#define TAM_FOCO_ID     50      /* Tamanho do campo foco_id (UUID)                 */
#define TAM_DATA        25      /* Tamanho do campo data_pas                       */
#define TAM_PAIS        20      /* Tamanho do campo pais                           */
#define TAM_ESTADO      30      /* Tamanho do campo estado                         */
#define TAM_MUNICIPIO   60      /* Tamanho do campo municipio                      */
#define TAM_BIOMA       30      /* Tamanho do campo bioma                          */
#define TAM_LINHA       512     /* Tamanho maximo de uma linha do CSV              */
#define TAM_CAMINHO     300     /* Tamanho maximo do caminho do arquivo            */

typedef struct {
    char id_bdq[TAM_ID];
    char foco_id[TAM_FOCO_ID];
    double lat;
    double lon;
    char data_pas[TAM_DATA];
    char pais[TAM_PAIS];
    char estado[TAM_ESTADO];
    char municipio[TAM_MUNICIPIO];
    char bioma[TAM_BIOMA];
} Foco;

typedef struct {
    unsigned long comparacoes;
    unsigned long trocas;
    double tempo_segundos;
} Metricas;

typedef struct {
    char nome[TAM_MUNICIPIO];
    int contagem;
} ItemContagem;

typedef void (*FuncaoSort)(Foco[], int, int (*)(const Foco*, const Foco*), Metricas*);

typedef struct {
    const char *nome;
    FuncaoSort funcao;
} AlgoritmoInfo;

Foco dados2023[MAX_REGISTROS];
Foco dados2024[MAX_REGISTROS];
int total2023 = 0;
int total2024 = 0;

Foco vetorTrabalho[MAX_REGISTROS];

/* Funcao: removerEspacos */
void removerEspacos(char *str) {
    int inicio = 0;
    int fim;
    int i, j;

    if (str == NULL || str[0] == '\0') {
        return;
    }

    while (str[inicio] == ' ' || str[inicio] == '\t' ||
           str[inicio] == '\r' || str[inicio] == '\n') {
        inicio++;
    }

    fim = (int)strlen(str) - 1;

    while (fim >= inicio && (str[fim] == ' ' || str[fim] == '\t' ||
           str[fim] == '\r' || str[fim] == '\n')) {
        fim--;
    }

    j = 0;
    for (i = inicio; i <= fim; i++) {
        str[j++] = str[i];
    }
    str[j] = '\0';
}

/* Funcao: lerCampoCSV */
void lerCampoCSV(const char *linha, int *pos, char *campo, int tamMax) {
    int i = 0;
    int p = *pos;

    while (linha[p] != ',' && linha[p] != '\0' && linha[p] != '\n' && i < tamMax - 1) {
        campo[i++] = linha[p++];
    }
    campo[i] = '\0';

    if (linha[p] == ',') {
        p++;
    }

    *pos = p;
    removerEspacos(campo);
}

/* ========================== LEITURA DO CSV ========================== */

int carregarCSV(const char *nomeArquivo, Foco vetor[], int *total) {
    FILE *arquivo;
    char linha[TAM_LINHA];
    char campoTemp[TAM_LINHA];
    int pos;
    int contadorLinhas = 0;

    arquivo = fopen(nomeArquivo, "r");
    if (arquivo == NULL) {
        printf("\n  [ERRO] Nao foi possivel abrir o arquivo: %s\n", nomeArquivo);
        return 0;
    }

    *total = 0;

    while (fgets(linha, TAM_LINHA, arquivo) != NULL) {
        contadorLinhas++;
        
        if (contadorLinhas == 1) {
            continue;
        }

        if (*total >= MAX_REGISTROS) {
            printf("  [AVISO] Limite de %d registros atingido.\n", MAX_REGISTROS);
            break;
        }

        pos = 0;
        lerCampoCSV(linha, &pos, vetor[*total].id_bdq, TAM_ID);
        lerCampoCSV(linha, &pos, vetor[*total].foco_id, TAM_FOCO_ID);

        lerCampoCSV(linha, &pos, campoTemp, TAM_LINHA);
        vetor[*total].lat = atof(campoTemp);

        lerCampoCSV(linha, &pos, campoTemp, TAM_LINHA);
        vetor[*total].lon = atof(campoTemp);

        lerCampoCSV(linha, &pos, vetor[*total].data_pas, TAM_DATA);
        lerCampoCSV(linha, &pos, vetor[*total].pais, TAM_PAIS);
        lerCampoCSV(linha, &pos, vetor[*total].estado, TAM_ESTADO);
        lerCampoCSV(linha, &pos, vetor[*total].municipio, TAM_MUNICIPIO);
        lerCampoCSV(linha, &pos, vetor[*total].bioma, TAM_BIOMA);

        (*total)++;
    }

    fclose(arquivo);
    printf("  Arquivo '%s' carregado com sucesso! (%d registros)\n", nomeArquivo, *total);
    return 1;
}

/* ========================== FUNCOES DE EXTRACAO E COMPARACAO ========================== */

const char* obterBioma(const Foco *f) {
    return f->bioma;
}

const char* obterMunicipio(const Foco *f) {
    return f->municipio;
}

/* Funcoes de comparacao para os algoritmos de ordenacao */
int compararPorData(const Foco *a, const Foco *b) {
    return strcmp(a->data_pas, b->data_pas);
}

int compararPorBioma(const Foco *a, const Foco *b) {
    return strcmp(a->bioma, b->bioma);
}

int compararPorMunicipio(const Foco *a, const Foco *b) {
    return strcmp(a->municipio, b->municipio);
}

int contarFrequencias(const Foco vetor[], int total,
                      const char* (*obterCampo)(const Foco*),
                      ItemContagem itens[], int maxItens) {
    int numItens = 0;
    int i, j, encontrado;

    for (i = 0; i < total; i++) {
        const char *chave = obterCampo(&vetor[i]);
        encontrado = 0;

        for (j = 0; j < numItens; j++) {
            if (strcmp(itens[j].nome, chave) == 0) {
                itens[j].contagem++;
                encontrado = 1;
                break;
            }
        }

        if (!encontrado && numItens < maxItens) {
            strncpy(itens[numItens].nome, chave, TAM_MUNICIPIO - 1);
            itens[numItens].nome[TAM_MUNICIPIO - 1] = '\0';
            itens[numItens].contagem = 1;
            numItens++;
        }
    }

    return numItens;
}

/* ========================== ALGORITMOS DE ORDENACAO ========================== */

void trocar(Foco *a, Foco *b) {
    Foco temp = *a;
    *a = *b;
    *b = temp;
}

/* ------------- BUBBLE SORT ------------- */
void bubbleSort(Foco vetor[], int n,
                int (*comparar)(const Foco*, const Foco*),
                Metricas *met) {
    int i, j, trocou;

    met->comparacoes = 0;
    met->trocas = 0;

    for (i = 0; i < n - 1; i++) {
        trocou = 0;
        for (j = 0; j < n - 1 - i; j++) {
            met->comparacoes++;
            if (comparar(&vetor[j], &vetor[j + 1]) > 0) {
                trocar(&vetor[j], &vetor[j + 1]);
                met->trocas++;
                trocou = 1;
            }
        }
        if (!trocou) {
            break;
        }
    }
}

/* ------------- QUICK SORT ------------- */
int particionar(Foco vetor[], int inicio, int fim,
                int (*comparar)(const Foco*, const Foco*),
                Metricas *met) {
    Foco pivo = vetor[fim];
    int i = inicio - 1;
    int j;

    for (j = inicio; j < fim; j++) {
        met->comparacoes++;
        if (comparar(&vetor[j], &pivo) <= 0) {
            i++;
            trocar(&vetor[i], &vetor[j]);
            met->trocas++;
        }
    }

    trocar(&vetor[i + 1], &vetor[fim]);
    met->trocas++;

    return i + 1;
}

void quickSortRecursivo(Foco vetor[], int inicio, int fim,
                        int (*comparar)(const Foco*, const Foco*),
                        Metricas *met) {
    int pivo;

    if (inicio < fim) {
        pivo = particionar(vetor, inicio, fim, comparar, met);
        quickSortRecursivo(vetor, inicio, pivo - 1, comparar, met);
        quickSortRecursivo(vetor, pivo + 1, fim, comparar, met);
    }
}

void quickSort(Foco vetor[], int n,
               int (*comparar)(const Foco*, const Foco*),
               Metricas *met) {
    met->comparacoes = 0;
    met->trocas = 0;

    if (n > 1) {
        quickSortRecursivo(vetor, 0, n - 1, comparar, met);
    }
}

static const AlgoritmoInfo ALGORITMOS[] = {
    {"Bubble Sort", bubbleSort},
    {"Quick Sort",  quickSort}
};
#define TOTAL_ALGORITMOS 2

void testarAlgoritmo(FuncaoSort funcao, const Foco fonte[], int total,
                     int (*comparar)(const Foco*, const Foco*),
                     Metricas *met) {
    int i;
    clock_t inicio, fim;

    for (i = 0; i < total; i++) {
        vetorTrabalho[i] = fonte[i];
    }

    inicio = clock();
    funcao(vetorTrabalho, total, comparar, met);
    fim = clock();

    met->tempo_segundos = (double)(fim - inicio) / CLOCKS_PER_SEC;
}

/* ========================== EXIBICAO DE DADOS ========================== */

void exibirCabecalhoTabela(void) {
    printf("\n  +------+---------------------+---------+-----------+--------------------------------+----------------+\n");
    printf("  | %-4s | %-19s | %-7s | %-9s | %-30s | %-14s |\n",
           "No.", "Data/Hora", "Lat", "Lon", "Municipio", "Bioma");
    printf("  +------+---------------------+---------+-----------+--------------------------------+----------------+\n");
}

void exibirRegistro(int num, const Foco *foco) {
    printf("  | %-4d | %-19s | %7.3f | %9.3f | %-30s | %-14s |\n",
           num, foco->data_pas, foco->lat, foco->lon,
           foco->municipio, foco->bioma);
}

void exibirRodapeTabela(void) {
    printf("  +------+---------------------+---------+-----------+--------------------------------+----------------+\n");
}

void exibirDadosPaginados(Foco vetor[], int total) {
    int pagina = 0;
    int registrosPorPagina = 20;
    int totalPaginas;
    int inicio, fim, i;
    char opcao;

    if (total == 0) {
        printf("\n  Nenhum dado para exibir.\n");
        return;
    }

    totalPaginas = (total + registrosPorPagina - 1) / registrosPorPagina;

    while (1) {
        inicio = pagina * registrosPorPagina;
        fim = inicio + registrosPorPagina;
        if (fim > total) {
            fim = total;
        }

        printf("\n  === Pagina %d de %d (Registros %d a %d de %d) ===\n",
               pagina + 1, totalPaginas, inicio + 1, fim, total);

        exibirCabecalhoTabela();
        for (i = inicio; i < fim; i++) {
            exibirRegistro(i + 1, &vetor[i]);
        }
        exibirRodapeTabela();

        printf("\n  [P] Proxima pagina | [A] Pagina anterior | [V] Voltar ao menu\n");
        printf("  Opcao: ");
        scanf(" %c", &opcao);

        if (opcao == 'P' || opcao == 'p') {
            if (pagina < totalPaginas - 1) pagina++;
            else printf("  Voce ja esta na ultima pagina.\n");
        } else if (opcao == 'A' || opcao == 'a') {
            if (pagina > 0) pagina--;
            else printf("  Voce ja esta na primeira pagina.\n");
        } else if (opcao == 'V' || opcao == 'v') {
            break;
        }
    }
}

/* ========================== RELATORIOS ========================== */

void exibirMetricas(const char *nomeAlgoritmo, const char *criterio,
                    const Metricas *met, int totalRegistros) {
    printf("\n  ====================================================\n");
    printf("  |         RELATORIO DE PERFORMANCE                 |\n");
    printf("  ====================================================\n");
    printf("  | Algoritmo:       %-30s |\n", nomeAlgoritmo);
    printf("  | Criterio:        %-30s |\n", criterio);
    printf("  | Total registros: %-30d |\n", totalRegistros);
    printf("  |--------------------------------------------------|\n");
    printf("  | Comparacoes:     %-30lu |\n", met->comparacoes);
    printf("  | Trocas/Moviment: %-30lu |\n", met->trocas);
    printf("  | Tempo (seg):     %-30.6f |\n", met->tempo_segundos);
    printf("  ====================================================\n");
}

void gerarRelatorioComparativo(Foco fonte[], int total,
                               int (*comparar)(const Foco*, const Foco*),
                               const char *criterio) {
    Metricas metricas[TOTAL_ALGORITMOS];
    int i;

    printf("\n  Gerando relatorio comparativo para %d registros...\n", total);
    printf("  Criterio: %s\n", criterio);
    printf("  Aguarde, executando algoritmos...\n");

    for (i = 0; i < TOTAL_ALGORITMOS; i++) {
        testarAlgoritmo(ALGORITMOS[i].funcao, fonte, total, comparar, &metricas[i]);
        printf("  [OK] %s concluido.\n", ALGORITMOS[i].nome);
    }

    printf("\n  ================================================================================\n");
    printf("  |           RELATORIO COMPARATIVO DE ALGORITMOS DE ORDENACAO                   |\n");
    printf("  |  Criterio: %-25s   Registros: %-10d              |\n", criterio, total);
    printf("  ================================================================================\n");
    printf("  | %-17s | %15s | %15s | %12s |\n",
           "Algoritmo", "Comparacoes", "Trocas/Mov.", "Tempo (s)");
    printf("  |-------------------|-----------------|-----------------|--------------|\n");

    for (i = 0; i < TOTAL_ALGORITMOS; i++) {
        printf("  | %-17s | %15lu | %15lu | %12.6f |\n",
               ALGORITMOS[i].nome, metricas[i].comparacoes, metricas[i].trocas,
               metricas[i].tempo_segundos);
    }

    printf("  ================================================================================\n");
}

void gerarRelatorioPorBioma(Foco vetor[], int total, const char *ano) {
    ItemContagem biomas[10];
    int numBiomas = contarFrequencias(vetor, total, obterBioma, biomas, 10);
    int i;

    printf("\n  ================================================\n");
    printf("  |   FOCOS DE INCENDIO POR BIOMA - %s          |\n", ano);
    printf("  ================================================\n");
    printf("  | %-25s | %10s | %5s |\n", "Bioma", "Quantidade", "%%");
    printf("  |---------------------------|------------|-------|\n");

    for (i = 0; i < numBiomas; i++) {
        double percentual = (total > 0) ? (biomas[i].contagem * 100.0 / total) : 0.0;
        printf("  | %-25s | %10d | %5.1f |\n", biomas[i].nome, biomas[i].contagem, percentual);
    }

    printf("  |---------------------------|------------|-------|\n");
    printf("  | %-25s | %10d | 100.0 |\n", "TOTAL", total);
    printf("  ================================================\n");
}

void gerarRelatorioTopMunicipios(Foco vetor[], int total, const char *ano) {
    ItemContagem municipios[700];
    int numMunicipios = contarFrequencias(vetor, total, obterMunicipio, municipios, 700);
    int i, j, maxIdx;
    int topN = 10;

    printf("\n  ================================================\n");
    printf("  |  TOP %d MUNICIPIOS COM MAIS FOCOS - %s     |\n", topN, ano);
    printf("  ================================================\n");
    printf("  | %3s | %-30s | %6s |\n", "Pos", "Municipio", "Focos");
    printf("  |-----|--------------------------------|--------|\n");

    /* Ordenacao parcial (Selection Sort) para obter os maiores */
    for (i = 0; i < topN && i < numMunicipios; i++) {
        maxIdx = i;
        for (j = i + 1; j < numMunicipios; j++) {
            if (municipios[j].contagem > municipios[maxIdx].contagem) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) {
            ItemContagem temp = municipios[i];
            municipios[i] = municipios[maxIdx];
            municipios[maxIdx] = temp;
        }

        printf("  | %3d | %-30s | %6d |\n", i + 1, municipios[i].nome, municipios[i].contagem);
    }

    printf("  ================================================\n");
    printf("  Total de municipios afetados: %d\n", numMunicipios);
}

void gerarRelatorioMensal(Foco vetor[], int total, const char *ano) {
    const char *meses[] = {"Janeiro", "Fevereiro", "Marco", "Abril",
                           "Maio", "Junho", "Julho", "Agosto",
                           "Setembro", "Outubro", "Novembro", "Dezembro"};
    int contMeses[12] = {0};
    int i, mes;
    char mesStr[3];
    int mesMaior = 0;

    for (i = 0; i < total; i++) {
        mesStr[0] = vetor[i].data_pas[5];
        mesStr[1] = vetor[i].data_pas[6];
        mesStr[2] = '\0';
        mes = atoi(mesStr);

        if (mes >= 1 && mes <= 12) {
            contMeses[mes - 1]++;
        }
    }

    for (i = 1; i < 12; i++) {
        if (contMeses[i] > contMeses[mesMaior]) {
            mesMaior = i;
        }
    }

    printf("\n  ================================================\n");
    printf("  |  FOCOS DE INCENDIO POR MES - %s              |\n", ano);
    printf("  ================================================\n");
    printf("  | %-12s | %10s | %-20s |\n", "Mes", "Focos", "Grafico");
    printf("  |--------------|------------|----------------------|\n");

    for (i = 0; i < 12; i++) {
        int barras = (contMeses[mesMaior] > 0) ? (contMeses[i] * 20) / contMeses[mesMaior] : 0;
        int b;

        printf("  | %-12s | %10d | ", meses[i], contMeses[i]);
        for (b = 0; b < barras; b++) printf("#");
        for (b = barras; b < 20; b++) printf(" ");
        printf(" |\n");
    }

    printf("  |--------------|------------|----------------------|\n");
    printf("  | %-12s | %10d |                      |\n", "TOTAL", total);
    printf("  ================================================\n");
    printf("  Mes com mais focos: %s (%d focos)\n", meses[mesMaior], contMeses[mesMaior]);
}

void buscarPorMunicipio(Foco vetor[], int total) {
    char municipioBusca[TAM_MUNICIPIO];
    int i;
    int encontrados = 0;

    printf("\n  Digite o nome do municipio (em MAIUSCULAS): ");
    scanf(" %[^\n]", municipioBusca);
    removerEspacos(municipioBusca);

    exibirCabecalhoTabela();

    for (i = 0; i < total; i++) {
        if (strcmp(vetor[i].municipio, municipioBusca) == 0) {
            encontrados++;
            exibirRegistro(encontrados, &vetor[i]);
        }
    }

    exibirRodapeTabela();

    if (encontrados == 0) {
        printf("  Nenhum registro encontrado para o municipio '%s'.\n", municipioBusca);
    } else {
        printf("  Total de registros encontrados: %d\n", encontrados);
    }
}

void compararAnos(void) {
    ItemContagem biomas2023[10], biomas2024[10];
    int nBiomas2023 = contarFrequencias(dados2023, total2023, obterBioma, biomas2023, 10);
    int nBiomas2024 = contarFrequencias(dados2024, total2024, obterBioma, biomas2024, 10);
    int i, j;
    double variacao;

    printf("\n  ====================================================\n");
    printf("  |     COMPARACAO DE FOCOS: 2023 vs 2024            |\n");
    printf("  |            Estado de Sao Paulo                   |\n");
    printf("  ====================================================\n\n");

    variacao = ((double)(total2024 - total2023) / total2023) * 100.0;
    printf("  Total de focos em 2023: %d\n", total2023);
    printf("  Total de focos em 2024: %d\n", total2024);
    printf("  Variacao geral: %+.1f%%\n\n", variacao);

    printf("  %-25s | %8s | %8s | %10s\n", "Bioma", "2023", "2024", "Variacao");
    printf("  --------------------------+----------+----------+----------\n");

    for (i = 0; i < nBiomas2023; i++) {
        int val2024 = 0;

        for (j = 0; j < nBiomas2024; j++) {
            if (strcmp(biomas2023[i].nome, biomas2024[j].nome) == 0) {
                val2024 = biomas2024[j].contagem;
                break;
            }
        }

        variacao = (biomas2023[i].contagem > 0)
            ? (((double)(val2024 - biomas2023[i].contagem) / biomas2023[i].contagem) * 100.0)
            : 0.0;

        printf("  %-25s | %8d | %8d | %+9.1f%%\n",
               biomas2023[i].nome, biomas2023[i].contagem, val2024, variacao);
    }
    printf("\n");
}

/* ========================== MENUS ========================== */

void exibirMenuPrincipal(void) {
    printf("\n");
    printf("  ====================================================\n");
    printf("  |     SISTEMA DE ANALISE DE FOCOS DE INCENDIO      |\n");
    printf("  |         Estado de Sao Paulo - INPE               |\n");
    printf("  ====================================================\n");
    printf("  |                                                  |\n");
    printf("  |  1. Visualizar dados (2023)                      |\n");
    printf("  |  2. Visualizar dados (2024)                      |\n");
    printf("  |  3. Ordenar e exibir dados                       |\n");
    printf("  |  4. Relatorio comparativo (Bubble vs Quick)      |\n");
    printf("  |  5. Relatorio por bioma                          |\n");
    printf("  |  6. Relatorio por municipio (Top 10)             |\n");
    printf("  |  7. Relatorio mensal                             |\n");
    printf("  |  8. Buscar por municipio                         |\n");
    printf("  |  9. Comparacao 2023 vs 2024                      |\n");
    printf("  |  0. Sair                                         |\n");
    printf("  |                                                  |\n");
    printf("  ====================================================\n");
    printf("  Escolha uma opcao: ");
}

/*
 * Menu de selecao de ano: apenas 2023 ou 2024 (opcao "Ambos" removida).
 */
int menuEscolherAno(Foco **vetor, int *total, char *anoStr) {
    int opcao;

    printf("\n  Escolha o ano dos dados:\n");
    printf("  1. 2023 (%d registros)\n", total2023);
    printf("  2. 2024 (%d registros)\n", total2024);
    printf("  Opcao: ");
    scanf("%d", &opcao);

    if (opcao == 1) {
        *vetor = dados2023;
        *total = total2023;
        strcpy(anoStr, "2023");
    } else if (opcao == 2) {
        *vetor = dados2024;
        *total = total2024;
        strcpy(anoStr, "2024");
    } else {
        printf("  Opcao invalida!\n");
        return 0;
    }

    return 1;
}

int menuEscolherCriterio(int (**comparar)(const Foco*, const Foco*), char *criterio) {
    int opcao;

    printf("\n  Escolha o criterio de ordenacao:\n");
    printf("  1. Data\n");
    printf("  2. Bioma\n");
    printf("  3. Municipio\n");
    printf("  Opcao: ");
    scanf("%d", &opcao);

    if (opcao == 1) {
        *comparar = compararPorData;
        strcpy(criterio, "Data");
    } else if (opcao == 2) {
        *comparar = compararPorBioma;
        strcpy(criterio, "Bioma");
    } else if (opcao == 3) {
        *comparar = compararPorMunicipio;
        strcpy(criterio, "Municipio");
    } else {
        printf("  Opcao invalida!\n");
        return 0;
    }

    return 1;
}

int menuEscolherAlgoritmo(void) {
    int opcao;

    printf("\n  Escolha o algoritmo de ordenacao:\n");
    printf("  1. Bubble Sort (Ordenacao por Bolha - O(n^2))\n");
    printf("  2. Quick Sort  (Ordenacao Rapida - O(n log n))\n");
    printf("  Opcao: ");
    scanf("%d", &opcao);

    if (opcao >= 1 && opcao <= TOTAL_ALGORITMOS) {
        return opcao - 1;
    }

    printf("  Opcao invalida!\n");
    return -1;
}

void executarOrdenacao(Foco fonte[], int total, int indiceAlgoritmo,
                       int (*comparar)(const Foco*, const Foco*),
                       const char *criterio) {
    Metricas met;
    char opcao;

    if (indiceAlgoritmo < 0 || indiceAlgoritmo >= TOTAL_ALGORITMOS) {
        return;
    }

    printf("\n  Ordenando %d registros com %s por %s...\n",
           total, ALGORITMOS[indiceAlgoritmo].nome, criterio);

    testarAlgoritmo(ALGORITMOS[indiceAlgoritmo].funcao, fonte, total, comparar, &met);

    printf("  Ordenacao concluida!\n");
    exibirMetricas(ALGORITMOS[indiceAlgoritmo].nome, criterio, &met, total);

    printf("\n  Deseja visualizar os dados ordenados? (S/N): ");
    scanf(" %c", &opcao);

    if (opcao == 'S' || opcao == 's') {
        exibirDadosPaginados(vetorTrabalho, total);
    }
}

/* ========================== FUNCAO PRINCIPAL ========================== */

int main(void) {
    int opcao;
    int dadosCarregados = 0;
    char caminhoArquivo[TAM_CAMINHO];
    Foco *vetorSelecionado;
    int totalSelecionado;
    char anoStr[20];
    char criterio[30];
    int indiceAlgoritmo;
    int (*comparar)(const Foco*, const Foco*);

    setlocale(LC_ALL, "");

    printf("\n");
    printf("  ====================================================\n");
    printf("  |                                                  |\n");
    printf("  |     APS - CIENCIA DA COMPUTACAO - 4o SEM         |\n");
    printf("  |                                                  |\n");
    printf("  |     Sistema de Analise de Focos de Incendio      |\n");
    printf("  |     Estado: Sao Paulo                            |\n");
    printf("  |     Dados: INPE (2023 e 2024)                    |\n");
    printf("  |                                                  |\n");
    printf("  ====================================================\n");

    printf("\n  Carregando dados...\n\n");

    printf("  Informe o caminho do arquivo CSV de 2023\n");
    printf("  (ou pressione ENTER para usar 'focos_br_sp_ref_2023.csv'): ");
    fgets(caminhoArquivo, TAM_CAMINHO, stdin);
    removerEspacos(caminhoArquivo);

    if (strlen(caminhoArquivo) == 0) {
        strcpy(caminhoArquivo, "focos_br_sp_ref_2023.csv");
    }

    if (carregarCSV(caminhoArquivo, dados2023, &total2023)) {
        dadosCarregados++;
    }

    printf("\n  Informe o caminho do arquivo CSV de 2024\n");
    printf("  (ou pressione ENTER para usar 'focos_br_sp_ref_2024.csv'): ");
    fgets(caminhoArquivo, TAM_CAMINHO, stdin);
    removerEspacos(caminhoArquivo);

    if (strlen(caminhoArquivo) == 0) {
        strcpy(caminhoArquivo, "focos_br_sp_ref_2024.csv");
    }

    if (carregarCSV(caminhoArquivo, dados2024, &total2024)) {
        dadosCarregados++;
    }

    if (dadosCarregados == 0) {
        printf("\n  [ERRO FATAL] Nenhum arquivo de dados foi carregado.\n");
        printf("  Pressione ENTER para sair...\n");
        getchar();
        return 1;
    }

    printf("\n  %d arquivo(s) carregado(s) com sucesso!\n", dadosCarregados);
    printf("  Total de registros: 2023 = %d | 2024 = %d\n", total2023, total2024);

    do {
        exibirMenuPrincipal();
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
                if (total2023 == 0) printf("\n  Dados de 2023 nao carregados!\n");
                else exibirDadosPaginados(dados2023, total2023);
                break;

            case 2:
                if (total2024 == 0) printf("\n  Dados de 2024 nao carregados!\n");
                else exibirDadosPaginados(dados2024, total2024);
                break;

            case 3:
                if (!menuEscolherAno(&vetorSelecionado, &totalSelecionado, anoStr)) break;
                if (!menuEscolherCriterio(&comparar, criterio)) break;
                indiceAlgoritmo = menuEscolherAlgoritmo();
                if (indiceAlgoritmo < 0) break;
                executarOrdenacao(vetorSelecionado, totalSelecionado,
                                  indiceAlgoritmo, comparar, criterio);
                break;

            case 4:
                if (!menuEscolherAno(&vetorSelecionado, &totalSelecionado, anoStr)) break;
                if (!menuEscolherCriterio(&comparar, criterio)) break;
                gerarRelatorioComparativo(vetorSelecionado, totalSelecionado,
                                          comparar, criterio);
                break;

            case 5:
                if (!menuEscolherAno(&vetorSelecionado, &totalSelecionado, anoStr)) break;
                gerarRelatorioPorBioma(vetorSelecionado, totalSelecionado, anoStr);
                break;

            case 6:
                if (!menuEscolherAno(&vetorSelecionado, &totalSelecionado, anoStr)) break;
                gerarRelatorioTopMunicipios(vetorSelecionado, totalSelecionado, anoStr);
                break;

            case 7:
                if (!menuEscolherAno(&vetorSelecionado, &totalSelecionado, anoStr)) break;
                gerarRelatorioMensal(vetorSelecionado, totalSelecionado, anoStr);
                break;

            case 8:
                if (!menuEscolherAno(&vetorSelecionado, &totalSelecionado, anoStr)) break;
                buscarPorMunicipio(vetorSelecionado, totalSelecionado);
                break;

            case 9:
                if (total2023 == 0 || total2024 == 0) {
                    printf("\n  Ambos os anos precisam estar carregados para comparacao!\n");
                } else {
                    compararAnos();
                }
                break;

            case 0:
                printf("\n  Encerrando o sistema...\n");
                printf("  Obrigado por utilizar o Sistema de Analise de Focos de Incendio!\n\n");
                break;

            default:
                printf("\n  Opcao invalida! Tente novamente.\n");
                break;
        }

    } while (opcao != 0);

    return 0;
}
