#include "core/solution.h"

#include <algorithm>
#include <set>
#include <map>

Solution::Solution() : objectiveValue(0.0), feasible(true), totalItems(0) {
}

void Solution::addOrder(int orderId, const Warehouse& warehouse) {
    // Verificar se o pedido já está na solução
    if (std::find(selectedOrders.begin(), selectedOrders.end(), orderId) != selectedOrders.end()) {
        return;
    }
    
    // Adicionar o pedido
    selectedOrders.push_back(orderId);
    
    // Atualizar o total de itens
    for (const auto& item : warehouse.orders[orderId]) {
        totalItems += item.second; // adiciona a quantidade de cada item
    }
    
    // Atualizar corredores
    updateCorridors(warehouse);
    
    // Atualizar função objetivo
    calculateObjectiveValue(warehouse);
}

void Solution::removeOrder(int orderId, const Warehouse& warehouse) {
    // Encontrar o pedido
    auto it = std::find(selectedOrders.begin(), selectedOrders.end(), orderId);
    if (it == selectedOrders.end()) {
        return;
    }
    
    // Remover o pedido
    selectedOrders.erase(it);
    
    // Atualizar o total de itens
    totalItems = 0;
    for (int id : selectedOrders) {
        for (const auto& item : warehouse.orders[id]) {
            totalItems += item.second;
        }
    }
    
    // Atualizar corredores
    updateCorridors(warehouse);
    
    // Atualizar função objetivo
    calculateObjectiveValue(warehouse);
}

void Solution::updateCorridors(const Warehouse& warehouse) {
    visitedCorridors.clear();
    
    // Se não há pedidos selecionados, não há corredores
    if (selectedOrders.empty()) {
        return;
    }
    
    // 1. Determinar quais itens precisamos e em que quantidade
    std::map<int, int> requiredItems;
    for (int orderId : selectedOrders) {
        for (const auto& itemPair : warehouse.orders[orderId]) {
            requiredItems[itemPair.first] += itemPair.second;
        }
    }
    
    // 2. Criar um mapa de corredores com sua utilidade
    // (utilidade = quantos itens diferentes/quantidade podemos obter do corredor)
    std::vector<std::pair<int, double>> corridorUtility;
    for (size_t corridorId = 0; corridorId < warehouse.corridors.size(); corridorId++) {
        int uniqueItemsCovered = 0;
        int totalQuantityCovered = 0;
        
        for (const auto& itemPair : warehouse.corridors[corridorId]) {
            auto it = requiredItems.find(itemPair.first);
            if (it != requiredItems.end() && it->second > 0) {
                uniqueItemsCovered++;
                totalQuantityCovered += std::min(it->second, itemPair.second);
            }
        }
        
        // Calcular utilidade combinando itens únicos e quantidade
        double utility = uniqueItemsCovered * 100.0 + totalQuantityCovered;
        if (utility > 0) {
            corridorUtility.push_back({corridorId, utility});
        }
    }
    
    // 3. Ordenar corredores por utilidade decrescente
    std::sort(corridorUtility.begin(), corridorUtility.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 4. Selecionar corredores até satisfazer todos os itens
    std::set<int> corridorSet;
    std::map<int, int> collectedItems;
    
    for (const auto& [corridorId, utility] : corridorUtility) {
        bool usefulCorridor = false;
        
        for (const auto& itemPair : warehouse.corridors[corridorId]) {
            int itemId = itemPair.first;
            int availableQty = itemPair.second;
            
            auto it = requiredItems.find(itemId);
            if (it != requiredItems.end()) {
                int neededQty = it->second - collectedItems[itemId];
                if (neededQty > 0) {
                    int takeQty = std::min(neededQty, availableQty);
                    collectedItems[itemId] += takeQty;
                    usefulCorridor = true;
                }
            }
        }
        
        if (usefulCorridor) {
            corridorSet.insert(corridorId);
        }
        
        // Verificar se já coletamos todos os itens necessários
        bool allSatisfied = true;
        for (const auto& [itemId, requiredQty] : requiredItems) {
            if (collectedItems[itemId] < requiredQty) {
                allSatisfied = false;
                break;
            }
        }
        
        if (allSatisfied) break;
    }
    
    // Converter set para vector
    visitedCorridors.assign(corridorSet.begin(), corridorSet.end());
}

double Solution::calculateObjectiveValue(const Warehouse& warehouse) {
    // Calcular a função objetivo: total de itens / número de corredores
    if (visitedCorridors.empty()) {
        objectiveValue = 0.0;
    } else {
        objectiveValue = static_cast<double>(totalItems) / visitedCorridors.size();
    }
    
    return objectiveValue;
}

const std::vector<int>& Solution::getSelectedOrders() const {
    return selectedOrders;
}

const std::vector<int>& Solution::getVisitedCorridors() const {
    return visitedCorridors;
}

double Solution::getObjectiveValue() const {
    return objectiveValue;
}

bool Solution::isFeasible() const {
    return feasible;
}

int Solution::getTotalItems() const {
    return totalItems;
}

void Solution::setFeasible(bool value) {
    feasible = value;
}