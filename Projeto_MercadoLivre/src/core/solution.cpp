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
            int itemId = itemPair.first;
            int quantity = itemPair.second;
            requiredItems[itemId] += quantity;
        }
    }
    
    // 2. Encontrar quais corredores precisamos visitar
    // (Esta é uma implementação simples - pode ser otimizada para minimizar o número de corredores)
    std::set<int> corridorSet;
    
    for (const auto& entry : requiredItems) {
        int itemId = entry.first;
        int requiredQty = entry.second;
        int remainingQty = requiredQty;
        
        // Procurar o item em todos os corredores
        for (size_t corridorId = 0; corridorId < warehouse.corridors.size(); corridorId++) {
            // Se este corredor já está selecionado, pule
            if (corridorSet.find(corridorId) != corridorSet.end()) {
                continue;
            }
            
            // Verificar se o corredor tem o item
            const auto& corridor = warehouse.corridors[corridorId];
            for (const auto& itemPair : corridor) {
                if (itemPair.first == itemId) {
                    corridorSet.insert(corridorId);
                    remainingQty -= itemPair.second;
                    
                    // Se já satisfizemos a demanda deste item, podemos parar
                    if (remainingQty <= 0) {
                        break;
                    }
                }
            }
            
            // Se já satisfizemos a demanda deste item, podemos passar para o próximo
            if (remainingQty <= 0) {
                break;
            }
        }
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