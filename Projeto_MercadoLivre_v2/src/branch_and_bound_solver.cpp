#include "branch_and_bound_solver.h"
#include "solucionar_desafio.h"
#include <iostream>
#include <queue>
#include <chrono>
#include <limits>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

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
    limiteTempo_(limiteTempo),
    estrategia_(estrategia),
    usarCortesDominancia_(true),
    usarCortesCobertura_(true),
    coeficienteLimite_(0.8),
    rng_(std::random_device{}())
{
    // Inicializar estatísticas
    estatisticas_ = Estatisticas{};
    
    // Se usando pseudo-custos, inicializar estrutura
    if (estrategia_ == EstrategiaSelecionarVariavel::PSEUDO_CUSTO) {
        pseudoCustos_.resize(backlog_.numPedidos, {0.0, 0.0});
    }
}

BranchAndBoundSolver::Solucao BranchAndBoundSolver::resolver(double lambda, int LB, int UB) {
    // Inicializar estatísticas e variáveis
    estatisticas_ = Estatisticas{};
    tempoInicio_ = std::chrono::high_resolution_clock::now();
    melhorSolucao_ = Solucao{};
    double melhorValor = -std::numeric_limits<double>::infinity();

    // Identificar pedidos incompatíveis para cortes de cobertura
    std::vector<std::pair<int, int>> pedidosIncompativeis;
    if (usarCortesCobertura_) {
        pedidosIncompativeis = identificarPedidosIncompativeis();
    }

    // Construir solução gulosa inicial
    Solucao solucaoInicial = construirSolucaoGulosa(lambda, LB, UB);

    // Verificar se temos uma solução inicial válida
    if (solucaoInicial.pedidosWave.empty()) {
        std::cout << "Não foi possível construir uma solução inicial viável." << std::endl;
        auto fimTempo = std::chrono::high_resolution_clock::now();
        estatisticas_.tempoExecucaoMs = std::chrono::duration<double, std::milli>(fimTempo - tempoInicio_).count();
        return Solucao{};
    }

    // Atualizar melhor solução
    melhorSolucao_ = solucaoInicial;
    melhorValor = solucaoInicial.totalUnidades - lambda * solucaoInicial.corredoresWave.size();

    // Criar fila de prioridade para branch and bound
    std::priority_queue<Node> filaNodos;

    // Criar nó raiz
    Node raiz;
    raiz.pedidosFixados.assign(backlog_.numPedidos, Estado::LIVRE);
    raiz.totalUnidades = 0;
    raiz.corredoresIncluidos.clear();
    raiz.nivel = 0;
    raiz.lambda = lambda;
    raiz.limiteSuperior = calcularLimiteSuperior(raiz);
    
    // Verificar se vale a pena explorar a árvore
    if (raiz.limiteSuperior <= melhorValor) {
        estatisticas_.nodesExplorados = 1;
        estatisticas_.nodesPodadosLS = 1;
        auto fimTempo = std::chrono::high_resolution_clock::now();
        estatisticas_.tempoExecucaoMs = std::chrono::duration<double, std::milli>(fimTempo - tempoInicio_).count();
        
        // Calcular BOV para solução final
        if (!melhorSolucao_.corredoresWave.empty()) {
            melhorSolucao_.valorObjetivo = static_cast<double>(melhorSolucao_.totalUnidades) / 
                                         melhorSolucao_.corredoresWave.size();
        } else {
            melhorSolucao_.valorObjetivo = 0.0;
        }
        
        return melhorSolucao_;
    }

    filaNodos.push(raiz);

    // Loop principal do Branch and Bound
    while (!filaNodos.empty()) {
        // Verificar limite de tempo
        if (tempoExcedido()) {
            break;
        }
        
        // Obter nó com maior limite superior
        Node nodoAtual = filaNodos.top();
        filaNodos.pop();

        estatisticas_.nodesExplorados++;

        // Podar por limite superior
        if (nodoAtual.limiteSuperior <= melhorValor) {
            estatisticas_.nodesPodadosLS++;
            continue;
        }

        // Verificar viabilidade para LB/UB
        int maxUnidadesPossiveis = nodoAtual.totalUnidades;
        for (int i = 0; i < backlog_.numPedidos; ++i) {
            if (nodoAtual.pedidosFixados[i] == Estado::LIVRE) {
                // Adicionar unidades de pedidos ainda livres
                for (const auto& [_, quantidade] : backlog_.pedido[i]) {
                    maxUnidadesPossiveis += quantidade;
                }
            }
        }
        
        if (maxUnidadesPossiveis < LB) {
            estatisticas_.nodesPodadosInfactivel++;
            continue;
        }
        
        if (nodoAtual.totalUnidades > UB) {
            estatisticas_.nodesPodadosInfactivel++;
            continue;
        }

        // Verificar se temos uma solução completa
        bool solucaoCompleta = true;
        for (int i = 0; i < backlog_.numPedidos; i++) {
            if (nodoAtual.pedidosFixados[i] == Estado::LIVRE) {
                solucaoCompleta = false;
                break;
            }
        }

        // Se solução completa, verificar viabilidade e atualizar
        if (solucaoCompleta) {
            if (nodoAtual.totalUnidades >= LB && nodoAtual.totalUnidades <= UB) {
                double valorAtualLinearizado = nodoAtual.totalUnidades - lambda * nodoAtual.corredoresIncluidos.size();

                if (valorAtualLinearizado > melhorValor) {
                    melhorValor = valorAtualLinearizado;
                    
                    // Construir nova melhor solução
                    melhorSolucao_.pedidosWave.clear();
                    for (int i = 0; i < backlog_.numPedidos; ++i) {
                        if (nodoAtual.pedidosFixados[i] == Estado::FIXO_IN) {
                            melhorSolucao_.pedidosWave.push_back(i);
                        }
                    }
                    
                    // Atualizar corredores e unidades
                    melhorSolucao_.corredoresWave.assign(nodoAtual.corredoresIncluidos.begin(), 
                                                      nodoAtual.corredoresIncluidos.end());
                    std::sort(melhorSolucao_.corredoresWave.begin(), melhorSolucao_.corredoresWave.end());
                    melhorSolucao_.totalUnidades = nodoAtual.totalUnidades;
                    melhorSolucao_.totalCorredores = melhorSolucao_.corredoresWave.size();
                }
            } else {
                estatisticas_.nodesPodadosInfactivel++;
            }
            continue;
        }

        // Selecionar variável para ramificação
        int varRamificar = selecionarPedidoParaRamificacao(nodoAtual);
        if (varRamificar == -1) {
            continue;
        }

        // Criar nó incluindo o pedido selecionado
        Node nodoIncluido = nodoAtual;
        nodoIncluido.pedidosFixados[varRamificar] = Estado::FIXO_IN;
        nodoIncluido.nivel++;
        
        // Adicionar unidades e corredores necessários
        int unidadesAdicionadas = 0;
        std::unordered_set<int> corredoresAdicionais;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[varRamificar]) {
            unidadesAdicionadas += quantidade;
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                if (nodoIncluido.corredoresIncluidos.find(corredorId) == nodoIncluido.corredoresIncluidos.end()) {
                    corredoresAdicionais.insert(corredorId);
                }
                nodoIncluido.corredoresIncluidos.insert(corredorId);
            }
        }
        nodoIncluido.totalUnidades += unidadesAdicionadas;

        // Aplicar cortes de cobertura
        if (usarCortesCobertura_) {
            for (const auto& [pedido1, pedido2] : pedidosIncompativeis) {
                if (pedido1 == varRamificar && nodoIncluido.pedidosFixados[pedido2] == Estado::LIVRE) {
                    nodoIncluido.pedidosFixados[pedido2] = Estado::FIXO_OUT;
                    estatisticas_.cortesDominancia++;
                } else if (pedido2 == varRamificar && nodoIncluido.pedidosFixados[pedido1] == Estado::LIVRE) {
                    nodoIncluido.pedidosFixados[pedido1] = Estado::FIXO_OUT;
                    estatisticas_.cortesDominancia++;
                }
            }
        }

        // Calcular novo limite superior
        nodoIncluido.limiteSuperior = calcularLimiteSuperior(nodoIncluido);

        // Verificar viabilidade para UB
        if (nodoIncluido.limiteSuperior > melhorValor && nodoIncluido.totalUnidades <= UB) {
            filaNodos.push(nodoIncluido);
            
            // Atualizar pseudo-custos se necessário
            if (estrategia_ == EstrategiaSelecionarVariavel::PSEUDO_CUSTO) {
                atualizarPseudoCusto(varRamificar, true, nodoAtual.limiteSuperior - nodoIncluido.limiteSuperior);
            }
        } else {
            estatisticas_.nodesPodadosLS++;
        }

        // Criar nó excluindo o pedido selecionado
        Node nodoExcluido = nodoAtual;
        nodoExcluido.pedidosFixados[varRamificar] = Estado::FIXO_OUT;
        nodoExcluido.nivel++;

        // Calcular novo limite superior
        nodoExcluido.limiteSuperior = calcularLimiteSuperior(nodoExcluido);

        // Verificar viabilidade
        if (nodoExcluido.limiteSuperior > melhorValor) {
            filaNodos.push(nodoExcluido);
            
            // Atualizar pseudo-custos se necessário
            if (estrategia_ == EstrategiaSelecionarVariavel::PSEUDO_CUSTO) {
                atualizarPseudoCusto(varRamificar, false, nodoAtual.limiteSuperior - nodoExcluido.limiteSuperior);
            }
        } else {
            estatisticas_.nodesPodadosLS++;
        }
    }

    // Calcular tempos de execução
    auto fimTempo = std::chrono::high_resolution_clock::now();
    estatisticas_.tempoExecucaoMs = std::chrono::duration<double, std::milli>(fimTempo - tempoInicio_).count();

    // Calcular BOV
    if (!melhorSolucao_.corredoresWave.empty()) {
        melhorSolucao_.valorObjetivo = static_cast<double>(melhorSolucao_.totalUnidades) / 
                                     melhorSolucao_.corredoresWave.size();
    } else {
        melhorSolucao_.valorObjetivo = 0.0;
    }
    
    melhorSolucao_.lambda = lambda;

    return melhorSolucao_;
}

// Melhorar o cálculo do limite superior para podas mais eficientes
double BranchAndBoundSolver::calcularLimiteSuperior(Node& node) {
    // Inicializa limite com valor da solução parcial atual
    double limite = node.totalUnidades - node.lambda * node.corredoresIncluidos.size();
    
    // Estrutura para armazenar contribuição potencial de cada pedido
    struct PedidoContrib {
        int id;
        double contribuicao;
        int unidades;
        std::unordered_set<int> novosCorredores;
        double eficiencia; // Nova métrica: unidades/corredores
    };
    std::vector<PedidoContrib> contribs;

    // Calcula contribuição de cada pedido livre
    for (int i = 0; i < backlog_.numPedidos; ++i) {
        if (node.pedidosFixados[i] == Estado::LIVRE) {
            int unidadesAdicionais = 0;
            std::unordered_set<int> corredoresNecessariosPedido;
            std::unordered_set<int> corredoresAdicionais;

            for (const auto& [itemId, quant] : backlog_.pedido[i]) {
                unidadesAdicionais += quant;
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    corredoresNecessariosPedido.insert(corredorId);
                    if (node.corredoresIncluidos.find(corredorId) == node.corredoresIncluidos.end()) {
                        corredoresAdicionais.insert(corredorId);
                    }
                }
            }

            // Calcular contribuição potencial
            double contrib = unidadesAdicionais - node.lambda * corredoresAdicionais.size();
            if (contrib > 0) {
                contribs.push_back({i, contrib, unidadesAdicionais, corredoresAdicionais});
            }
        }
    }

    // Calcular eficiência para cada pedido
    for (auto& pc : contribs) {
        pc.eficiencia = pc.novosCorredores.empty() ? 
                      std::numeric_limits<double>::max() : 
                      static_cast<double>(pc.unidades) / pc.novosCorredores.size();
    }

    // Ordenar por eficiência em vez de contribuição bruta
    std::sort(contribs.begin(), contribs.end(), 
            [](const PedidoContrib& a, const PedidoContrib& b) {
                return a.eficiencia > b.eficiencia;
            });

    // Adicionar contribuições positivas ao limite
    for (const auto& pc : contribs) {
        limite += pc.contribuicao;
    }

    return limite;
}

int BranchAndBoundSolver::selecionarPedidoParaRamificacao(const Node& node) {
    switch (estrategia_) {
        case EstrategiaSelecionarVariavel::PRIMEIRA: {
            // Simplesmente retorna o primeiro pedido livre
            for (int i = 0; i < backlog_.numPedidos; ++i) {
                if (node.pedidosFixados[i] == Estado::LIVRE) return i;
            }
            break;
        }
        case EstrategiaSelecionarVariavel::MAIOR_IMPACTO:
            return selecionarPedidoPorMaiorImpacto(node);
        case EstrategiaSelecionarVariavel::PSEUDO_CUSTO:
            return selecionarPedidoPorPseudoCusto(node);
        default: {
            // Fallback para o primeiro pedido livre
            for (int i = 0; i < backlog_.numPedidos; ++i) {
                if (node.pedidosFixados[i] == Estado::LIVRE) return i;
            }
        }
    }
    return -1; // Não encontrado
}

// Melhorar a estratégia de seleção de variáveis
int BranchAndBoundSolver::selecionarPedidoPorMaiorImpacto(const Node& node) {
    // Implementar estratégia que priorize pedidos com maior BOV potencial
    // Focar em pedidos que maximizem a relação unidades/corredores
    
    std::vector<std::pair<int, double>> impactosPedidos;
    
    for (int i = 0; i < backlog_.numPedidos; i++) {
        if (node.pedidosFixados[i] != Estado::LIVRE) continue;
        
        // Calcular unidades e corredores adicionais
        int unidadesAdicionais = 0;
        std::unordered_set<int> corredoresNovos;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[i]) {
            unidadesAdicionais += quantidade;
            
            // Verificar se precisamos de novos corredores
            for (const auto& [corredorId, qtd] : localizador_.getCorredoresComItem(itemId)) {
                if (node.corredoresIncluidos.find(corredorId) == node.corredoresIncluidos.end()) {
                    corredoresNovos.insert(corredorId);
                }
            }
        }
        
        // Calcular BOV incremental
        double bovAtual = (node.totalUnidades == 0 || node.corredoresIncluidos.empty()) ? 0 : 
                         (double)node.totalUnidades / node.corredoresIncluidos.size();
        
        double bovNovo = (double)(node.totalUnidades + unidadesAdicionais) / 
                       (node.corredoresIncluidos.size() + corredoresNovos.size());
        
        double impacto = bovNovo - bovAtual;
        impactosPedidos.push_back({i, impacto});
    }
    
    // Ordenar por impacto no BOV (decrescente)
    std::sort(impactosPedidos.begin(), impactosPedidos.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return !impactosPedidos.empty() ? impactosPedidos[0].first : -1;
}

int BranchAndBoundSolver::selecionarPedidoPorPseudoCusto(const Node& node) {
    double maxScore = -std::numeric_limits<double>::infinity();
    int melhorPedido = -1;

    for (int i = 0; i < backlog_.numPedidos; ++i) {
        if (node.pedidosFixados[i] == Estado::LIVRE) {
            // Calcular pseudo-custos médios
            double avgPseudoCostDown = (pseudoCustos_[i].second > 0) ? 
                                      (pseudoCustos_[i].first / pseudoCustos_[i].second) : 0.0;
            double avgPseudoCostUp = (pseudoCustos_[i].second > 0) ? 
                                    (pseudoCustos_[i].first / pseudoCustos_[i].second) : 0.0;

            // Calcular ganho potencial
            int unidadesPedido = 0;
            std::unordered_set<int> corredoresAdicionais;
            
            for (const auto& [itemId, quant] : backlog_.pedido[i]) {
                unidadesPedido += quant;
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    if (node.corredoresIncluidos.find(corredorId) == node.corredoresIncluidos.end()) {
                        corredoresAdicionais.insert(corredorId);
                    }
                }
            }
            
            double potentialGain = unidadesPedido - node.lambda * corredoresAdicionais.size();

            // Calcular score combinando ganho e pseudo-custo
            double score = potentialGain * (1.0 + avgPseudoCostUp);

            if (score > maxScore) {
                maxScore = score;
                melhorPedido = i;
            }
        }
    }

    // Fallback para o primeiro livre se não encontrou
    if (melhorPedido == -1) {
        for (int i = 0; i < backlog_.numPedidos; ++i) {
            if (node.pedidosFixados[i] == Estado::LIVRE) return i;
        }
    }
    
    return melhorPedido;
}

void BranchAndBoundSolver::atualizarPseudoCusto(int pedidoId, bool decisaoInclusao, double impactoReal) {
    if (pedidoId >= 0 && pedidoId < (int)pseudoCustos_.size()) {
        pseudoCustos_[pedidoId].first += std::abs(impactoReal);
        pseudoCustos_[pedidoId].second += 1.0;
    }
}

BranchAndBoundSolver::Solucao BranchAndBoundSolver::construirSolucaoGulosa(double lambda, int LB, int UB) {
    // Estrutura para ordenação de pedidos
    struct PedidoPontuado {
        int id;
        int unidades;
        std::unordered_set<int> corredores;
        double pontuacao;
        double eficiencia;
    };

    std::vector<PedidoPontuado> pedidos;
    pedidos.reserve(backlog_.numPedidos);

    // Calcular pontuação para cada pedido
    for (int i = 0; i < backlog_.numPedidos; i++) {
        PedidoPontuado pedido;
        pedido.id = i;
        pedido.unidades = 0;
        pedido.corredores.clear();

        // Adicionar unidades e corredores do pedido
        for (const auto& [itemId, quantidade] : backlog_.pedido[i]) {
            pedido.unidades += quantidade;
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                pedido.corredores.insert(corredorId);
            }
        }

        if (pedido.unidades == 0) continue;

        // Calcular pontuação (valor objetivo linearizado) e eficiência
        pedido.pontuacao = pedido.unidades - lambda * pedido.corredores.size();
        pedido.eficiencia = pedido.corredores.empty() ? 
                           std::numeric_limits<double>::infinity() : 
                           static_cast<double>(pedido.unidades) / pedido.corredores.size();

        pedidos.push_back(pedido);
    }

    // Ordenar pedidos por pontuação decrescente
    std::sort(pedidos.begin(), pedidos.end(),
             [](const PedidoPontuado& a, const PedidoPontuado& b) {
                 return a.pontuacao > b.pontuacao;
             });

    // Inicializar solução vazia
    Solucao solucao;
    solucao.pedidosWave.clear();
    solucao.corredoresWave.clear();
    solucao.valorObjetivo = 0.0;
    solucao.totalUnidades = 0;
    solucao.totalCorredores = 0;

    // Controlar estoque e corredores
    std::unordered_set<int> corredoresUnicos;
    std::unordered_map<int, int> estoqueConsumido;

    // Adicionar pedidos em ordem de pontuação
    for (const auto& pedido : pedidos) {
        // Pular pedidos com pontuação negativa
        if (pedido.pontuacao <= 0) continue;

        // Verificar limite superior
        if (solucao.totalUnidades + pedido.unidades > UB) {
            continue;
        }

        // Verificar viabilidade de estoque
        bool viavelEstoque = true;
        std::unordered_map<int, int> consumoPedido;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedido.id]) {
            consumoPedido[itemId] += quantidade;
            int estoqueDisponivelItem = verificador_.estoqueTotal[itemId];
            if (estoqueConsumido[itemId] + quantidade > estoqueDisponivelItem) {
                viavelEstoque = false;
                break;
            }
        }

        // Pular pedido inviável
        if (!viavelEstoque) {
            continue;
        }

        // Adicionar pedido à solução
        solucao.pedidosWave.push_back(pedido.id);
        solucao.totalUnidades += pedido.unidades;

        // Atualizar estoque consumido
        for (const auto& [itemId, quantidade] : consumoPedido) {
            estoqueConsumido[itemId] += quantidade;
        }

        // Atualizar corredores
        for (int corredor : pedido.corredores) {
            corredoresUnicos.insert(corredor);
        }

        // Parar se atingir LB
        if (solucao.totalUnidades >= LB) {
            break;
        }
    }

    // Se não atingiu LB, tentar adicionar por eficiência
    if (solucao.totalUnidades < LB) {
        // Reordenar por eficiência
        std::sort(pedidos.begin(), pedidos.end(),
             [](const PedidoPontuado& a, const PedidoPontuado& b) {
                 if (a.corredores.empty() && b.corredores.empty()) return a.unidades > b.unidades;
                 if (a.corredores.empty()) return true;
                 if (b.corredores.empty()) return false;
                 return a.eficiencia > b.eficiencia;
             });

        // Tentar adicionar pedidos mais eficientes
        for (const auto& pedido : pedidos) {
            // Verificar se já está incluído
            bool jaIncluido = false;
            for (int pid : solucao.pedidosWave) { 
                if (pid == pedido.id) { 
                    jaIncluido = true; 
                    break; 
                } 
            }
            
            if (jaIncluido || pedido.unidades == 0) continue;

            // Verificar limite superior
            if (solucao.totalUnidades + pedido.unidades > UB) {
                continue;
            }

            // Verificar viabilidade de estoque
            bool viavelEstoque = true;
            std::unordered_map<int, int> consumoPedido;
            
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedido.id]) {
                consumoPedido[itemId] += quantidade;
                int estoqueDisponivelItem = verificador_.estoqueTotal[itemId];
                if (estoqueConsumido[itemId] + quantidade > estoqueDisponivelItem) {
                    viavelEstoque = false;
                    break;
                }
            }

            // Pular pedido inviável
            if (!viavelEstoque) {
                continue;
            }

            // Adicionar pedido à solução
            solucao.pedidosWave.push_back(pedido.id);
            solucao.totalUnidades += pedido.unidades;

            // Atualizar estoque consumido
            for (const auto& [itemId, quantidade] : consumoPedido) {
                estoqueConsumido[itemId] += quantidade;
            }

            // Atualizar corredores
            for (int corredor : pedido.corredores) {
                corredoresUnicos.insert(corredor);
            }

            // Parar se atingir LB
            if (solucao.totalUnidades >= LB) {
                break;
            }
        }
    }

    // Verificar se solução atinge LB
    if (solucao.totalUnidades < LB) {
        return Solucao{}; // Solução vazia
    }

    // Atualizar corredores e calcular valor objetivo
    solucao.corredoresWave.assign(corredoresUnicos.begin(), corredoresUnicos.end());
    std::sort(solucao.corredoresWave.begin(), solucao.corredoresWave.end());
    solucao.totalCorredores = solucao.corredoresWave.size();
    
    if (solucao.totalCorredores > 0) {
        solucao.valorObjetivo = static_cast<double>(solucao.totalUnidades) / solucao.totalCorredores;
    } else {
        solucao.valorObjetivo = 0.0;
    }

    return solucao;
}

std::vector<std::pair<int, int>> BranchAndBoundSolver::identificarPedidosIncompativeis() {
    std::vector<std::pair<int, int>> incompativeis;

    // Verificar todos os pares de pedidos
    for (int i = 0; i < backlog_.numPedidos; i++) {
        for (int j = i + 1; j < backlog_.numPedidos; j++) {
            // Se os pedidos não podem estar juntos, adicionar ao vetor
            if (!verificarCompabilidadePedidos(i, j)) {
                incompativeis.push_back({i, j});
            }
        }
    }

    return incompativeis;
}

bool BranchAndBoundSolver::verificarCompabilidadePedidos(int pedidoId1, int pedidoId2) {
    // Verificar se há estoque suficiente para atender ambos os pedidos juntos
    std::unordered_map<int, int> demandaTotal;
    
    // Somar demanda do primeiro pedido
    for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId1]) {
        demandaTotal[itemId] += quantidade;
    }
    
    // Somar demanda do segundo pedido
    for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId2]) {
        demandaTotal[itemId] += quantidade;
    }
    
    // Verificar se há estoque suficiente para cada item
    for (const auto& [itemId, quantidadeTotal] : demandaTotal) {
        if (verificador_.estoqueTotal[itemId] < quantidadeTotal) {
            return false; // Incompatíveis por estoque insuficiente
        }
    }
    
    return true; // Os pedidos são compatíveis
}

bool BranchAndBoundSolver::tempoExcedido() const {
    if (limiteTempo_ <= 0) return false;
    auto tempoAtual = std::chrono::high_resolution_clock::now();
    double tempoDecorrido = std::chrono::duration<double>(tempoAtual - tempoInicio_).count();
    return tempoDecorrido > limiteTempo_;
}