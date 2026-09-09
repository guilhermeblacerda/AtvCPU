#include "escalonador.h"

bool ler_configuracao(const char* nome_arquivo, ConfiguracaoEscalonador* config) {
    FILE* arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        fprintf(stderr, "Erro: Arquivo '%s' inexistente ou ilegível\n", nome_arquivo);
        return false;
    }
    
    if (fscanf(arquivo, "%d\n", &config->tempo_total) != 1) {
        fprintf(stderr, "Erro: Arquivo malformado - tempo total inválido\n");
        fclose(arquivo);
        return false;
    }
    
    if (config->tempo_total <= 0) {
        fprintf(stderr, "Erro: Tempo total deve ser positivo\n");
        fclose(arquivo);
        return false;
    }
    
    config->num_tarefas = 0;
    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == '\n') continue;
        
        Tarefa* tarefa = &config->tarefas[config->num_tarefas];
        char nome[MAX_NOME];
        int periodo, prazo, rajada;
        
        if (sscanf(linha, "%s %d %d %d", nome, &periodo, &prazo, &rajada) != 4) {
            fprintf(stderr, "Erro: Arquivo malformado - campos faltando na linha %d\n", 
                    config->num_tarefas + 2);
            fclose(arquivo);
            return false;
        }
        
        if (periodo <= 0 || prazo <= 0 || rajada <= 0) {
            fprintf(stderr, "Erro: Valores devem ser positivos (tarefa: %s)\n", nome);
            fclose(arquivo);
            return false;
        }
        
        if (prazo > periodo) {
            fprintf(stderr, "Erro: Tarefa %s viola P ≥ D (%d > %d)\n", 
                    nome, prazo, periodo);
            fclose(arquivo);
            return false;
        }
        
        if (rajada > prazo) {
            fprintf(stderr, "Erro: Tarefa %s viola D ≥ C (%d > %d)\n", 
                    nome, rajada, prazo);
            fclose(arquivo);
            return false;
        }
        
        strcpy(tarefa->nome, nome);
        tarefa->periodo = periodo;
        tarefa->prazo = prazo;
        tarefa->rajada = rajada;
        tarefa->rajada_restante = rajada;
        tarefa->proxima_chegada = 0;
        tarefa->prazo_absoluto = prazo;
        tarefa->execucoes_completas = 0;
        tarefa->prazos_perdidos = 0;
        tarefa->ativa = true;
        
        config->num_tarefas++;
        
        if (config->num_tarefas >= MAX_TAREFAS) {
            fprintf(stderr, "Erro: Número máximo de tarefas excedido (%d)\n", MAX_TAREFAS);
            fclose(arquivo);
            return false;
        }
    }
    
    fclose(arquivo);
    return true;
}

int prioridade_rate(Tarefa* tarefa) {
    return tarefa->periodo;
}

void escrever_saida(const char* algoritmo, ConfiguracaoEscalonador* config,
                   char* historico[], int tempos[], char status[], int num_eventos) {
    char nome_arquivo[50];
    sprintf(nome_arquivo, "%s_%s.out", algoritmo, config->login);
    
    FILE* saida = fopen(nome_arquivo, "w");
    if (!saida) {
        fprintf(stderr, "Erro: Não foi possível criar %s\n", nome_arquivo);
        return;
    }
    
    fprintf(saida, "EXECUTION BY %s\n", algoritmo);
    
    int i;
    for (i = 0; i < num_eventos; i++) {
        if (strcmp(historico[i], "idle") == 0) {
            fprintf(saida, "idle for %d units\n", tempos[i]);
        } else {
            fprintf(saida, "[%s] for %d units - %c\n", historico[i], tempos[i], status[i]);
        }
    }
    
    fprintf(saida, "\nLOST DEADLINES\n");
    for (i = 0; i < config->num_tarefas; i++) {
        fprintf(saida, "[%s] %d\n", config->tarefas[i].nome, config->tarefas[i].prazos_perdidos);
    }
    
    fprintf(saida, "\nCOMPLETE EXECUTION\n");
    for (i = 0; i < config->num_tarefas; i++) {
        fprintf(saida, "[%s] %d\n", config->tarefas[i].nome, config->tarefas[i].execucoes_completas);
    }
    
    fprintf(saida, "\nKILLED\n");
    for (i = 0; i < config->num_tarefas; i++) {
        int morta = (config->tarefas[i].ativa && config->tarefas[i].rajada_restante > 0) ? 1 : 0;
        fprintf(saida, "[%s] %d\n", config->tarefas[i].nome, morta);
    }
    
    fclose(saida);
}

void simular_rate(ConfiguracaoEscalonador* config) {
    int tempo_atual = 0;
    Tarefa* tarefa_atual = NULL;
    int execucao_atual = 0;
    char* historico[1000];
    int tempos[1000];
    char status[1000];
    int num_eventos = 0;
    bool idle_em_andamento = false;
    
    while (tempo_atual < config->tempo_total) {
        for (int i = 0; i < config->num_tarefas; i++) {
            Tarefa* tarefa = &config->tarefas[i];
            if (tempo_atual >= tarefa->proxima_chegada) {
                if (!tarefa->ativa) {
                    tarefa->ativa = true;
                    tarefa->rajada_restante = tarefa->rajada;
                    tarefa->prazo_absoluto = tempo_atual + tarefa->prazo;
                }
                tarefa->proxima_chegada += tarefa->periodo;
            }
        }
        
        for (int i = 0; i < config->num_tarefas; i++) {
            Tarefa* tarefa = &config->tarefas[i];
            if (tarefa->ativa && tempo_atual >= tarefa->prazo_absoluto && 
                tarefa->rajada_restante > 0) {
                if (tarefa_atual == tarefa && execucao_atual > 0) {
                    historico[num_eventos] = strdup(tarefa->nome);
                    tempos[num_eventos] = execucao_atual;
                    status[num_eventos] = 'L';
                    num_eventos++;
                    execucao_atual = 0;
                    tarefa_atual = NULL;
                    idle_em_andamento = false;
                }
                tarefa->prazos_perdidos++;
                tarefa->ativa = false;
                tarefa->rajada_restante = 0;
            }
        }
        
        Tarefa* tarefa_selecionada = NULL;
        int melhor_prioridade = -1;
        
        for (int i = 0; i < config->num_tarefas; i++) {
            Tarefa* tarefa = &config->tarefas[i];
            if (tarefa->ativa && tarefa->rajada_restante > 0) {
                int prioridade = tarefa->periodo;
                if (tarefa_selecionada == NULL || prioridade < melhor_prioridade || 
                    (prioridade == melhor_prioridade && i < tarefa_selecionada - config->tarefas)) {
                    tarefa_selecionada = tarefa;
                    melhor_prioridade = prioridade;
                }
            }
        }
        
        if (tarefa_selecionada != NULL) {
            if (tarefa_atual != tarefa_selecionada) {
                if (tarefa_atual != NULL && execucao_atual > 0) {
                    historico[num_eventos] = strdup(tarefa_atual->nome);
                    tempos[num_eventos] = execucao_atual;
                    status[num_eventos] = (tarefa_atual->rajada_restante > 0) ? 'H' : 'F';
                    num_eventos++;
                    execucao_atual = 0;
                }
                if (idle_em_andamento && execucao_atual > 0) {
                    historico[num_eventos] = strdup("idle");
                    tempos[num_eventos] = execucao_atual;
                    status[num_eventos] = 'I';
                    num_eventos++;
                    execucao_atual = 0;
                    idle_em_andamento = false;
                }
                tarefa_atual = tarefa_selecionada;
            }
            
            tarefa_selecionada->rajada_restante--;
            execucao_atual++;
            
            if (tarefa_selecionada->rajada_restante == 0) {
                tarefa_selecionada->execucoes_completas++;
                tarefa_selecionada->ativa = false;
                historico[num_eventos] = strdup(tarefa_selecionada->nome);
                tempos[num_eventos] = execucao_atual;
                status[num_eventos] = 'F';
                num_eventos++;
                execucao_atual = 0;
                tarefa_atual = NULL;
            }
        } else {
            if (tarefa_atual != NULL && execucao_atual > 0) {
                historico[num_eventos] = strdup(tarefa_atual->nome);
                tempos[num_eventos] = execucao_atual;
                status[num_eventos] = (tarefa_atual->rajada_restante > 0) ? 'H' : 'F';
                num_eventos++;
                execucao_atual = 0;
                tarefa_atual = NULL;
            }
            
            if (!idle_em_andamento) {
                idle_em_andamento = true;
                execucao_atual = 1;
            } else {
                execucao_atual++;
            }
        }
        
        tempo_atual++;
    }
    
    if (tarefa_atual != NULL && execucao_atual > 0) {
        historico[num_eventos] = strdup(tarefa_atual->nome);
        tempos[num_eventos] = execucao_atual;
        status[num_eventos] = (tarefa_atual->rajada_restante > 0) ? 'H' : 'F';
        num_eventos++;
    } else if (idle_em_andamento && execucao_atual > 0) {
        historico[num_eventos] = strdup("idle");
        tempos[num_eventos] = execucao_atual;
        status[num_eventos] = 'I';
        num_eventos++;
    }
    
    escrever_saida("RATE", config, historico, tempos, status, num_eventos);
}

int prioridade_edf(Tarefa* tarefa) {
    return tarefa->prazo_absoluto;
}

void simular_edf(ConfiguracaoEscalonador* config) {
    int tempo_atual = 0;
    Tarefa* tarefa_atual = NULL;
    int execucao_atual = 0;
    char* historico[1000];
    int tempos[1000];
    char status[1000];
    int num_eventos = 0;
    bool idle_em_andamento = false;
    
    while (tempo_atual < config->tempo_total) {
        for (int i = 0; i < config->num_tarefas; i++) {
            Tarefa* tarefa = &config->tarefas[i];
            if (tempo_atual >= tarefa->proxima_chegada) {
                if (!tarefa->ativa) {
                    tarefa->ativa = true;
                    tarefa->rajada_restante = tarefa->rajada;
                    tarefa->prazo_absoluto = tempo_atual + tarefa->prazo;
                }
                tarefa->proxima_chegada += tarefa->periodo;
            }
        }
        
        for (int i = 0; i < config->num_tarefas; i++) {
            Tarefa* tarefa = &config->tarefas[i];
            if (tarefa->ativa && tempo_atual >= tarefa->prazo_absoluto && 
                tarefa->rajada_restante > 0) {
                if (tarefa_atual == tarefa && execucao_atual > 0) {
                    historico[num_eventos] = strdup(tarefa->nome);
                    tempos[num_eventos] = execucao_atual;
                    status[num_eventos] = 'L';
                    num_eventos++;
                    execucao_atual = 0;
                    tarefa_atual = NULL;
                    idle_em_andamento = false;
                }
                tarefa->prazos_perdidos++;
                tarefa->ativa = false;
                tarefa->rajada_restante = 0;
            }
        }
        
        Tarefa* tarefa_selecionada = NULL;
        int melhor_prazo = -1;
        
        for (int i = 0; i < config->num_tarefas; i++) {
            Tarefa* tarefa = &config->tarefas[i];
            if (tarefa->ativa && tarefa->rajada_restante > 0) {
                int prazo = tarefa->prazo_absoluto;
                if (tarefa_selecionada == NULL || prazo < melhor_prazo || 
                    (prazo == melhor_prazo && i < tarefa_selecionada - config->tarefas)) {
                    tarefa_selecionada = tarefa;
                    melhor_prazo = prazo;
                }
            }
        }
        
        if (tarefa_selecionada != NULL) {
            if (tarefa_atual != tarefa_selecionada) {
                if (tarefa_atual != NULL && execucao_atual > 0) {
                    historico[num_eventos] = strdup(tarefa_atual->nome);
                    tempos[num_eventos] = execucao_atual;
                    status[num_eventos] = (tarefa_atual->rajada_restante > 0) ? 'H' : 'F';
                    num_eventos++;
                    execucao_atual = 0;
                }
                if (idle_em_andamento && execucao_atual > 0) {
                    historico[num_eventos] = strdup("idle");
                    tempos[num_eventos] = execucao_atual;
                    status[num_eventos] = 'I';
                    num_eventos++;
                    execucao_atual = 0;
                    idle_em_andamento = false;
                }
                tarefa_atual = tarefa_selecionada;
            }
            
            tarefa_selecionada->rajada_restante--;
            execucao_atual++;
            
            if (tarefa_selecionada->rajada_restante == 0) {
                tarefa_selecionada->execucoes_completas++;
                tarefa_selecionada->ativa = false;
                historico[num_eventos] = strdup(tarefa_selecionada->nome);
                tempos[num_eventos] = execucao_atual;
                status[num_eventos] = 'F';
                num_eventos++;
                execucao_atual = 0;
                tarefa_atual = NULL;
            }
        } else {
            if (tarefa_atual != NULL && execucao_atual > 0) {
                historico[num_eventos] = strdup(tarefa_atual->nome);
                tempos[num_eventos] = execucao_atual;
                status[num_eventos] = (tarefa_atual->rajada_restante > 0) ? 'H' : 'F';
                num_eventos++;
                execucao_atual = 0;
                tarefa_atual = NULL;
            }
            
            if (!idle_em_andamento) {
                idle_em_andamento = true;
                execucao_atual = 1;
            } else {
                execucao_atual++;
            }
        }
        
        tempo_atual++;
    }
    
    if (tarefa_atual != NULL && execucao_atual > 0) {
        historico[num_eventos] = strdup(tarefa_atual->nome);
        tempos[num_eventos] = execucao_atual;
        status[num_eventos] = (tarefa_atual->rajada_restante > 0) ? 'H' : 'F';
        num_eventos++;
    } else if (idle_em_andamento && execucao_atual > 0) {
        historico[num_eventos] = strdup("idle");
        tempos[num_eventos] = execucao_atual;
        status[num_eventos] = 'I';
        num_eventos++;
    }
    
    escrever_saida("EDF", config, historico, tempos, status, num_eventos);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Erro: Número incorreto de argumentos\n");
        fprintf(stderr, "Uso: %s <rate|edf> <arquivo_entrada>\n", argv[0]);
        return 1;
    }
    
    char* algoritmo = argv[1];
    if (strcmp(algoritmo, "rate") != 0 && strcmp(algoritmo, "edf") != 0) {
        fprintf(stderr, "Erro: Algoritmo deve ser 'rate' ou 'edf'\n");
        return 1;
    }
    
    ConfiguracaoEscalonador config;
    strcpy(config.login, "gbl2");
    
    if (!ler_configuracao(argv[2], &config)) {
        return 1;
    }
    
    if (strcmp(algoritmo, "rate") == 0) {
        simular_rate(&config);
    } else {
        simular_edf(&config);
    }
    
    return 0;
}