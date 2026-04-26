/*
 * Trabalho Prático 1 - Produtor-Consumidor com Semáforos
 * Alunos: Pedro Henrique Ferreira Santos e Thiago Leonardo Oliveira Bertolino
 * Compilar: gcc -O2 -o sem_pc sem_produtor_consumidor.c -lpthread -lm
 *   N  = tamanho do buffer compartilhado
 *   Np = número de threads produtoras
 *   Nc = número de threads consumidoras
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>

#define M_PADRAO      100000   //valor padrão de M
#define MAX_NUMERO    10000000 

int M_TOTAL; //total de números a processar
int modo_silencioso = 0;
int  *buffer;          //vetor de memória compartilhada (tamanho N)
int   tam_buffer;      //N: capacidade do buffer

int   pos_escrita = 0; 
int   pos_leitura = 0; 

//Semáforos POSIX
sem_t sem_vazio;   // conta posições vazias
sem_t sem_cheio;   // conta posições ocupadas 
sem_t mutex_buf;   

// Controle de término
int total_consumido = 0;       // quantidade de números já processados
pthread_mutex_t mutex_total; 

#define MAX_REGISTROS (M_TOTAL * 3) /
int *ocupacao_historico;            
int  qtd_registros = 0;             // quantos registros foram feitos
pthread_mutex_t mutex_historico;    // protege o vetor de ocupação

//retorna o nível atual de ocupação do buffer.
int nivel_buffer_atual(void) {
    int valor;
    sem_getvalue(&sem_cheio, &valor); //sem_cheio == posições ocupadas
    return valor;
}

//registra a ocupação atual no histórico.
void registrar_ocupacao(void) {
    pthread_mutex_lock(&mutex_historico);
    if (qtd_registros < MAX_REGISTROS) {
        ocupacao_historico[qtd_registros] = nivel_buffer_atual();
        qtd_registros++;
    }
    pthread_mutex_unlock(&mutex_historico);
}

//Checa se um número é primo.
int eh_primo(long long num) {
    if (num < 2) return 0;
    if (num == 2) return 1;
    if (num % 2 == 0) return 0;
    for (long long i = 3; i * i <= num; i += 2) {
        if (num % i == 0) return 0;
    }
    return 1;
}

//Thread PRODUTORA
//Gera números aleatórios entre 1 e MAX_NUMERO e os coloca no buffer
//Continua produzindo enquanto o total consumido não atingir M_TOTAL

void *produtor(void *arg) {
    int id = *((int *)arg); // identificador da thread
    free(arg);

    //semente diferente por thread para evitar sequências iguais
    unsigned int semente = (unsigned int)(time(NULL)) ^ (unsigned int)(pthread_self());

    if (!modo_silencioso)
        printf("[Produtor %d] iniciando\n", id);

    while (1) {
        //verifica se já atingimos o limite de consumo antes de produzir
        pthread_mutex_lock(&mutex_total);
        int ja_terminou = (total_consumido >= M_TOTAL);
        pthread_mutex_unlock(&mutex_total);

        if (ja_terminou) break;

        //gera um número aleatório entre 1 e MAX_NUMERO
        long long numero = (long long)(rand_r(&semente) % MAX_NUMERO) + 1;

        //seção de entrada do produtor
        //aguarda haver pelo menos uma posição vazia
        sem_wait(&sem_vazio);
        pthread_mutex_lock(&mutex_total);
        ja_terminou = (total_consumido >= M_TOTAL);
        pthread_mutex_unlock(&mutex_total);

        if (ja_terminou) {
            // devolve o sem_vazio que acabou de consumir e sai
            sem_post(&sem_vazio);
            break;
        }
        //entra na região crítica do buffer
        sem_wait(&mutex_buf);

        //região crítica
        buffer[pos_escrita] = (int)numero;
        pos_escrita = (pos_escrita + 1) % tam_buffer; //buffer circular

        if (!modo_silencioso)
            printf("[Produtor %d] inseriu: %lld\n", id, numero);

        //seção de saída do produtor
        sem_post(&mutex_buf);  // libera acesso ao buffer
        sem_post(&sem_cheio);  // sinaliza mais uma posição ocupada
        registrar_ocupacao();  // grava nível do buffer no histórico
    }

    if (!modo_silencioso)
        printf("[Produtor %d] encerrando\n", id);
    return NULL;
}

//Thread CONSUMIDORA
//Retira números do buffer e verifica se são primos.
//Para quando o total consumido atingir M_TOTAL.
void *consumidor(void *arg) {
    int id = *((int *)arg);
    free(arg);

    if (!modo_silencioso)
        printf("[Consumidor %d] iniciando\n", id);

    while (1) {
        //verifica antecipadamente se já terminou
        pthread_mutex_lock(&mutex_total);
        int ja_terminou = (total_consumido >= M_TOTAL);
        pthread_mutex_unlock(&mutex_total);

        if (ja_terminou) break;

        //seção de entrada do consumidor
        //aguarda haver pelo menos um item no buffer
        sem_wait(&sem_cheio);

        //revalida após acordar
        pthread_mutex_lock(&mutex_total);
        ja_terminou = (total_consumido >= M_TOTAL);
        pthread_mutex_unlock(&mutex_total);

        if (ja_terminou) {
            sem_post(&sem_cheio); //devolve o token e sai
            break;
        }

        //entra na região crítica do buffer
        sem_wait(&mutex_buf);

        //região crítica
        long long numero = (long long)buffer[pos_leitura];
        pos_leitura = (pos_leitura + 1) % tam_buffer; //buffer circular

        //atualiza o contador global de consumidos
        pthread_mutex_lock(&mutex_total);
        total_consumido++;
        int meu_numero = total_consumido; //número de ordem deste item
        ja_terminou    = (total_consumido >= M_TOTAL);
        pthread_mutex_unlock(&mutex_total);

        //seção de saída do consumidor
        sem_post(&mutex_buf); //libera acesso ao buffer
        sem_post(&sem_vazio); //sinaliza mais uma posição vazia
        registrar_ocupacao(); //grava nível do buffer no histórico

        //verifica primalidade
        if (!modo_silencioso) {
            if (eh_primo(numero))
                printf("[Consumidor %d] #%d -> %lld: PRIMO\n",    id, meu_numero, numero);
            else
                printf("[Consumidor %d] #%d -> %lld: nao primo\n", id, meu_numero, numero);
        } else {
            eh_primo(numero);
        }

        if (ja_terminou) break;
    }

    if (!modo_silencioso)
        printf("[Consumidor %d] encerrando\n", id);
    return NULL;
}

//Salva o histórico de ocupação em arquivo CSV.
void salvar_historico(int n_buf, int np, int nc) {
    char nome_arquivo[128];
    snprintf(nome_arquivo, sizeof(nome_arquivo),
             "ocupacao_N%d_Np%d_Nc%d.csv", n_buf, np, nc);

    FILE *arq = fopen(nome_arquivo, "w");
    if (!arq) {
        perror("erro ao abrir arquivo de histórico");
        return;
    }

    fprintf(arq, "operacao,ocupacao\n");
    for (int i = 0; i < qtd_registros; i++) {
        fprintf(arq, "%d,%d\n", i + 1, ocupacao_historico[i]);
    }

    fclose(arq);
    fprintf(stderr, "[Main] histórico salvo em: %s\n", nome_arquivo);
}

//Programa principal
int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 6) {
        fprintf(stderr, "uso: %s <N> <Np> <Nc> [M] [--silencioso]\n", argv[0]);
        fprintf(stderr, "  N            = tamanho do buffer\n");
        fprintf(stderr, "  Np           = numero de threads produtoras\n");
        fprintf(stderr, "  Nc           = numero de threads consumidoras\n");
        fprintf(stderr, "  M            = quantidade a processar (padrao: %d)\n", M_PADRAO);
        fprintf(stderr, "  --silencioso = suprime saida por item (para benchmark)\n");
        return EXIT_FAILURE;
    }

    tam_buffer = atoi(argv[1]);
    int num_produtores   = atoi(argv[2]);
    int num_consumidores = atoi(argv[3]);

   //M = quantidade a processar
    M_TOTAL = M_PADRAO;
    int proximo_arg = 4;
    if (argc > 4 && strcmp(argv[4], "--silencioso") != 0) {
        M_TOTAL = atoi(argv[4]);
        proximo_arg = 5;
    }

    //flag opcional de modo silencioso
    if (argc > proximo_arg && strcmp(argv[proximo_arg], "--silencioso") == 0)
        modo_silencioso = 1;

    if (tam_buffer <= 0 || num_produtores <= 0 || num_consumidores <= 0) {
        fprintf(stderr, "todos os parâmetros devem ser positivos\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[Main] N=%d | Np=%d | Nc=%d | M=%d\n",
            tam_buffer, num_produtores, num_consumidores, M_TOTAL);

    //aloca o buffer compartilhado
    buffer = (int *)malloc(sizeof(int) * tam_buffer);
    if (!buffer) {
        perror("erro ao alocar buffer");
        return EXIT_FAILURE;
    }

    //aloca vetor de histórico de ocupação
    ocupacao_historico = (int *)malloc(sizeof(int) * MAX_REGISTROS);
    if (!ocupacao_historico) {
        perror("erro ao alocar historico");
        free(buffer);
        return EXIT_FAILURE;
    }

    //inicializa semáforos
    sem_init(&sem_vazio, 0, tam_buffer); //N posições livres no início
    sem_init(&sem_cheio, 0, 0);          //0 posições ocupadas no início
    sem_init(&mutex_buf, 0, 1);          //semáforo binário (mutex)

    //inicializa mutexes POSIX
    pthread_mutex_init(&mutex_total,     NULL);
    pthread_mutex_init(&mutex_historico, NULL);

    //cria arrays de threads
    pthread_t *tid_produtores   = malloc(sizeof(pthread_t) * num_produtores);
    pthread_t *tid_consumidores = malloc(sizeof(pthread_t) * num_consumidores);

    //marca o tempo de início
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    //lança threads produtoras
    for (int i = 0; i < num_produtores; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&tid_produtores[i], NULL, produtor, id) != 0) {
            perror("erro ao criar thread produtora");
            return EXIT_FAILURE;
        }
    }

    //lança threads consumidoras
    for (int i = 0; i < num_consumidores; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&tid_consumidores[i], NULL, consumidor, id) != 0) {
            perror("erro ao criar thread consumidora");
            return EXIT_FAILURE;
        }
    }

    //aguarda todas as threads produtoras terminarem
    for (int i = 0; i < num_produtores; i++) {
        pthread_join(tid_produtores[i], NULL);
    }

    for (int i = 0; i < num_consumidores; i++) {
        sem_post(&sem_cheio);
    }

    //aguarda todas as threads consumidoras terminarem
    for (int i = 0; i < num_consumidores; i++) {
        pthread_join(tid_consumidores[i], NULL);
    }

    //marca o tempo de fim
    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo_seg = (fim.tv_sec  - inicio.tv_sec) +
                       (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    fprintf(stderr, "[Main] total consumido: %d\n", total_consumido);

    //imprime o tempo para o script capturar
    fprintf(stderr, "TEMPO_SEG=%.6f\n", tempo_seg);

    //salva histórico de ocupação em CSV
    salvar_historico(tam_buffer, num_produtores, num_consumidores);

    //libera recursos/
    sem_destroy(&sem_vazio);
    sem_destroy(&sem_cheio);
    sem_destroy(&mutex_buf);
    pthread_mutex_destroy(&mutex_total);
    pthread_mutex_destroy(&mutex_historico);

    free(buffer);
    free(ocupacao_historico);
    free(tid_produtores);
    free(tid_consumidores);

    fprintf(stderr, "[Main] programa finalizado\n");
    return EXIT_SUCCESS;
}
