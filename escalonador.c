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