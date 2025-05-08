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
 * @brief Classe para implementação da decomposição de Dantzig-Wolfe para o problema de otimização de waves
 * 
 * A decomposição de Dantzig-Wolfe reformula o problema usando combinação convexa de pontos extremos,
 * sendo adequada para problemas com estrutura em bloco.
 */
class DecomposicaoDantzigWolfe {
public:
    /**
     * @brief Estrutura que representa uma coluna (padrão) no método de geração de colunas
     */
    struct Coluna {
        std::vector<int> pedidosIncluidos;
        double custo;
        double valorPrimal;
    };
    
    /**
     * @brief Construtor da classe DecomposicaoDantzigWolfe
     * @param deposito Informações do depósito
     * @param backlog Informações do backlog de pedidos
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     * @param limiteTempo Limite de tempo em segundos
     * @param tolerancia Tolerância para convergência
     * @param maxIteracoes Número máximo de iterações
     */
    DecomposicaoDantzigWolfe(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador,
        double limiteTempo = 3600.0,
        double tolerancia = 1e-6,
        int maxIteracoes = 100
    );
    
    /**
     * @brief Resolve o problema usando decomposição de Dantzig-Wolfe
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
    std::vector<Coluna> colunas_;
    double limiteInferior_;
    double limiteSuperior_;
    Solucao melhorSolucao_;
    
    // Estatísticas
    int iteracoesRealizadas_;
    double tempoTotal_;
    double gap_;
    int colunasGeradas_;
    
    // Métodos auxiliares
    bool resolverProblemaMestreRestrito();
    bool gerarNovaColuna();
    Solucao construirSolucaoFinal();
    bool verificarConvergencia();
    
    // Métodos para construção dos problemas
    void inicializarColunas();
    double calcularCustoReducido(const std::vector<double>& variaveisDuais, int pedidoId);
    void adicionarColuna(const Coluna& coluna);
};