#include "modules/solucao_inicial.h"
#include "modules/selecao_otimizada.h"
#include "modules/cria_auxiliares.h"
#include "modules/refinamento_local.h"
#include "core/solution.h"
#include "input/input_parser.h"
#include "core/warehouse.h"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <set>
#include <algorithm>

// Função para validar ID de pedido
bool isValidOrderId(int orderId, const Warehouse& warehouse) {
    return orderId >= 0 && orderId < warehouse.numOrders;
}

// Implementação da função principal
bool gerarSolucaoInicial(const Warehouse& warehouse, Solution& solution) {
    std::cout << "    Gerando solução inicial otimizada..." << std::endl;
    
    // Certifique-se de que este conjunto seja definido no início da função gerarSolucaoInicial
    std::set<int> corredoresVisitados;

    // Inicializar estruturas necessárias
    solution.clear();
    
    // Inicializar estoque disponível
    std::vector<int> estoqueDisponivel(warehouse.numItems, 0);
    for (int c = 0; c < warehouse.numCorridors; c++) {
        for (const auto& item : warehouse.corridors[c]) {
            estoqueDisponivel[item.first] += item.second;
        }
    }
    
    // Mapear quais corredores contêm cada item
    std::vector<std::vector<int>> corredoresPorItem(warehouse.numItems);
    for (int c = 0; c < warehouse.numCorridors; c++) {
        for (const auto& item : warehouse.corridors[c]) {
            corredoresPorItem[item.first].push_back(c);
        }
    }
    
    // Calcular eficiência dos pedidos (itens / corredores necessários)
    std::vector<std::pair<int, double>> pedidosPriorizados;
    for (int p = 0; p < warehouse.numOrders; p++) {
        // Verificar se ID é válido
        if (!isValidOrderId(p, warehouse)) {
            continue;
        }
        
        // Contar itens totais no pedido
        int totalItens = 0;
        for (const auto& item : warehouse.orders[p]) {
            totalItens += item.second;
        }
        
        // Identificar corredores necessários
        std::set<int> corredoresNecessarios;
        for (const auto& item : warehouse.orders[p]) {
            for (int corredor : corredoresPorItem[item.first]) {
                corredoresNecessarios.insert(corredor);
            }
        }
        
        // Calcular eficiência
        double eficiencia = corredoresNecessarios.empty() ? 0 : 
                           static_cast<double>(totalItens) / corredoresNecessarios.size();
        
        pedidosPriorizados.push_back({p, eficiencia});
    }
    
    // Ordenar pedidos por eficiência (maior para menor)
    std::sort(pedidosPriorizados.begin(), pedidosPriorizados.end(),
             [](const auto& a, const auto& b) {
                 return a.second > b.second;
             });
    
    std::cout << "    Fase 1: Seleção de pedidos prioritários (limite: LB=" << warehouse.LB 
              << ", UB=" << warehouse.UB << ")..." << std::endl;
    
    std::vector<int> estoqueAtual = estoqueDisponivel;
    int totalItensAdicionados = 0;
    
    for (const auto& candidato : pedidosPriorizados) {
        int orderId = candidato.first;
        double eficiencia = candidato.second;
        
        // Verificar se ID é válido
        if (!isValidOrderId(orderId, warehouse)) {
            std::cerr << "    AVISO: ID inválido ignorado: " << orderId << std::endl;
            continue;
        }
        
        // Calcular itens no pedido
        int itensPedido = 0;
        for (const auto& item : warehouse.orders[orderId]) {
            itensPedido += item.second;
        }
        
        // Verificar limite superior
        if (totalItensAdicionados + itensPedido > warehouse.UB) {
            continue;
        }
        
        // Verificar disponibilidade de estoque
        bool estoqueOk = true;
        for (const auto& item : warehouse.orders[orderId]) {
            if (estoqueAtual[item.first] < item.second) {
                estoqueOk = false;
                break;
            }
        }
        
        if (!estoqueOk) {
            continue;
        }
        
        // Adicionar pedido à solução
        solution.addOrder(orderId, warehouse);
        
        // Atualizar estoque
        for (const auto& item : warehouse.orders[orderId]) {
            estoqueAtual[item.first] -= item.second;
        }
        
        // Identificar e adicionar corredores necessários
        for (const auto& item : warehouse.orders[orderId]) {
            for (int corredor : corredoresPorItem[item.first]) {
                corredoresVisitados.insert(corredor);
            }
        }
        
        totalItensAdicionados += itensPedido;
        
        // Se atingimos LB, podemos parar
        if (totalItensAdicionados >= warehouse.LB) {
            break;
        }
    }
    
    // Se não atingimos LB, tentar adicionar mais pedidos
    if (totalItensAdicionados < warehouse.LB) {
        std::cout << "    AVISO: Não foi possível atingir LB=" << warehouse.LB 
                 << " (atual: " << totalItensAdicionados << ")" << std::endl;
        
        // Tentar adicionar mais pedidos, mesmo que não sejam tão eficientes
        for (int p = 0; p < warehouse.numOrders; p++) {
            // Pular pedidos já adicionados
            if (std::find(solution.getSelectedOrders().begin(),
                         solution.getSelectedOrders().end(), p) != solution.getSelectedOrders().end()) {
                continue;
            }
            
            // Verificar se ID é válido
            if (!isValidOrderId(p, warehouse)) {
                continue;
            }
            
            // Calcular itens no pedido
            int itensPedido = 0;
            for (const auto& item : warehouse.orders[p]) {
                itensPedido += item.second;
            }
            
            // Verificar limite superior
            if (totalItensAdicionados + itensPedido > warehouse.UB) {
                continue;
            }
            
            // Verificar disponibilidade de estoque
            bool estoqueOk = true;
            for (const auto& item : warehouse.orders[p]) {
                if (estoqueAtual[item.first] < item.second) {
                    estoqueOk = false;
                    break;
                }
            }
            
            if (!estoqueOk) {
                continue;
            }
            
            // Adicionar pedido à solução
            solution.addOrder(p, warehouse);
            
            // Atualizar estoque
            for (const auto& item : warehouse.orders[p]) {
                estoqueAtual[item.first] -= item.second;
            }
            
            // Identificar e adicionar corredores necessários
            for (const auto& item : warehouse.orders[p]) {
                for (int corredor : corredoresPorItem[item.first]) {
                    corredoresVisitados.insert(corredor);
                }
            }
            
            totalItensAdicionados += itensPedido;
            
            // Se atingimos LB, podemos parar
            if (totalItensAdicionados >= warehouse.LB) {
                break;
            }
        }
    }
    
    // Limpar apenas os corredores, preservando os pedidos
    // Assumindo que visitedCorridors é um membro público ou há um método para modificá-lo
    std::vector<int> pedidosSelecionados = solution.getSelectedOrders();
    solution.clear();  // Isso remove tudo, incluindo pedidos e corredores

    // Readicionar os pedidos
    for (int orderId : pedidosSelecionados) {
        solution.addOrder(orderId, warehouse);
    }

    // Quando precisar adicionar corredores visitados
    for (int corredor : corredoresVisitados) {
        solution.addVisitedCorridor(corredor);
    }

    // Ao invés de atualizar corredores manualmente, use o método da classe
    solution.updateCorridors(warehouse);

    // Calcular função objetivo
    solution.calculateObjectiveValue(warehouse);
    
    // Mostrar estatísticas da solução
    std::cout << "    Solução inicial gerada:" << std::endl;
    std::cout << "      - Total de itens: " << solution.getTotalItems() << std::endl;
    std::cout << "      - Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
    std::cout << "      - Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
    std::cout << "      - Função objetivo: " << solution.getObjectiveValue() << std::endl;
    
    // Verificar viabilidade final
    bool viavel = solution.getTotalItems() >= warehouse.LB && 
                  solution.getTotalItems() <= warehouse.UB;
    
    solution.setFeasible(viavel);
    
    return viavel;
}