#include "pli_solver_custom.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <queue>
#include "pli_solver.h"
#include "armazem.h"
#include <sstream>
#include <limits>



PLISolverCustom::PLISolverCustom() {
    // Inicialização padrão
    config_.metodo = PLISolver::Config::Metodo::BRANCH_AND_CUT;
    config_.limiteTempo = 60.0;
    config_.tolerancia = 1e-6;
    config_.usarCortesPersonalizados = true;
    config_.usarWarmStart = true;
    
    // Inicializar estatísticas
    estatisticas_.tempoTotal = 0.0;
    estatisticas_.valorOtimo = 0.0;
    estatisticas_.gap = 1.0;
    estatisticas_.iteracoes = 0;
    estatisticas_.nodesExplorados = 0;
    estatisticas_.cortes = 0;
    estatisticas_.variaveisFixadas = 0;
}

void PLISolverCustom::configurar(const PLISolver::Config& config) {
    config_ = config;
}

Solucao PLISolverCustom::resolver(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    // Iniciar timer
    auto tempoInicio = std::chrono::high_resolution_clock::now();
    
    Solucao solucao;
    
    // Escolher método de solução baseado na configuração
    switch (config_.metodo) {
        case PLISolver::Config::Metodo::PONTOS_INTERIORES:
            solucao = resolverPontosInteriores(deposito, backlog, lambda, LB, UB, solucaoInicial);
            break;
        case PLISolver::Config::Metodo::SIMPLEX_BNB:
            solucao = resolverSimplexBNB(deposito, backlog, lambda, LB, UB, solucaoInicial);
            break;
        case PLISolver::Config::Metodo::GERACAO_COLUNAS:
            solucao = resolverGeracaoColunas(deposito, backlog, lambda, LB, UB, solucaoInicial);
            break;
        case PLISolver::Config::Metodo::BRANCH_AND_CUT:
            solucao = resolverBranchAndCut(deposito, backlog, lambda, LB, UB, solucaoInicial);
            break;
        case PLISolver::Config::Metodo::HIBRIDO:
            solucao = resolverHibrido(deposito, backlog, lambda, LB, UB, solucaoInicial);
            break;
        default:
            std::cerr << "Método não reconhecido. Usando Branch-and-Cut." << std::endl;
            solucao = resolverBranchAndCut(deposito, backlog, lambda, LB, UB, solucaoInicial);
    }
    
    // Calcular tempo de execução
    auto tempoFim = std::chrono::high_resolution_clock::now();
    estatisticas_.tempoTotal = std::chrono::duration<double>(tempoFim - tempoInicio).count();
    
    return solucao;
}

std::string PLISolverCustom::obterEstatisticas() const {
    std::stringstream ss;
    ss << "Estatísticas do PLI Solver:\n";
    ss << "- Tempo total: " << estatisticas_.tempoTotal << " segundos\n";
    ss << "- Valor ótimo: " << estatisticas_.valorOtimo << "\n";
    ss << "- Gap de otimalidade: " << (estatisticas_.gap * 100) << "%\n";
    ss << "- Iterações: " << estatisticas_.iteracoes << "\n";
    ss << "- Nós explorados: " << estatisticas_.nodesExplorados << "\n";
    ss << "- Cortes aplicados: " << estatisticas_.cortes << "\n";
    ss << "- Variáveis fixadas: " << estatisticas_.variaveisFixadas << "\n";
    return ss.str();
}

// Implementações básicas dos métodos específicos
Solucao PLISolverCustom::resolverPontosInteriores(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    std::cout << "Resolvendo com método de Pontos Interiores..." << std::endl;
    
    // Implementação simplificada - substituir por método real
    return resolverBranchAndBoundPersonalizado(deposito, backlog, lambda, LB, UB, solucaoInicial);
}

Solucao PLISolverCustom::resolverSimplexBNB(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    std::cout << "Resolvendo com método Simplex com Branch-and-Bound..." << std::endl;
    
    // Implementação simplificada - substituir por método real
    return resolverBranchAndBoundPersonalizado(deposito, backlog, lambda, LB, UB, solucaoInicial);
}

Solucao PLISolverCustom::resolverGeracaoColunas(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    std::cout << "Resolvendo com método de Geração de Colunas..." << std::endl;
    
    // Implementação simplificada - substituir por método real
    return resolverGulosoComMultipleStarts(deposito, backlog, lambda, LB, UB, solucaoInicial);
}

// Implementação dos métodos auxiliares

double PLISolverCustom::calcularLimiteSuperior(
    const Node& node,
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda
) {
    // Implementação básica de limite superior
    // Soma as unidades de todos os pedidos já fixados e pedidos disponíveis
    // E subtrai lambda vezes o número de corredores necessários
    
    int totalUnidades = node.totalUnidades;
    std::unordered_set<int> corredoresNecessarios = node.corredoresIncluidos;
    
    // Adicionar contribuição potencial de todos os pedidos disponíveis
    for (int pedidoId : node.pedidosDisponiveis) {
        // Calcular unidades do pedido
        int unidadesPedido = 0;
        for (const auto& [_, quantidade] : backlog.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }
        totalUnidades += unidadesPedido;
        
        // Adicionar corredores necessários
        for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
            for (int j = 0; j < deposito.numCorredores; j++) {
                if (deposito.corredor[j].find(itemId) != deposito.corredor[j].end()) {
                    corredoresNecessarios.insert(j);
                }
            }
        }
    }
    
    // Calcular valor relaxado
    return totalUnidades - lambda * corredoresNecessarios.size();
}

// Implementação do método para resolução básica usando branch-and-bound
Solucao PLISolverCustom::resolverBranchAndBoundPersonalizado(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial,
    bool usarCortes
) {
    // Iniciar timer
    tempoInicio_ = std::chrono::high_resolution_clock::now();
    
    // Criar nó raiz
    Node raiz;
    raiz.pedidosFixosIn.clear();
    raiz.pedidosFixosOut.clear();
    raiz.pedidosDisponiveis.resize(backlog.numPedidos);
    raiz.corredoresIncluidos.clear();
    raiz.totalUnidades = 0;
    raiz.nivel = 0;
    
    // Inicializar pedidos disponíveis
    for (int i = 0; i < backlog.numPedidos; i++) {
        raiz.pedidosDisponiveis[i] = i;
    }
    
    // Calcular limite superior
    raiz.limiteSuperior = calcularLimiteSuperior(raiz, deposito, backlog, lambda);
    
    // Melhor solução conhecida
    Solucao melhorSolucao;
    melhorSolucao.valorObjetivo = -std::numeric_limits<double>::max();
    
    if (solucaoInicial) {
        melhorSolucao = *solucaoInicial;
    }
    
    // Fila de prioridade para nós
    std::priority_queue<Node> fila;
    fila.push(raiz);
    
    // Estatísticas
    int nodesExplorados = 0;
    int nodesPodados = 0;
    int cortesAplicados = 0;
    
    // Loop principal de branch-and-bound
    while (!fila.empty() && !tempoExcedido()) {
        Node no = fila.top();
        fila.pop();
        nodesExplorados++;
        
        // Podar por limite
        if (no.limiteSuperior <= melhorSolucao.valorObjetivo) {
            nodesPodados++;
            continue;
        }
        
        // Se é uma folha, verificar solução
        if (no.pedidosDisponiveis.empty()) {
            Solucao solucao;
            solucao.pedidosWave = no.pedidosFixosIn;
            
            // Calcular corredores necessários
            std::unordered_set<int> corredoresNecessarios = no.corredoresIncluidos;
            solucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
            
            // Calcular valor objetivo
            int totalUnidades = no.totalUnidades;
            solucao.valorObjetivo = totalUnidades - lambda * corredoresNecessarios.size();
            
            // Verificar viabilidade (LB/UB)
            if (totalUnidades >= LB && totalUnidades <= UB) {
                if (solucao.valorObjetivo > melhorSolucao.valorObjetivo) {
                    melhorSolucao = solucao;
                }
            }
            continue;
        }
        
        // Aplicar cortes se configurado
        if (usarCortes && config_.usarCortesPersonalizados) {
            // Corte 1: Se o nó já viola UB, podar
            if (no.totalUnidades > UB) {
                nodesPodados++;
                cortesAplicados++;
                continue;
            }
            
            // Corte 2: Se mesmo com todos os pedidos disponíveis não atingir LB, podar
            int maxUnidadesDisponiveis = no.totalUnidades;
            for (int p : no.pedidosDisponiveis) {
                for (const auto& [_, quantidade] : backlog.pedido[p]) {
                    maxUnidadesDisponiveis += quantidade;
                }
            }
            
            if (maxUnidadesDisponiveis < LB) {
                nodesPodados++;
                cortesAplicados++;
                continue;
            }
        }
        
        // Selecionar variável para ramificação (pedido)
        int idxPedidoSelecionado = 0; // Por padrão, seleciona o primeiro
        
        // Estratégia: selecionar pedido com maior contribuição marginal
        double melhorContribuicao = -std::numeric_limits<double>::max();
        for (size_t i = 0; i < no.pedidosDisponiveis.size(); i++) {
            int pedidoId = no.pedidosDisponiveis[i];
            
            // Calcular contribuição: unidades - lambda*corredores_adicionais
            int unidadesAdicionais = 0;
            std::unordered_set<int> corredoresAdicionais;
            
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                unidadesAdicionais += quantidade;
                
                // Verificar corredores adicionais necessários
                for (int c = 0; c < deposito.numCorredores; c++) {
                    if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end() && 
                        no.corredoresIncluidos.find(c) == no.corredoresIncluidos.end()) {
                        corredoresAdicionais.insert(c);
                    }
                }
            }
            
            double contribuicao = unidadesAdicionais - lambda * corredoresAdicionais.size();
            if (contribuicao > melhorContribuicao) {
                melhorContribuicao = contribuicao;
                idxPedidoSelecionado = i;
            }
        }
        
        // Obter o ID do pedido selecionado
        int pedidoId = no.pedidosDisponiveis[idxPedidoSelecionado];
        
        // Criar dois nós: incluir ou excluir o pedido
        Node noIncluir = no;
        noIncluir.pedidosDisponiveis.erase(noIncluir.pedidosDisponiveis.begin() + idxPedidoSelecionado);
        noIncluir.pedidosFixosIn.push_back(pedidoId);
        noIncluir.nivel++;
        
        // Atualizar unidades e corredores para o nó incluir
        int unidadesPedido = 0;
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            unidadesPedido += quantidade;
            
            // Adicionar corredores necessários
            for (int c = 0; c < deposito.numCorredores; c++) {
                if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                    noIncluir.corredoresIncluidos.insert(c);
                }
            }
        }
        noIncluir.totalUnidades += unidadesPedido;
        
        // Recalcular limite para o nó incluir
        noIncluir.limiteSuperior = calcularLimiteSuperior(noIncluir, deposito, backlog, lambda);
        
        // Nó excluir
        Node noExcluir = no;
        noExcluir.pedidosDisponiveis.erase(noExcluir.pedidosDisponiveis.begin() + idxPedidoSelecionado);
        noExcluir.pedidosFixosOut.push_back(pedidoId);
        noExcluir.nivel++;
        
        // Recalcular limite para o nó excluir
        noExcluir.limiteSuperior = calcularLimiteSuperior(noExcluir, deposito, backlog, lambda);
        
        // Adicionar nós à fila se não podem ser podados
        if (noIncluir.limiteSuperior > melhorSolucao.valorObjetivo) {
            fila.push(noIncluir);
        } else {
            nodesPodados++;
        }
        
        if (noExcluir.limiteSuperior > melhorSolucao.valorObjetivo) {
            fila.push(noExcluir);
        } else {
            nodesPodados++;
        }
    }
    
    // Atualizar estatísticas
    estatisticas_.nodesExplorados = nodesExplorados;
    estatisticas_.variaveisFixadas = nodesPodados;
    estatisticas_.cortes = cortesAplicados;
    estatisticas_.valorOtimo = melhorSolucao.valorObjetivo;
    
    // Calcular gap de otimalidade
    double limiteSuperior = raiz.limiteSuperior;
    if (limiteSuperior > 0 && melhorSolucao.valorObjetivo > -std::numeric_limits<double>::max()) {
        estatisticas_.gap = (limiteSuperior - melhorSolucao.valorObjetivo) / limiteSuperior;
    } else {
        estatisticas_.gap = 1.0; // 100%
    }
    
    return melhorSolucao;
}

// Método auxiliar para verificar tempo excedido
bool PLISolverCustom::tempoExcedido() const {
    auto agora = std::chrono::high_resolution_clock::now();
    auto tempoDecorrido = std::chrono::duration_cast<std::chrono::seconds>(
        agora - tempoInicio_).count();
    return tempoDecorrido >= config_.limiteTempo;
}

// Métodos para os algoritmos gulosos (stub)
Solucao PLISolverCustom::resolverGulosoComRelaxacao(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    // Implementação simplificada
    Solucao solucao;
    // Implementar algoritmo guloso baseado em relaxação linear
    return solucao;
}

Solucao PLISolverCustom::resolverGulosoComMultipleStarts(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    // Implementação simplificada
    Solucao solucao;
    // Implementar algoritmo guloso com múltiplos inícios
    return solucao;
}

// Implementação para Branch-and-Cut e Híbrido (stubs)
Solucao PLISolverCustom::resolverBranchAndCut(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    std::cout << "Resolvendo com método de Branch-and-Cut..." << std::endl;
    
    // Inicializar cronômetro para controle de tempo
    tempoInicio_ = std::chrono::high_resolution_clock::now();
    
    // Por enquanto, redirecionar para o branch-and-bound personalizado
    return resolverBranchAndBoundPersonalizado(deposito, backlog, lambda, LB, UB, solucaoInicial, true);
}

Solucao PLISolverCustom::resolverHibrido(
    const Deposito& deposito,
    const Backlog& backlog,
    double lambda,
    int LB, int UB,
    const Solucao* solucaoInicial
) {
    std::cout << "Resolvendo com método Híbrido..." << std::endl;
    
    // Primeiro resolver com abordagem gulosa
    Solucao solucaoGulosa = resolverGulosoComRelaxacao(deposito, backlog, lambda, LB, UB, solucaoInicial);
    
    // Depois tentar melhorar com Branch-and-Bound com 30% do tempo disponível
    double tempoRestante = config_.limiteTempo * 0.3;
    
    // Criar um novo solver com tempo limite reduzido
    PLISolverCustom subSolver;
    Config subConfig = config_;
    subConfig.limiteTempo = tempoRestante;
    subSolver.configurar(subConfig);
    
    // Resolver com Branch-and-Bound usando a solução gulosa como inicial
    Solucao solucaoBNB = subSolver.resolverBranchAndBoundPersonalizado(
        deposito, backlog, lambda, LB, UB, &solucaoGulosa);
    
    // Retornar a melhor solução encontrada
    if (solucaoBNB.valorObjetivo > solucaoGulosa.valorObjetivo) {
        return solucaoBNB;
    } else {
        return solucaoGulosa;
    }
}