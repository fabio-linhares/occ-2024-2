#pragma once

#include "problema.h"
#include "solucao.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

// Estrutura para armazenar métricas de desempenho de uma solução
struct MetricasDesempenho {
    std::string nome_instancia;
    int num_pedidos_atendidos;
    int num_corredores_utilizados;
    int total_itens;
    double razao_itens_corredor;
    long long tempo_execucao_ms;
    std::string algoritmo_utilizado;
    std::string data_execucao;
    long long tempo_total_ms; // Novo campo para o tempo total de execução
};

// Estrutura para estatísticas consolidadas
struct EstatisticasConsolidadas {
    double razao_min;
    double razao_max;
    double razao_media;
    double tempo_min;
    double tempo_max;
    double tempo_medio;
    int pedidos_min;
    int pedidos_max;
    double pedidos_medio;
    int corredores_min;
    int corredores_max;
    double corredores_medio;
};

// Funções principais
void registrarDesempenho(const std::vector<ResultadoInstancia>& resultados, const std::vector<Problema>& problemas);
void gerarRelatorioCompleto(const std::vector<ResultadoInstancia>& resultados, const std::vector<Problema>& problemas);
void exibirEstatisticasTerminal(const std::vector<ResultadoInstancia>& resultados, const std::vector<Problema>& problemas);
void salvarHistoricoDesempenho(const std::vector<MetricasDesempenho>& metricas);
void gerarRelatorioExecucao(const std::vector<MetricasDesempenho>& metricas, const std::string& data);