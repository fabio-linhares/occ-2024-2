#pragma once

#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "branch_and_bound_solver.h"
#include <vector>
#include <unordered_set>
#include <map>
#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>

/**
 * @brief Classe para otimização de waves usando o algoritmo de Dinkelbach
 * para problemas de programação fracionária (maximizar a razão unidades/corredores)
 */
class OtimizadorDinkelbach {
public:
    /**
     * @brief Estrutura para representar a solução de uma wave
     */
    struct SolucaoWave {
        std::vector<int> pedidosWave;        // IDs dos pedidos na wave
        std::vector<int> corredoresWave;     // IDs dos corredores na wave
        double valorObjetivo;                // Valor da função objetivo (unidades/corredores)
    };

    /**
     * @brief Estrutura para rastrear a convergência do algoritmo
     */
    struct InfoConvergencia {
        std::vector<double> valoresLambda;       // Valores de lambda em cada iteração
        std::vector<double> valoresObjetivo;     // Valores da função objetivo em cada iteração
        int iteracoesRealizadas;                // Número de iterações realizadas
        bool convergiu;                         // Indica se o algoritmo convergiu
        double tempoTotal;                      // Tempo total de execução em segundos
    };

    /**
     * @brief Construtor
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog de pedidos
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     */
    OtimizadorDinkelbach(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador
    );

    /**
     * @brief Configura os parâmetros de otimização
     * @param epsilon Tolerância para convergência
     * @param maxIteracoes Número máximo de iterações
     * @param usarBranchAndBound Flag para usar branch-and-bound nas instâncias pequenas
     */
    void configurarParametros(double epsilon, int maxIteracoes, bool usarBranchAndBound = true);

    /**
     * @brief Define se deve usar Busca Local Avançada
     * @param usar Verdadeiro para usar, falso caso contrário
     */
    void setUsarBuscaLocalAvancada(bool usar) {
        usarBuscaLocalAvancada_ = usar;
    }
    
    /**
     * @brief Define limite de tempo para Busca Local Avançada
     * @param limite Tempo em segundos
     */
    void setLimiteTempoBuscaLocal(double limite) {
        limiteTempoBuscaLocal_ = limite;
    }

    /**
     * @brief Otimiza a wave usando o algoritmo de Dinkelbach
     * @param LB Limite inferior para o número de unidades
     * @param UB Limite superior para o número de unidades
     * @return Solução otimizada
     */
    SolucaoWave otimizarWave(int LB, int UB);

    /**
     * @brief Obtém informações sobre o processo de convergência
     * @return Estrutura com informações de convergência
     */
    const InfoConvergencia& getInfoConvergencia() const;

    /**
     * @brief Exibe detalhes do processo de convergência
     */
    void exibirDetalhesConvergencia() const;

private:
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    double epsilon_;
    int maxIteracoes_;
    bool usarBranchAndBound_;
    InfoConvergencia infoConvergencia_;

    // Controle para uso de busca local avançada
    bool usarBuscaLocalAvancada_ = true;
    
    // Limite de tempo para busca local (em segundos)
    double limiteTempoBuscaLocal_ = 2.0;

    /**
     * @brief Resolve o subproblema linearizado usando branch-and-bound para instâncias pequenas
     * @param lambda Valor atual de lambda
     * @param LB Limite inferior para o número de unidades
     * @param UB Limite superior para o número de unidades
     * @return Par com a solução e o valor objetivo
     */
    std::pair<SolucaoWave, double> resolverSubproblemaComBranchAndBound(double lambda, int LB, int UB);

    /**
     * @brief Resolve o subproblema linearizado usando heurística gulosa para instâncias grandes
     * @param lambda Valor atual de lambda
     * @param LB Limite inferior para o número de unidades
     * @param UB Limite superior para o número de unidades
     * @return Par com a solução e o valor objetivo
     */
    std::pair<SolucaoWave, double> resolverSubproblemaComHeuristica(double lambda, int LB, int UB);

    /**
     * @brief Calcula o valor da função objetivo fracionária: unidades/corredores
     * @param pedidosWave IDs dos pedidos na wave
     * @return Valor da função objetivo
     */
    double calcularValorObjetivo(const std::vector<int>& pedidosWave);

    /**
     * @brief Constrói a lista de corredores necessários para os pedidos
     * @param pedidosWave IDs dos pedidos na wave
     * @return Lista de IDs dos corredores necessários
     */
    std::vector<int> construirListaCorredores(const std::vector<int>& pedidosWave);

    /**
     * @brief Calcula o valor do subproblema linearizado
     * @param solucao Solução a avaliar
     * @param lambda Valor atual de lambda
     * @return Valor do subproblema (totalUnidades - lambda * numCorredores)
     */
    double calcularValorSubproblema(const SolucaoWave& solucao, double lambda);
};