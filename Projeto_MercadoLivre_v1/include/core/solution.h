#pragma once

#include "core/warehouse.h"
#include <set>
#include <string>
#include <sstream>

class Solution {
private:
    std::set<int> selectedOrders;
    std::set<int> visitedCorridors;

public:
    // Construtores
    Solution() = default;
    
    // Métodos definidos inline (diretamente no header)
    void addOrder(int orderId) {
        selectedOrders.insert(orderId);
    }
    
    void addVisitedCorridor(int corridorId) {
        visitedCorridors.insert(corridorId);
    }
    
    const std::set<int>& getSelectedOrders() const {
        return selectedOrders;
    }
    
    const std::set<int>& getVisitedCorridors() const {
        return visitedCorridors;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Solução: " << selectedOrders.size() << " pedidos, "
            << visitedCorridors.size() << " corredores";
        return oss.str();
    }
};