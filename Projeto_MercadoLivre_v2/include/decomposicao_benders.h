#pragma once

#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "solucionar_desafio.h"
#include <vector>
#include <set>
#include <utility>
#include <memory>
#include <chrono>
#include <limits>

/**
 * @brief Classe para implementação da decomposição de Benders para o problema de otimização de waves
 * 
 * A decomposição de Benders divide o problema em:
 * - Problema mestre: Seleção de pedidos (variáveis inteiras)
 * - Subproblema: Alocação de corredores (variáveis contínuas)
 */
class DecomposicaoBenders {
public:
    /**
     * @brief Estrutura que representa um corte de Benders
     */
    struct Corte {
        double termo_independente;
        std::vector<double> coeficientes;
    };
    
    /**
     * @brief Construtor da classe DecomposicaoBenders
     * @param deposito Informações do depósito
     * @param backlog Informações do backlog de pedidos
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     * @param limiteTempo Limite de tempo em segundos
     * @param tolerancia Tolerância para convergência
     * @param maxIteracoes Número máximo de iterações
     */
    DecomposicaoBenders(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador,
        double limiteTempo = 3600.0,
        double tolerancia = 1e-6,
        int maxIteracoes = 100
    );
    
    /**
     * @brief Resolve o problema usando decomposição de Benders
     * @return Solução encontrada
     */
    Solucao resolver();
    
    /**
     * @brief Obtém estatísticas da última execução
     * @return String com estatísticas
     */
    std::string obterEstatisticas() const;
    
private:
    // Estruturas de dados do problema
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    
    // Parâmetros do algoritmo
    double limiteTempo_;
    double tolerancia_;
    int maxIteracoes_;
    
    // Variáveis internas
    std::vector<Corte> cortes_;
    double limiteSuperior_;
    double limiteInferior_;
    Solucao melhorSolucao_;
    
    // Estatísticas
    int iteracoesRealizadas_;
    double tempoTotal_;
    double gap_;
    
    // Métodos auxiliares
    Solucao resolverProblemaMestre();
    std::pair<double, Corte> resolverSubproblema(const std::vector<int>& pedidosSelecionados);
    void adicionarCorte(const Corte& corte);
    bool verificarConvergencia();
    
    // Métodos para construção dos problemas
    void construirProblemaMestre();
    void construirSubproblema();
};