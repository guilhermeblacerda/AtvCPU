#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_TAREFAS 100
#define MAX_NOME 50

typedef struct {
    char nome[MAX_NOME];
    int periodo;
    int prazo;
    int rajada;
    int rajada_restante;
    int proxima_chegada;
    int prazo_absoluto;
    int execucoes_completas;
    int prazos_perdidos;
    bool ativa;
} Tarefa;

typedef struct {
    Tarefa tarefas[MAX_TAREFAS];
    int num_tarefas;
    int tempo_total;
    char login[10];
} ConfiguracaoEscalonador;

#endif