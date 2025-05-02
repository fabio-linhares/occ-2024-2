#include "otimizador_pli.h"
#include <limits>
#include <memory>
#include "pli_solver_custom.h"
#include "localizador_itens.h"
#include "analisador_relevancia.h"

OtimizadorPLI::OtimizadorPLI() 
    : limiteDual_(0.0), limiteTempo_(std::numeric_limits<double>::infinity()) {
    // Sempre usar nossa implementação personalizada
    solver_custom_ = std::make_unique<PLISolverCustom>();
    
    // Configurar o solver
    PLISolverCustom::Config config;
    config.metodo = PLISolverCustom::Metodo::BRANCH_AND_CUT;
    config.limiteTempo = 60.0;
    config.tolerancia = 1e-6;
    solver_custom_->configurar(config);
}

OtimizadorPLI::~OtimizadorPLI() {
    // Resources managed by unique_ptr are automatically freed
}

void OtimizadorPLI::definirLimiteTempo(double limiteTempo) {
    limiteTempo_ = limiteTempo;
    
    // Atualizar configuração do solver customizado
    PLISolverCustom::Config config;
    config.limiteTempo = limiteTempo;
    solver_custom_->configurar(config);
}

Solucao OtimizadorPLI::otimizar(
    const Deposito& deposito,
    const Backlog& backlog,
    bool resolverRelaxacaoLinear) {
    
    // Obter limitações do problema
    int LB = backlog.wave.LB;
    int UB = backlog.wave.UB;
    
    // Usar nosso solver personalizado
    PLISolverCustom::Config config;
    
    if (resolverRelaxacaoLinear) {
        config.metodo = PLISolverCustom::Metodo::PONTOS_INTERIORES;
    } else {
        config.metodo = PLISolverCustom::Metodo::BRANCH_AND_CUT;
    }
    
    solver_custom_->configurar(config);
    
    // Resolver o problema
    Solucao resultado = solver_custom_->resolver(deposito, backlog, 0.0, LB, UB);
    
    // Armazenar o valor dual para referência
    limiteDual_ = resultado.valorObjetivo;
    
    return resultado;
}

Solucao OtimizadorPLI::resolverSubproblemaDinkelbach(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    const Solucao* solucaoInicial) {
    
    // Obter limitações do problema
    int LB = backlog.wave.LB;
    int UB = backlog.wave.UB;
    
    // Resolver usando o solver personalizado
    Solucao resultado = solver_custom_->resolver(
        deposito, backlog, lambda, LB, UB, solucaoInicial);
    
    // Armazenar o valor dual
    limiteDual_ = resultado.valorObjetivo;
    
    return resultado;
}

double OtimizadorPLI::obterLimiteDual() const {
    return limiteDual_;
}

Solucao OtimizadorPLI::recuperarSolucaoInteira(
    const Deposito& deposito,
    const Backlog& backlog) {
    
    // Obter limitações do problema
    int LB = backlog.wave.LB;
    int UB = backlog.wave.UB;
    
    // Configurar para usar o método híbrido
    PLISolverCustom::Config config;
    config.metodo = PLISolverCustom::Metodo::HIBRIDO;
    solver_custom_->configurar(config);
    
    // Resolver e retornar solução inteira
    return solver_custom_->resolver(deposito, backlog, 0.0, LB, UB);
}