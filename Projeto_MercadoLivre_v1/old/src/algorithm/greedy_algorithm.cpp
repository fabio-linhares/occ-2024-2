#include "algorithm/greedy_algorithm.h"
#include <algorithm>
#include <limits>
#include <iostream>
#include <set>

GreedyAlgorithm::GreedyAlgorithm() {
    // Inicialização padrão
}

void GreedyAlgorithm::buildAuxiliaryStructures(const Warehouse& warehouse) {
    // Limpar estruturas existentes
    itemToCorridors.clear();
    orderEfficiency.clear();
    
    // Construir mapa de item para corredores
    for (size_t corridorId = 0; corridorId < warehouse.corridors.size(); corridorId++) {
        for (const auto& itemPair : warehouse.corridors[corridorId]) {
            int itemId = itemPair.first;
            itemToCorridors[itemId].push_back(corridorId);
        }
    }
    
    // Calcular eficiência de cada pedido
    for (size_t orderId = 0; orderId < warehouse.orders.size(); orderId++) {
        orderEfficiency[orderId] = calculateOrderEfficiency(orderId, warehouse);
    }
}

double GreedyAlgorithm::calculateOrderEfficiency(int orderId, const Warehouse& warehouse) {
    // Calcular o número total de itens no pedido
    int totalItems = 0;
    for (const auto& itemPair : warehouse.orders[orderId]) {
        totalItems += itemPair.second;
    }
    
    // Determinar os corredores necessários
    std::set<int> requiredCorridors;
    for (const auto& itemPair : warehouse.orders[orderId]) {
        int itemId = itemPair.first;
        auto it = itemToCorridors.find(itemId);
        if (it != itemToCorridors.end()) {
            for (int corridorId : it->second) {
                requiredCorridors.insert(corridorId);
            }
        }
    }
    
    // Calcular a eficiência (itens por corredor)
    if (requiredCorridors.empty()) {
        return 0.0; // Evitar divisão por zero
    }
    
    return static_cast<double>(totalItems) / requiredCorridors.size();
}

Solution GreedyAlgorithm::solve(const Warehouse& warehouse) {
    // Construir estruturas auxiliares
    buildAuxiliaryStructures(warehouse);
    
    // Inicializar solução vazia
    Solution solution;
    
    // Ordenar pedidos por eficiência (do maior para o menor)
    std::vector<std::pair<int, double>> orderedOrders;
    for (const auto& pair : orderEfficiency) {
        orderedOrders.push_back(pair);
    }
    
    std::sort(orderedOrders.begin(), orderedOrders.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Adicionar pedidos à solução, enquanto respeitar as restrições
    for (const auto& orderPair : orderedOrders) {
        int orderId = orderPair.first;
        
        // Criar uma cópia temporária da solução para testar
        Solution tempSolution = solution;
        tempSolution.addOrder(orderId, warehouse);
        
        // Verificar se adicionar este pedido ainda mantém a solução válida
        int totalItems = tempSolution.getTotalItems();
        
        // Verificar limites de tamanho da wave
        if (totalItems > warehouse.UB) {
            continue; // Excede o limite superior, pular este pedido
        }
        
        // Se estamos satisfazendo o limite inferior, adicionar o pedido à solução
        if (totalItems >= warehouse.LB) {
            solution = tempSolution;
        } 
        // Se ainda não atingimos o limite inferior, adicionar de qualquer forma
        else if (totalItems < warehouse.LB) {
            solution = tempSolution;
        }
    }
    
    // Se não conseguimos atingir o limite inferior, a solução não é viável
    if (solution.getTotalItems() < warehouse.LB) {
        std::cout << "AVISO: Não foi possível atingir o limite inferior LB = " 
                  << warehouse.LB << std::endl;
        solution.setFeasible(false);
    }
    
    return solution;
}

Solution GreedyAlgorithm::optimize(const Warehouse& warehouse, 
                                 const Solution& initialSolution,
                                 int maxIterations,
                                 double timeLimit) {
    // Para a implementação básica, apenas retornamos a solução inicial
    // Uma implementação mais avançada poderia aplicar busca local ou outras melhorias
    return initialSolution;
}