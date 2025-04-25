#pragma once
#include <vector>
#include <set>
#include "core/warehouse.h"

class Solution {
private:
    std::vector<int> selectedOrders;
    std::vector<int> visitedCorridors;
    double objectiveValue;
    bool feasible;
    int totalItems;

public:
    Solution();
    
    // Adiciona um pedido à solução
    void addOrder(int orderId, const Warehouse& warehouse);
    
    // Remove um pedido da solução
    void removeOrder(int orderId, const Warehouse& warehouse);
    
    // Atualiza os corredores necessários com base nos pedidos selecionados
    void updateCorridors(const Warehouse& warehouse);
    
    // Calcula o valor da função objetivo (itens/corredores)
    double calculateObjectiveValue(const Warehouse& warehouse);
    
    // Getters
    const std::vector<int>& getSelectedOrders() const;
    const std::vector<int>& getVisitedCorridors() const;
    double getObjectiveValue() const;
    bool isFeasible() const;
    int getTotalItems() const;
    
    // Setters
    void setFeasible(bool value);
};