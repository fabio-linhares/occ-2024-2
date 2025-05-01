#include "modules/solucao_valida_inicial.h"
#include "modules/cria_auxiliares.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <unordered_set>

// Modificar a função de verificação de disponibilidade

bool verificarDisponibilidadeTotal(const std::vector<int>& pedidos, 
                                 const Warehouse& warehouse,
                                 std::map<int, int>& estoque_necessario,
                                 std::map<int, int>& estoque_disponivel) {
    // Limpar estruturas
    estoque_necessario.clear();
    
    // 1. Calcular total necessário por item
    for (int pedidoId : pedidos) {
        if (pedidoId < 0 || pedidoId >= warehouse.numOrders) {
            std::cerr << "ERRO: ID de pedido inválido: " << pedidoId << std::endl;
            return false;
        }
        
        for (const auto& [itemId, quantidade] : warehouse.orders[pedidoId]) {
            if (itemId < 0 || itemId >= warehouse.numItems) {
                std::cerr << "ERRO: ID de item inválido no pedido " << pedidoId 
                         << ": " << itemId << std::endl;
                return false;
            }
            estoque_necessario[itemId] += quantidade;
        }
    }
    
    // 2. Verificar se há estoque suficiente por item
    for (const auto& [itemId, quantidade] : estoque_necessario) {
        if (estoque_disponivel[itemId] < quantidade) {
            std::cerr << "ERRO: Estoque insuficiente para o item " << itemId 
                     << ". Necessário: " << quantidade 
                     << ", Disponível: " << estoque_disponivel[itemId] << std::endl;
            return false;
        }
    }
    
    return true;
}

// Função para atualizar o estoque após selecionar um pedido
void atualizarEstoque(const Warehouse& warehouse, 
                     int pedidoId,
                     std::map<int, int>& estoque_atual) {
    for (const auto& [itemId, quantidade] : warehouse.orders[pedidoId]) {
        if (itemId >= 0 && itemId < warehouse.numItems) {
            estoque_atual[itemId] -= quantidade;
        }
    }
}

// Helper function to check stock for a single order
bool verificarDisponibilidadePedido(int pedidoId,
                                    const Warehouse& warehouse,
                                    const std::map<int, int>& estoque_disponivel) {
    if (pedidoId < 0 || pedidoId >= warehouse.numOrders) {
        std::cerr << "ERRO: ID de pedido inválido ao verificar disponibilidade: " << pedidoId << std::endl;
        return false;
    }

    for (const auto& [itemId, quantidade] : warehouse.orders[pedidoId]) {
        if (itemId < 0 || itemId >= warehouse.numItems) {
            std::cerr << "ERRO: ID de item inválido no pedido " << pedidoId
                      << ": " << itemId << std::endl;
            return false; // Or handle as appropriate, maybe skip? For now, return false.
        }
        // Check if item exists in available stock map and if quantity is sufficient
        auto it = estoque_disponivel.find(itemId);
        if (it == estoque_disponivel.end() || it->second < quantidade) {
             // Optional: Add debug output
             // std::cout << "    Estoque insuficiente para item " << itemId << " no pedido " << pedidoId
             //           << ". Necessário: " << quantidade << ", Disponível: " << (it == estoque_disponivel.end() ? 0 : it->second) << std::endl;
            return false;
        }
    }
    return true;
}

bool gerarSolucaoInicialValida(const Warehouse& warehouse, Solution& solution) {
    // Limpeza e preparação inicial...
    
    std::cout << "\n=== ESTRATÉGIA AGRESSIVA PARA ATINGIR LB ===\n";
    std::cout << "Objetivo: atingir mínimo de " << warehouse.LB << " itens (máximo " << warehouse.UB << ")\n";
    
    // Ordenar TODOS os pedidos por número de itens (decrescente)
    std::vector<std::pair<int, int>> todosPedidos; // (pedidoId, totalItens)
    
    for (int p = 0; p < warehouse.numOrders; p++) {
        int totalItens = 0;
        for (const auto& [itemId, quantidade] : warehouse.orders[p]) {
            totalItens += quantidade;
        }
        todosPedidos.push_back({p, totalItens});
    }
    
    // Ordenar por número de itens (decrescente)
    std::sort(todosPedidos.begin(), todosPedidos.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Limpar solução e preparar variáveis
    solution.clear();
    int totalItensColetados = 0;
    std::set<int> corredoresAdicionados;
    std::map<int, int> estoqueConsumido; // (itemId, quantidade)
    
    // Loop principal - adicionar pedidos até atingir LB
    for (const auto& [pedidoId, totalItens] : todosPedidos) {
        // Já atingiu o LB? Pode parar
        if (totalItensColetados >= warehouse.LB) {
            break;
        }
        
        // Verificar limites UB
        if (totalItensColetados + totalItens > warehouse.UB) {
            continue; // Pular este pedido
        }
        
        // Adicionar pedido
        solution.addOrder(pedidoId, warehouse);
        totalItensColetados += totalItens;
        
        std::cout << "Adicionado pedido #" << pedidoId << " com " 
                  << totalItens << " itens. Total: " << totalItensColetados 
                  << "/" << warehouse.LB << std::endl;
        
        // Registrar consumo de estoque
        for (const auto& [itemId, quantidade] : warehouse.orders[pedidoId]) {
            estoqueConsumido[itemId] += quantidade;
        }
    }
    
    // 3. GARANTIR DISPONIBILIDADE - Adicionar TODOS os corredores necessários
    std::map<int, int> estoqueDisponivel;
    std::vector<int> itensFaltantes;
    
    // Identificar todos os itens que precisamos
    for (const auto& [itemId, quantidade] : estoqueConsumido) {
        // Para cada item, adicionar TODOS os corredores que o contêm
        for (int c = 0; c < warehouse.numCorridors; c++) {
            auto it = warehouse.corridors[c].find(itemId);
            if (it != warehouse.corridors[c].end()) {
                if (corredoresAdicionados.insert(c).second) {
                    solution.addVisitedCorridor(c);
                    
                    // Atualizar estoque disponível
                    for (const auto& [id, qtd] : warehouse.corridors[c]) {
                        estoqueDisponivel[id] += qtd;
                    }
                }
            }
        }
    }
    
    // Verificação final de disponibilidade
    bool disponibilidadeOk = true;
    
    for (const auto& [itemId, quantidadeNecessaria] : estoqueConsumido) {
        if (estoqueDisponivel[itemId] < quantidadeNecessaria) {
            std::cout << "ALERTA: Disponibilidade insuficiente para item #" << itemId 
                      << ". Necessário: " << quantidadeNecessaria 
                      << ", Disponível: " << estoqueDisponivel[itemId] << std::endl;
            disponibilidadeOk = false;
            itensFaltantes.push_back(itemId);
        }
    }
    
    // Se ainda falta disponibilidade, tentar resolver
    if (!disponibilidadeOk) {
        if (!resolverDisponibilidadeEmergencia(warehouse, solution, 
                                              estoqueConsumido, estoqueDisponivel, 
                                              itensFaltantes)) {
            std::cout << "FALHA: Não foi possível garantir disponibilidade de todos os itens.\n";
            return false;
        }
    }
    
    // Verificação final de LB
    if (totalItensColetados < warehouse.LB) {
        std::cout << "FALHA: Não foi possível atingir o LB de " << warehouse.LB 
                  << " itens. Total: " << totalItensColetados << std::endl;
        return false;
    }
    
    // Registrar métricas finais
    double razao = corredoresAdicionados.empty() ? 0.0 : 
                  static_cast<double>(totalItensColetados) / corredoresAdicionados.size();
    
    std::cout << "\n=== SOLUÇÃO FINAL ===\n";
    std::cout << "- Pedidos: " << solution.getOrders().size() << "\n";
    std::cout << "- Itens: " << totalItensColetados << " (LB=" << warehouse.LB 
              << ", UB=" << warehouse.UB << ")\n";
    std::cout << "- Corredores: " << corredoresAdicionados.size() << "\n";
    std::cout << "- Razão: " << razao << " itens/corredor\n";
    
    solution.setFeasible(true);
    return true;
}

// Nova função auxiliar para resolver problemas extremos de disponibilidade
bool resolverDisponibilidadeEmergencia(const Warehouse& warehouse, 
                                     Solution& solution,
                                     const std::map<int, int>& consumo,
                                     std::map<int, int>& estoque,
                                     const std::vector<int>& itensFaltantes) {
    std::cout << "Tentando resolver disponibilidade em modo de emergência...\n";
    
    // Para cada item faltante, remover seletivamente pedidos que o consomem
    for (int itemId : itensFaltantes) {
        int faltando = consumo.at(itemId) - estoque[itemId];
        
        // Identificar pedidos que consomem este item, ordenados por consumo
        std::vector<std::pair<int, int>> pedidosQueConsomem; // (pedidoId, quantidade)
        
        for (int pedidoId : solution.getOrders()) {
            auto it = warehouse.orders[pedidoId].find(itemId);
            if (it != warehouse.orders[pedidoId].end()) {
                pedidosQueConsomem.push_back({pedidoId, it->second});
            }
        }
        
        // Ordenar por consumo (decrescente)
        std::sort(pedidosQueConsomem.begin(), pedidosQueConsomem.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Remover pedidos até resolver o problema
        for (const auto& [pedidoId, quantidade] : pedidosQueConsomem) {
            if (estoque[itemId] >= consumo.at(itemId) - quantidade) {
                // Remover este pedido resolve o problema
                solution.removeOrder(pedidoId);
                
                // Atualizar o consumo
                for (const auto& [id, qtd] : warehouse.orders[pedidoId]) {
                    // Use um mapa temporário para registrar as mudanças
                    estoque[id] += qtd;
                }
                
                std::cout << "Removido pedido #" << pedidoId 
                          << " para resolver disponibilidade do item #" << itemId << std::endl;
                break;
            }
        }
    }
    
    // Verificar novamente a disponibilidade
    bool disponibilidadeOk = true;
    
    // Recalcular o consumo após remover pedidos
    std::map<int, int> novoConsumo;
    int totalItens = 0;
    
    for (int pedidoId : solution.getOrders()) {
        for (const auto& [itemId, quantidade] : warehouse.orders[pedidoId]) {
            novoConsumo[itemId] += quantidade;
            totalItens += quantidade;
        }
    }
    
    // Verificar disponibilidade final
    for (const auto& [itemId, necessario] : novoConsumo) {
        if (estoque[itemId] < necessario) {
            disponibilidadeOk = false;
            break;
        }
    }
    
    // Verificar LB após as remoções
    if (totalItens < warehouse.LB) {
        std::cout << "AVISO: Após resolver disponibilidade, total de itens (" 
                  << totalItens << ") ficou abaixo do LB (" << warehouse.LB << ")\n";
        return false;
    }
    
    return disponibilidadeOk;
}