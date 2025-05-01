#include "branch_and_bound_solver.h"
#include <iostream>

BranchAndBoundSolver::BranchAndBoundSolver(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador,
    double limiteTempo,
    EstrategiaSelecionarVariavel estrategia
) : 
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    limiteTempo_(limiteTempo * 1000), // Converter segundos para milissegundos
    estrategia_(estrategia),
    usarCortesCobertura_(true),
    usarCortesDominancia_(true),
    coeficienteLimite_(0.7),
    rng_(std::random_device()())
{
    // Inicializar melhor solução com valores inválidos
    melhorSolucao_.valorObjetivo = -std::numeric_limits<double>::max();
    melhorSolucao_.totalUnidades = 0;
    melhorSolucao_.totalCorredores = 0;
    
    // Inicializar pseudo-custos
    pseudoCustos_.resize(backlog_.numPedidos, {1.0, 1.0});
}

BranchAndBoundSolver::Solucao BranchAndBoundSolver::resolver(double lambda, int LB, int UB) {
    // Inicializar dados para estatísticas
    estatisticas_ = Estatisticas();
    cacheContribuicoes_.clear();
    
    // Inicializar o nó raiz
    Node raiz;
    raiz.pedidosFixosIn.clear();
    raiz.pedidosFixosOut.clear();
    raiz.nivel = 0;
    raiz.lambda = lambda;
    raiz.corredoresIncluidos.clear();
    raiz.totalUnidades = 0;
    
    // Todos os pedidos estão disponíveis inicialmente
    raiz.pedidosDisponiveis.resize(backlog_.numPedidos);
    for (int i = 0; i < backlog_.numPedidos; i++) {
        raiz.pedidosDisponiveis[i] = i;
    }
    
    // Calcular limites para o nó raiz
    raiz.limiteSuperior = calcularLimiteSuperior(raiz);
    raiz.limiteInferior = calcularLimiteInferior(raiz);
    
    // Pré-processamento: Identificar pedidos incompatíveis para cortes de cobertura
    std::vector<std::pair<int, int>> pedidosIncompativeis;
    if (usarCortesCobertura_) {
        pedidosIncompativeis = identificarPedidosIncompativeis();
        std::cout << "Identificados " << pedidosIncompativeis.size() 
                  << " pares de pedidos incompatíveis para cortes de cobertura" << std::endl;
    }
    
    // Fila de prioridade para armazenar nós abertos, ordenados por limite superior decrescente
    std::priority_queue<Node> filaNodes;
    filaNodes.push(raiz);
    
    // Registrar o momento de início
    tempoInicio_ = std::chrono::high_resolution_clock::now();
    
    // Loop principal do branch-and-bound
    while (!filaNodes.empty() && !tempoExcedido()) {
        // Pegar o nó com maior limite superior
        Node nodeAtual = filaNodes.top();
        filaNodes.pop();
        
        estatisticas_.nodesExplorados++;
        
        // Se o limite superior deste nó não supera a melhor solução atual, pode ser podado
        if (nodeAtual.limiteSuperior <= melhorSolucao_.valorObjetivo) {
            estatisticas_.nodesPodados++;
            estatisticas_.nodesPodadosLS++;
            continue;
        }
        
        // Aplicar cortes de cobertura (para pedidos incompatíveis)
        if (usarCortesCobertura_ && aplicarCortesCobertura(nodeAtual, LB, UB)) {
            estatisticas_.nodesPodados++;
            estatisticas_.cortesCobertura++;
            continue;
        }
        
        // Aplicar cortes de dominância
        if (usarCortesDominancia_ && aplicarCortesDominancia(nodeAtual)) {
            estatisticas_.nodesPodados++;
            estatisticas_.cortesDominancia++;
            continue;
        }
        
        // Se não há mais pedidos disponíveis, este nó representa uma solução completa
        if (nodeAtual.pedidosDisponiveis.empty()) {
            // Construir solução a partir dos pedidos fixados para inclusão
            Solucao solucaoAtual = construirSolucao(nodeAtual.pedidosFixosIn, lambda);
            
            // Verificar se a solução é viável e melhor que a atual
            if (solucaoViavel(solucaoAtual, LB, UB) && 
                solucaoAtual.valorObjetivo > melhorSolucao_.valorObjetivo) {
                atualizarMelhorSolucao(solucaoAtual);
            }
            continue;
        }
        
        // Aplicar ramificação forte escolhendo a melhor variável para ramificar
        int pedidoIdx = selecionarPedidoParaRamificacao(nodeAtual);
        if (pedidoIdx < 0 || pedidoIdx >= nodeAtual.pedidosDisponiveis.size()) {
            // Erro na seleção de pedido
            continue;
        }
        
        int pedidoId = nodeAtual.pedidosDisponiveis[pedidoIdx];
        
        // Guardar valor atual para cálculo do impacto
        double valorObjetivoAtual = nodeAtual.limiteInferior;
        
        // Ramificar o nó atual
        auto [nodeIncluir, nodeExcluir] = ramificar(nodeAtual, pedidoId);
        
        // Calcular limites superiores e inferiores para os novos nós
        nodeIncluir.limiteSuperior = calcularLimiteSuperior(nodeIncluir);
        nodeIncluir.limiteInferior = calcularLimiteInferior(nodeIncluir);
        
        nodeExcluir.limiteSuperior = calcularLimiteSuperior(nodeExcluir);
        nodeExcluir.limiteInferior = calcularLimiteInferior(nodeExcluir);
        
        // Calcular impacto das decisões para atualizar pseudo-custos
        double impactoIncluir = nodeIncluir.limiteInferior - valorObjetivoAtual;
        double impactoExcluir = nodeExcluir.limiteInferior - valorObjetivoAtual;
        
        // Atualizar pseudo-custos
        atualizarPseudoCusto(pedidoId, true, impactoIncluir);
        atualizarPseudoCusto(pedidoId, false, impactoExcluir);
        
        // Verificar viabilidade prévia dos nós
        Solucao solIncluir = construirSolucao(nodeIncluir.pedidosFixosIn, lambda);
        bool incluirViavel = solucaoViavel(solIncluir, 0, UB); // Verificar apenas limite superior
        
        // Adicionar nós viáveis à fila
        if (incluirViavel && nodeIncluir.limiteSuperior > melhorSolucao_.valorObjetivo) {
            filaNodes.push(nodeIncluir);
        } else {
            estatisticas_.nodesPodados++;
            if (!incluirViavel) estatisticas_.nodesPodadosInfactivel++;
        }
        
        if (nodeExcluir.limiteSuperior > melhorSolucao_.valorObjetivo) {
            filaNodes.push(nodeExcluir);
        } else {
            estatisticas_.nodesPodados++;
        }
    }
    
    // Calcular estatísticas finais
    auto tempoFim = std::chrono::high_resolution_clock::now();
    estatisticas_.tempoExecucaoMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tempoFim - tempoInicio_).count();
    
    // Exibir estatísticas de execução
    std::cout << "Branch-and-Bound: explorados " << estatisticas_.nodesExplorados 
              << ", podados " << estatisticas_.nodesPodados 
              << " (LS: " << estatisticas_.nodesPodadosLS
              << ", Infactível: " << estatisticas_.nodesPodadosInfactivel
              << ", Cortes Cobertura: " << estatisticas_.cortesCobertura
              << ", Cortes Dominância: " << estatisticas_.cortesDominancia << ")"
              << ", tempo " << estatisticas_.tempoExecucaoMs << "ms" << std::endl;
    
    if (tempoExcedido()) {
        std::cout << "Tempo limite excedido. Retornando melhor solução encontrada." << std::endl;
    }
    
    return melhorSolucao_;
}

bool BranchAndBoundSolver::tempoExcedido() const {
    auto tempoAtual = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(tempoAtual - tempoInicio_).count();
    return duracao > limiteTempo_;
}

// Melhorar o cálculo de limite superior
double BranchAndBoundSolver::calcularLimiteSuperior(Node& node) {
    // Abordagem gulosa melhorada para calcular um limite superior
    
    // Primeiro, construir a solução parcial com os pedidos fixados para inclusão
    std::unordered_set<int> corredoresIncluidos = node.corredoresIncluidos;
    double valorAtual = node.totalUnidades - node.lambda * corredoresIncluidos.size();
    
    // Calcular contribuições potenciais dos pedidos disponíveis
    std::vector<std::tuple<double, int, double, double>> contribuicoes;
    
    for (int pedidoId : node.pedidosDisponiveis) {
        // Verificar se o pedido está na lista de exclusões
        bool excluido = false;
        for (int pedExcluido : node.pedidosFixosOut) {
            if (pedExcluido == pedidoId) {
                excluido = true;
                break;
            }
        }
        if (excluido) continue;
        
        // Calcular a contribuição potencial deste pedido
        auto [contribuicao, novosCorredores] = calcularContribuicaoPedido(pedidoId, node.lambda, corredoresIncluidos);
        
        // Calcular métricas adicionais para avaliação mais precisa
        int unidadesPedido = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }
        
        // Adicionar densidade (unidades/corredor)
        double densidade = novosCorredores > 0 ? (double)unidadesPedido / novosCorredores : unidadesPedido;
        
        // Adicionar à lista com múltiplas métricas
        if (contribuicao > 0) {
            contribuicoes.push_back({contribuicao, pedidoId, unidadesPedido, densidade});
        }
    }
    
    // Ordenar contribuições em ordem decrescente de valor
    std::sort(contribuicoes.begin(), contribuicoes.end(), 
              [](const auto& a, const auto& b) { return std::get<0>(a) > std::get<0>(b); });
    
    // Simular adição de pedidos de forma mais inteligente
    double limiteSuperior = valorAtual;
    std::unordered_set<int> corredoresSimulados = corredoresIncluidos;
    
    for (const auto& [contribuicao, pedidoId, unidades, densidade] : contribuicoes) {
        // Verificar se adicionar este pedido melhora a solução
        std::unordered_set<int> novosCorredores = corredoresSimulados;
        
        // Adicionar corredores necessários
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                novosCorredores.insert(corredorId);
            }
        }
        
        // Calcular novo valor
        double novoValor = (valorAtual + unidades) - node.lambda * novosCorredores.size();
        
        // Apenas adicionar se melhorar o valor
        if (novoValor > limiteSuperior) {
            limiteSuperior = novoValor;
            corredoresSimulados = novosCorredores;
            valorAtual += unidades;
        }
    }
    
    return limiteSuperior;
}

// Implementar cálculo de limite inferior
double BranchAndBoundSolver::calcularLimiteInferior(Node& node) {
    // Fazer uma construção gulosa rápida apenas com os pedidos fixos
    Solucao solucao = construirSolucao(node.pedidosFixosIn, node.lambda);
    
    // Se já temos uma solução, usar seu valor como limite inferior
    if (melhorSolucao_.valorObjetivo > -std::numeric_limits<double>::max()) {
        return std::max(solucao.valorObjetivo, melhorSolucao_.valorObjetivo);
    }
    
    return solucao.valorObjetivo;
}

BranchAndBoundSolver::Solucao BranchAndBoundSolver::construirSolucao(
    const std::vector<int>& pedidosSelecionados, double lambda) {
    
    Solucao solucao;
    solucao.pedidosWave = pedidosSelecionados;
    solucao.totalUnidades = 0;
    
    // Conjunto para rastrear corredores únicos
    std::unordered_set<int> corredoresUnicos;
    
    // Para cada pedido selecionado
    for (int pedidoId : pedidosSelecionados) {
        // Para cada item no pedido
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            // Adicionar unidades
            solucao.totalUnidades += quantidade;
            
            // Adicionar corredores necessários
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresUnicos.insert(corredorId);
            }
        }
    }
    
    // Converter conjunto para vetor de corredores
    solucao.corredoresWave.assign(corredoresUnicos.begin(), corredoresUnicos.end());
    solucao.totalCorredores = corredoresUnicos.size();
    
    // Calcular valor objetivo
    solucao.valorObjetivo = solucao.totalUnidades - lambda * solucao.totalCorredores;
    
    return solucao;
}

bool BranchAndBoundSolver::solucaoViavel(
    const Solucao& solucao, int LB, int UB) const {
    
    // Verificar limites de unidades
    if (solucao.totalUnidades < LB || solucao.totalUnidades > UB) {
        return false;
    }
    
    return true;
}

int BranchAndBoundSolver::selecionarPedidoParaRamificacao(const Node& node) {
    if (node.pedidosDisponiveis.empty()) return -1;
    
    switch (estrategia_) {
        case EstrategiaSelecionarVariavel::PRIMEIRA:
            return 0; // Simplesmente retorna o primeiro pedido disponível
            
        case EstrategiaSelecionarVariavel::MAIOR_IMPACTO:
            return selecionarPedidoPorMaiorImpacto(node);
            
        case EstrategiaSelecionarVariavel::MOST_INFEASIBLE:
            // Para problemas binários como este, a estratégia most-infeasible
            // baseia-se em valores fracionários de uma relaxação linear.
            // Como não estamos usando relaxação linear diretamente, usamos
            // uma aproximação baseada na contribuição relativa
            {
                // Implementação similar a MAIOR_IMPACTO, mas considerando outras métricas
                std::vector<std::pair<double, int>> impactos;
                
                for (size_t i = 0; i < node.pedidosDisponiveis.size(); i++) {
                    int pedidoId = node.pedidosDisponiveis[i];
                    
                    // Calcular impacto positivo (se incluir) e negativo (se excluir)
                    auto [contribuicaoPositiva, novosCorredores] = 
                        calcularContribuicaoPedido(pedidoId, node.lambda, node.corredoresIncluidos);
                    
                    // Calcular um valor que represente o quão "infeasible" seria esta variável
                    // em uma relaxação LP. Valores próximos a 0.5 seriam mais infeasíveis.
                    double unidadesPedido = 0;
                    for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                        unidadesPedido += quantidade;
                    }
                    
                    // Normalizar a contribuição para o intervalo [0,1]
                    double contribuicaoNormalizada = std::max(0.0, std::min(1.0, 
                        (contribuicaoPositiva + node.lambda * novosCorredores) / 
                        (2 * std::max(1.0, unidadesPedido))));
                    
                    // Calcular a distância para 0.5 (quanto menor, mais infeasible)
                    double distanciaDe05 = std::abs(contribuicaoNormalizada - 0.5);
                    
                    // Adicionar com prioridade inversa (quanto menor a distância, maior a prioridade)
                    impactos.push_back({-distanciaDe05, i});
                }
                
                if (impactos.empty()) return 0;
                
                // Ordenar por distância (menores distâncias primeiro)
                std::sort(impactos.begin(), impactos.end());
                
                return impactos[0].second;
            }
            
        case EstrategiaSelecionarVariavel::PSEUDO_CUSTO:
            return selecionarPedidoPorPseudoCusto(node);
            
        default:
            return 0;
    }
}

int BranchAndBoundSolver::selecionarPedidoPorMaiorImpacto(const Node& node) {
    std::vector<std::pair<double, int>> impactos;
    
    for (size_t i = 0; i < node.pedidosDisponiveis.size(); i++) {
        int pedidoId = node.pedidosDisponiveis[i];
        
        // Verificar cache primeiro
        auto cacheIt = cacheContribuicoes_.find(pedidoId);
        if (cacheIt != cacheContribuicoes_.end() && 
            node.corredoresIncluidos.size() == 0) {
            // Se o cache for válido (apenas para nós raiz, onde não há corredores incluídos)
            impactos.push_back({cacheIt->second.first, i});
        } else {
            // Calcular contribuição
            auto [contribuicao, novosCorredores] = 
                calcularContribuicaoPedido(pedidoId, node.lambda, node.corredoresIncluidos);
                
            // Armazenar no cache apenas para nós raiz
            if (node.corredoresIncluidos.size() == 0) {
                cacheContribuicoes_[pedidoId] = {contribuicao, novosCorredores};
            }
            
            impactos.push_back({contribuicao, i});
        }
    }
    
    if (impactos.empty()) return 0;
    
    // Ordenar por impacto (maior primeiro)
    std::sort(impactos.begin(), impactos.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Retornar o índice do pedido com maior impacto
    return impactos[0].second;
}

int BranchAndBoundSolver::selecionarPedidoPorPseudoCusto(const Node& node) {
    std::vector<std::pair<double, int>> pseudoCustosAcumulados;
    
    for (size_t i = 0; i < node.pedidosDisponiveis.size(); i++) {
        int pedidoId = node.pedidosDisponiveis[i];
        
        // Usar os pseudo-custos acumulados para este pedido
        double pseudoCustoIncluir = pseudoCustos_[pedidoId].first;
        double pseudoCustoExcluir = pseudoCustos_[pedidoId].second;
        
        // Calcular o produto dos pseudo-custos como métrica de impacto
        double impactoPonderado = pseudoCustoIncluir * pseudoCustoExcluir;
        
        pseudoCustosAcumulados.push_back({impactoPonderado, i});
    }
    
    if (pseudoCustosAcumulados.empty()) return 0;
    
    // Ordenar por pseudo-custo (maior primeiro)
    std::sort(pseudoCustosAcumulados.begin(), pseudoCustosAcumulados.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    return pseudoCustosAcumulados[0].second;
}

void BranchAndBoundSolver::atualizarPseudoCusto(int pedidoId, bool decisao, double impacto) {
    const double ALPHA = 0.15; // Fator de aprendizado para atualização dos pseudo-custos
    
    if (decisao) {
        // Atualização para a decisão de incluir (1)
        pseudoCustos_[pedidoId].first = (1 - ALPHA) * pseudoCustos_[pedidoId].first + ALPHA * impacto;
    } else {
        // Atualização para a decisão de excluir (0)
        pseudoCustos_[pedidoId].second = (1 - ALPHA) * pseudoCustos_[pedidoId].second + ALPHA * impacto;
    }
}

std::pair<double, int> BranchAndBoundSolver::calcularContribuicaoPedido(
    int pedidoId, double lambda, const std::unordered_set<int>& corredoresJaIncluidos) {
    
    int unidadesPedido = 0;
    std::unordered_set<int> novosCorredores;
    
    // Calcular unidades do pedido
    for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
        unidadesPedido += quantidade;
        
        // Verificar corredores necessários
        for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
            if (corredoresJaIncluidos.find(corredorId) == corredoresJaIncluidos.end()) {
                novosCorredores.insert(corredorId);
            }
        }
    }
    
    // Calcular contribuição: unidades - lambda * novos corredores
    double contribuicao = unidadesPedido - lambda * novosCorredores.size();
    
    return {contribuicao, static_cast<int>(novosCorredores.size())};
}

std::pair<BranchAndBoundSolver::Node, BranchAndBoundSolver::Node> 
BranchAndBoundSolver::ramificar(const Node& node, int pedidoId) {
    // Criar dois novos nós: um incluindo o pedido, outro excluindo
    Node nodeIncluir = node;
    Node nodeExcluir = node;
    
    // Incrementar nível
    nodeIncluir.nivel = node.nivel + 1;
    nodeExcluir.nivel = node.nivel + 1;
    
    // Adicionar pedido à lista de fixados
    nodeIncluir.pedidosFixosIn.push_back(pedidoId);
    nodeExcluir.pedidosFixosOut.push_back(pedidoId);
    
    // Remover pedido da lista de disponíveis em ambos os nós
    auto removerPedido = [pedidoId](std::vector<int>& pedidos) {
        pedidos.erase(
            std::remove(pedidos.begin(), pedidos.end(), pedidoId),
            pedidos.end()
        );
    };
    
    removerPedido(nodeIncluir.pedidosDisponiveis);
    removerPedido(nodeExcluir.pedidosDisponiveis);
    
    return {nodeIncluir, nodeExcluir};
}

bool BranchAndBoundSolver::atualizarMelhorSolucao(const Solucao& solucao) {
    if (solucao.valorObjetivo > melhorSolucao_.valorObjetivo) {
        melhorSolucao_ = solucao;
        return true;
    }
    return false;
}

// Implementar identificação de pedidos incompatíveis
std::vector<std::pair<int, int>> BranchAndBoundSolver::identificarPedidosIncompativeis() {
    std::vector<std::pair<int, int>> pedidosIncompativeis;
    
    // Duas principais razões para pedidos serem incompatíveis:
    // 1. Quando juntos excedem o limite superior (UB)
    // 2. Quando competem por recursos limitados (corredores/itens)
    
    // Verificar cada par de pedidos
    for (int i = 0; i < backlog_.numPedidos; i++) {
        for (int j = i + 1; j < backlog_.numPedidos; j++) {
            bool incompativeis = false;
            
            // Calcular total de unidades combinadas
            int unidadesI = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[i]) {
                unidadesI += quantidade;
            }
            
            int unidadesJ = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[j]) {
                unidadesJ += quantidade;
            }
            
            // Se juntos excedem o limite superior, são incompatíveis
            if (unidadesI + unidadesJ > backlog_.wave.UB) {
                incompativeis = true;
            }
            
            // Verificar se competem por recursos limitados (estoque insuficiente)
            if (!incompativeis) {
                std::unordered_map<int, int> estoqueNecessario;
                
                // Somar estoque necessário para ambos os pedidos
                for (const auto& [itemId, quantidade] : backlog_.pedido[i]) {
                    estoqueNecessario[itemId] += quantidade;
                }
                
                for (const auto& [itemId, quantidade] : backlog_.pedido[j]) {
                    estoqueNecessario[itemId] += quantidade;
                }
                
                // Verificar se há estoque suficiente para todos os itens
                for (const auto& [itemId, quantidadeNecessaria] : estoqueNecessario) {
                    int estoqueDisponivel = 0;
                    
                    // Somar estoque disponível em todos os corredores
                    for (int c = 0; c < deposito_.numCorredores; c++) {
                        const auto& corredor = deposito_.corredor[c];
                        auto it = corredor.find(itemId);
                        if (it != corredor.end()) {
                            estoqueDisponivel += it->second;
                        }
                    }
                    
                    // Se o estoque disponível é menor que o necessário
                    if (estoqueDisponivel < quantidadeNecessaria) {
                        incompativeis = true;
                        break;
                    }
                }
            }
            
            if (incompativeis) {
                pedidosIncompativeis.emplace_back(i, j);
            }
        }
    }
    
    return pedidosIncompativeis;
}

// Implementar aplicação de cortes de cobertura
bool BranchAndBoundSolver::aplicarCortesCobertura(const Node& node, int LB, int UB) {
    static std::vector<std::pair<int, int>> pedidosIncompativeis;
    
    // Inicializar apenas na primeira chamada
    if (pedidosIncompativeis.empty()) {
        pedidosIncompativeis = identificarPedidosIncompativeis();
    }
    
    // Para cada par de pedidos incompatíveis
    for (const auto& [pedidoA, pedidoB] : pedidosIncompativeis) {
        bool pedidoAIncluido = false;
        bool pedidoBIncluido = false;
        
        // Verificar se pedidoA está em pedidosFixosIn
        for (int id : node.pedidosFixosIn) {
            if (id == pedidoA) {
                pedidoAIncluido = true;
                break;
            }
        }
        
        // Verificar se pedidoB está em pedidosFixosIn
        for (int id : node.pedidosFixosIn) {
            if (id == pedidoB) {
                pedidoBIncluido = true;
                break;
            }
        }
        
        // Se ambos estão incluídos, este nó deve ser podado
        if (pedidoAIncluido && pedidoBIncluido) {
            return true;
        }
    }
    
    // Verificar cortes baseados na estrutura de corredores
    
    // Calcular unidades mínimas requeridas pelos pedidos fixos
    int unidadesMinimas = 0;
    for (int pedidoId : node.pedidosFixosIn) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesMinimas += quantidade;
        }
    }
    
    // Verificar se é possível atingir o limite inferior
    if (unidadesMinimas > UB) {
        // Impossível satisfazer limite superior
        return true;
    }
    
    // Calcular unidades máximas possíveis (incluindo todos os pedidos disponíveis)
    int unidadesMaximas = unidadesMinimas;
    for (int pedidoId : node.pedidosDisponiveis) {
        int unidadesPedido = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }
        unidadesMaximas += unidadesPedido;
    }
    
    if (unidadesMaximas < LB) {
        // Impossível atingir o limite inferior
        return true;
    }
    
    // Nenhum corte aplicável
    return false;
}

// Implementar cortes de dominância
bool BranchAndBoundSolver::aplicarCortesDominancia(const Node& node) {
    // Identificar pedidos dominados
    // Um pedido A domina um pedido B se:
    // 1. A tem menos ou igual número de unidades que B
    // 2. A requer um subconjunto dos corredores de B
    // 3. A tem um melhor valor de contribuição para a função objetivo
    
    for (int i = 0; i < node.pedidosDisponiveis.size(); i++) {
        int pedidoA = node.pedidosDisponiveis[i];
        
        // Calcular unidades e corredores para pedido A
        int unidadesA = 0;
        std::unordered_set<int> corredoresA;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoA]) {
            unidadesA += quantidade;
            
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                if (node.corredoresIncluidos.find(corredorId) == node.corredoresIncluidos.end()) {
                    corredoresA.insert(corredorId);
                }
            }
        }
        
        auto [contribuicaoA, _] = calcularContribuicaoPedido(pedidoA, node.lambda, node.corredoresIncluidos);
        
        // Verificar dominância sobre pedidos fixados como excluídos
        for (int pedidoB : node.pedidosFixosOut) {
            // Não verificar pedidos já excluídos por outros motivos
            bool pularPedidoB = false;
            for (int pedExcluido : node.pedidosFixosOut) {
                if (pedExcluido == pedidoB) {
                    pularPedidoB = true;
                    break;
                }
            }
            if (pularPedidoB) continue;
            
            // Calcular unidades e corredores para pedido B
            int unidadesB = 0;
            std::unordered_set<int> corredoresB;
            
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoB]) {
                unidadesB += quantidade;
                
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    if (node.corredoresIncluidos.find(corredorId) == node.corredoresIncluidos.end()) {
                        corredoresB.insert(corredorId);
                    }
                }
            }
            
            auto [contribuicaoB, _] = calcularContribuicaoPedido(pedidoB, node.lambda, node.corredoresIncluidos);
            
            // Verificar dominância de A sobre B
            bool corredoresContidos = true;
            for (int corredorId : corredoresA) {
                if (corredoresB.find(corredorId) == corredoresB.end()) {
                    corredoresContidos = false;
                    break;
                }
            }
            
            if (unidadesA <= unidadesB && corredoresContidos && contribuicaoA > contribuicaoB) {
                // Se B foi excluído e A domina B, então A também deve ser excluído
                return true;
            }
        }
    }
    
    return false;
}