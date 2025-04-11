#pragma once

#include "problema.h"
#include "solucao.h"
#include "metricas.h"
#include <vector>

struct Parametros {
    double intensidade_perturbacao;
    int max_iteracoes_perturbacao;
    double intensidade_grasp;
    int max_iteracoes_grasp;
    bool usar_perturbacao_agressiva;
    bool usar_modelo_exato_4pedidos;
};

Solucao calcularWave(const Problema& problema, const std::vector<int>& indices_pedidos, bool aplicar_busca_local = true);
Solucao resolverProblemaAdaptativo(const Problema& problema);
bool calibrarAlgoritmo(Parametros& parametros);
Solucao algoritmoGulosoMelhorado(const Problema& problema, const MetricasInstancia& metricas);
void melhorar_solucao_local_com_metricas(const Problema& problema, Solucao& solucao, const MetricasInstancia& metricas);
Solucao aplicarGRASPComRankings(const Problema& problema, const Parametros& params, const MetricasInstancia& metricas);
Solucao buscaExaustiva(const Problema& problema);
Solucao solucionadorRobusto(const Problema& problema);
void busca_local_otimizada(const Problema& problema, Solucao& solucao);

Solucao aplicarILS(const Problema& problema, const Parametros& params, const MetricasInstancia& metricas);
Solucao construirSolucaoInicial(const Problema& problema, int L_min, int L_max);
Solucao perturbarSolucao(const Solucao& solucao_atual, const Problema& problema, 
                        double intensidade, int L_min, int L_max);