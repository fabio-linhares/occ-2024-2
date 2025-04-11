#pragma once

#include "problema.h"
#include "solucao.h"
#include "metricas.h"
#include "restricoes.h"
#include <vector>
#include <set>

struct Parametros {
    double intensidade_perturbacao = 0.3;
    int max_iteracoes_perturbacao = 20;
};

// Funções principais
Solucao resolverProblemaAdaptativo(const Problema& problema, const RestricoesConfig& config);
bool verificarCorrigirDisponibilidade(const Problema& problema, Solucao& solucao, const RestricoesConfig& config);
bool verificarCorrigirDisponibilidade(const Problema& problema, Solucao& solucao);
bool validarSolucao(const Problema& problema, const Solucao& solucao, const RestricoesConfig& config);
void busca_local_otimizada(const Problema& problema, Solucao& solucao, int max_iteracoes);
Solucao construirSolucaoInicial(const Problema& problema, int L_min, int L_max);
Solucao solucaoDeUltimoRecurso(const Problema& problema, const RestricoesConfig& config);

// Método de Dinkelbach
Solucao aplicarDinkelbachAcelerado(const Problema& problema, const Parametros& params, const MetricasInstancia& metricas);
Solucao resolverProblemaParametricoOtimizado(const Problema& problema, double lambda, int L_min, int L_max);

// Funções auxiliares
void removerCorredoresRedundantes(const Problema& problema, Solucao& solucao);
std::pair<std::set<int>, int> calcularCorredoresEItens(const std::vector<int>& pedidos, const Problema& problema);
std::set<int> calcularCorredoresAdicionais(const Problema& problema, int pedido_id, const std::set<int>& corredores_existentes);
void ajustarSolucaoParaLimites(Solucao& solucao, const Problema& problema, int total_itens, int L_min, int L_max);