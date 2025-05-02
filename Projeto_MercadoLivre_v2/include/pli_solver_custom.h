#pragma once

#include <vector>
#include <memory>
#include <string>
#include "armazem.h"
#include "solucionar_desafio.h"

/**
 * @brief Solver personalizado para Programação Linear Inteira
 */
class PLISolverCustom {
public:
    /**
     * @brief Métodos disponíveis para resolução
     */
    enum class Metodo {
        PONTOS_INTERIORES,
        SIMPLEX_BNB,
        GERACAO_COLUNAS,
        BRANCH_AND_CUT,
        HIBRIDO
    };
    
    /**
     * @brief Configurações do solver
     */
    struct Config {
        Metodo metodo = Metodo::BRANCH_AND_CUT;
        double limiteTempo = 60.0;
        double tolerancia = 1e-6;
        double gap = 0.01;
        bool usarPreprocessamento = true;
        bool usarCortesPersonalizados = true;
        int maxIteracoes = 1000;
        int numThreads = 1;
    };
    
    /**
     * @brief Construtor padrão
     */
    PLISolverCustom();
    
    /**
     * @brief Configurar o solver
     * @param config Configurações desejadas
     */
    void configurar(const Config& config);
    
    /**
     * @brief Resolver o problema de seleção de wave
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param lambda Valor de lambda para Dinkelbach
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @param solucaoInicial Solução inicial (opcional)
     * @return Solução ótima ou viável
     */
    Solucao resolver(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );
    
    /**
     * @brief Obter estatísticas da última execução
     * @return String formatada com estatísticas
     */
    std::string obterEstatisticas() const;

private:
    // Configurações
    Config config_;
    
    // Estatísticas
    struct Estatisticas {
        double tempoTotal = 0.0;
        double valorOtimo = 0.0;
        double gap = 0.0;
        int iteracoes = 0;
        int nodesExplorados = 0;
        int cortes = 0;
        int variaveisFixadas = 0;
    } estatisticas_;
    
    // Componentes do solver
    
    /**
     * @brief Implementação do método de pontos interiores
     */
    class PontosInteriores {
    public:
        void configurar(double tolerancia, int maxIteracoes);
        std::vector<double> resolver(
            const std::vector<std::vector<double>>& A,
            const std::vector<double>& b,
            const std::vector<double>& c
        );
    private:
        double tolerancia_ = 1e-6;
        int maxIteracoes_ = 100;
    };
    
    /**
     * @brief Branch-and-bound básico
     */
    class BranchAndBound {
    public:
        struct Node {
            std::vector<int> fixadosZero;
            std::vector<int> fixadosUm;
            double limiteSuperior;
        };
        
        struct Solucao {
            std::vector<int> variaveis;
            double valor;
        };
        
        Solucao resolver(
            const std::vector<std::vector<double>>& A,
            const std::vector<double>& b,
            const std::vector<double>& c,
            int limiteTempo
        );
    };
    
    // Componentes internos do solver
    std::unique_ptr<PontosInteriores> pontosInteriores_;
    std::unique_ptr<BranchAndBound> branchAndBound_;
    
    // Métodos auxiliares
    
    /**
     * @brief Constrói a matriz de restrições para o problema
     */
    void construirMatriz(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda
    );
    
    /**
     * @brief Gera cortes válidos para o problema
     */
    std::vector<std::vector<double>> gerarCortes(
        const std::vector<double>& solucaoAtual
    );
    
    /**
     * @brief Implementação do método de pontos interiores
     */
    std::vector<double> resolverPontosInteriores();
    
    /**
     * @brief Implementação de geração de colunas
     */
    Solucao resolverGeracaoColunas(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB
    );
    
    /**
     * @brief Extrai uma solução a partir das variáveis
     */
    Solucao extrairSolucao(
        const std::vector<double>& variaveis,
        const Deposito& deposito,
        const Backlog& backlog
    );
};