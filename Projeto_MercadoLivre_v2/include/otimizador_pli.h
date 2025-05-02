#pragma once

#include <string>
#include <vector>
#include <memory>
#include "armazem.h"
#include "solucionar_desafio.h"
#include "pli_solver_custom.h"

/**
 * @brief Classe para otimização usando Programação Linear Inteira
 */
class OtimizadorPLI {
public:
    // Métodos existentes...
    OtimizadorPLI();
    ~OtimizadorPLI();
    
    void definirLimiteTempo(double limiteTempo);
    
    Solucao otimizar(
        const Deposito& deposito,
        const Backlog& backlog,
        bool resolverRelaxacaoLinear = false
    );
    
    Solucao resolverSubproblemaDinkelbach(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        const Solucao* solucaoInicial = nullptr
    );
    
    double obterLimiteDual() const;
    
    Solucao recuperarSolucaoInteira(
        const Deposito& deposito,
        const Backlog& backlog
    );
    
private:
    // Valor do limite dual
    double limiteDual_;
    
    // Limite de tempo para o solver (em segundos)
    double limiteTempo_;
    
    // Usar nosso solver customizado
    std::unique_ptr<PLISolverCustom> solver_custom_;
};