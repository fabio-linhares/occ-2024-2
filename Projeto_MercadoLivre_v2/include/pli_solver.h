#pragma once

#include "armazem.h"
#include "solucionar_desafio.h"
#include <string>

/**
 * @brief Classe base para solvers de Programação Linear Inteira
 */
class PLISolver {
public:
    /**
     * @brief Configurações para o solver
     */
    struct Config {
        enum class Metodo {
            PONTOS_INTERIORES,  // Método de pontos interiores
            SIMPLEX_BNB,        // Método simplex com branch-and-bound
            GERACAO_COLUNAS,    // Método de geração de colunas
            BRANCH_AND_CUT,     // Método branch-and-cut
            HIBRIDO             // Método híbrido
        };
        
        Metodo metodo = Metodo::BRANCH_AND_CUT;  // Método padrão
        double limiteTempo = 60.0;               // Limite de tempo em segundos
        double tolerancia = 1e-6;                // Tolerância para convergência
        bool usarCortesPersonalizados = true;    // Usar cortes customizados
        bool usarWarmStart = true;               // Usar warm start quando disponível
    };
    
    /**
     * @brief Destrutor virtual para permitir herança
     */
    virtual ~PLISolver() = default;
    
    /**
     * @brief Configura o solver com as opções especificadas
     * @param config Configurações para o solver
     */
    virtual void configurar(const Config& config) = 0;
    
    /**
     * @brief Resolve o problema de otimização
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param lambda Valor atual de lambda (parâmetro do algoritmo de Dinkelbach)
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @param solucaoInicial Solução inicial opcional para warm start
     * @return Solução otimizada
     */
    virtual Solucao resolver(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    ) = 0;
    
    /**
     * @brief Retorna estatísticas da última execução
     * @return String com estatísticas formatadas
     */
    virtual std::string obterEstatisticas() const = 0;
};