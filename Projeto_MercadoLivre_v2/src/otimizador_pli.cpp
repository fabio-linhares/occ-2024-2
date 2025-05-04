#include "otimizador_pli.h"
#include <limits>
#include <memory>
#include "pli_solver_custom.h"
#include "localizador_itens.h"
#include "pli_solver.h"

OtimizadorPLI::OtimizadorPLI() : limiteDual_(0.0), limiteTempo_(60.0) {
    // Criar solver customizado
    solver_custom_ = std::make_unique<PLISolverCustom>();
    
    // Configuração padrão
    PLISolver::Config config;
    config.metodo = PLISolver::Config::Metodo::BRANCH_AND_CUT;
    config.limiteTempo = limiteTempo_;
    solver_custom_->configurar(config);
}

OtimizadorPLI::~OtimizadorPLI() = default;

void OtimizadorPLI::definirLimiteTempo(double limiteTempo) {
    limiteTempo_ = limiteTempo;
    
    // Atualizar configuração do solver
    PLISolver::Config config;
    config.limiteTempo = limiteTempo;
    solver_custom_->configurar(config);
}

Solucao OtimizadorPLI::otimizar(
    const Deposito& deposito,
    const Backlog& backlog,
    bool resolverRelaxacaoLinear
) {
    // Configurar solver
    PLISolver::Config config;
    
    if (resolverRelaxacaoLinear) {
        config.metodo = PLISolver::Config::Metodo::PONTOS_INTERIORES;
    } else {
        config.metodo = PLISolver::Config::Metodo::BRANCH_AND_CUT;
    }
    
    solver_custom_->configurar(config);
    
    // Resolver com lambda = 0 inicialmente (maximizar unidades sem penalidade)
    double lambda = 0.0;
    return solver_custom_->resolver(deposito, backlog, lambda, 
                                   backlog.wave.LB, backlog.wave.UB);
}

Solucao OtimizadorPLI::resolverSubproblemaDinkelbach(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    const Solucao* solucaoInicial
) {
    return solver_custom_->resolver(deposito, backlog, lambda, 
                                   backlog.wave.LB, backlog.wave.UB, 
                                   solucaoInicial);
}

double OtimizadorPLI::obterLimiteDual() const {
    return limiteDual_;
}

Solucao OtimizadorPLI::recuperarSolucaoInteira(
    const Deposito& deposito,
    const Backlog& backlog
) {
    // Configurar solver para método híbrido que combina relaxação e arredondamento
    PLISolver::Config config;
    config.metodo = PLISolver::Config::Metodo::HIBRIDO;
    solver_custom_->configurar(config);
    
    return solver_custom_->resolver(deposito, backlog, 0.0, 
                                   backlog.wave.LB, backlog.wave.UB);
}