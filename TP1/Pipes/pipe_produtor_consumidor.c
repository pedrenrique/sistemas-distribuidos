/*
 * Trabalho Prático 1 - Parte 2: Produtor-Consumidor com Pipes
 * Sistemas Distribuídos - CEFET-MG
 * Alunos: Pedro Henrique Ferreira Santos e Thiago Leonardo Oliveira Bertolino
 * Compilar: gcc -o pipe_pc pipe_produtor_consumidor.c
 * Uso:      ./pipe_pc <quantidade_de_numeros>
 * Exemplo:  ./pipe_pc 1000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define TAM_MSG 20

/* checa se o numero e primo */
int eh_primo(long long num) {
    if (num < 2) return 0;
    if (num == 2) return 1;
    if (num % 2 == 0) return 0;
    for (long long i = 3; i * i <= num; i += 2) {
        if (num % i == 0) return 0;
    }
    return 1;
}

/* produtor: gera os numeros e manda pelo pipe */
void produtor(int fd_escrita, int quantidade) {
    char buf[TAM_MSG];
    long long n = 1;
    int delta;

    srand((unsigned int)time(NULL));
    printf("[Produtor] vai gerar %d numeros\n", quantidade);

    for (int i = 0; i < quantidade; i++) {
        if (i == 0) {
            n = 1;
        } else {
            delta = (rand() % 100) + 1;
            n = n + delta;
        }

        memset(buf, 0, TAM_MSG);
        snprintf(buf, TAM_MSG, "%lld", n);
        printf("[Produtor] enviando: %s\n", buf);

        if (write(fd_escrita, buf, TAM_MSG) != TAM_MSG) {
            perror("produtor: erro na escrita do pipe");
            close(fd_escrita);
            exit(EXIT_FAILURE);
        }
    }

    /* manda 0 pra avisar que acabou */
    memset(buf, 0, TAM_MSG);
    snprintf(buf, TAM_MSG, "0");
    write(fd_escrita, buf, TAM_MSG);

    close(fd_escrita);
    printf("[Produtor] terminou\n");
}

/* consumidor: le os numeros do pipe e verifica se sao primos */
void consumidor(int fd_leitura) {
    char buf[TAM_MSG];
    long long n;
    int cont = 0;
    ssize_t lido;

    printf("[Consumidor] esperando numeros...\n");

    while (1) {
        lido = read(fd_leitura, buf, TAM_MSG);

        if (lido <= 0) break;

        buf[TAM_MSG - 1] = '\0';
        n = atoll(buf);

        if (n == 0) {
            printf("[Consumidor] recebeu sinal de fim\n");
            break;
        }

        cont++;
        if (eh_primo(n))
            printf("[Consumidor] #%d -> %lld: PRIMO\n", cont, n);
        else
            printf("[Consumidor] #%d -> %lld: nao primo\n", cont, n);
    }

    close(fd_leitura);
    printf("[Consumidor] total processado: %d\n", cont);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <quantidade>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int quantidade = atoi(argv[1]);
    if (quantidade <= 0) {
        fprintf(stderr, "quantidade tem que ser positiva\n");
        return EXIT_FAILURE;
    }

    int fd[2]; /* fd[0] leitura, fd[1] escrita */

    if (pipe(fd) == -1) {
        perror("erro ao criar pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("erro no fork");
        return EXIT_FAILURE;

    } else if (pid == 0) {
        /* filho vira o consumidor */
        close(fd[1]);
        consumidor(fd[0]);
        exit(EXIT_SUCCESS);

    } else {
        /* pai vira o produtor */
        close(fd[0]);
        produtor(fd[1], quantidade);

        int status;
        waitpid(pid, &status, 0);
        printf("[Main] programa finalizado\n");
    }

    return EXIT_SUCCESS;
}