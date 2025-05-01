#pragma once

#include <vector>
#include <map>
#include <string>

/**
 * @brief Classe que representa o armazém com pedidos e corredores
 */
class Warehouse {
public:
    // Atributos
    int numOrders = 0;
    int numItems = 0;
    int numCorridors = 0;
    int LB = 0;
    int UB = 0;
    std::vector<std::map<int, int>> orders;     // [orderId][itemId] = quantidade
    std::vector<std::map<int, int>> corridors;  // [corridorId][itemId] = quantidade
    
    // Métodos definidos inline
    bool isValid() const {
        return true;  // Implementação mínima
    }
    
    int getTotalAvailableItems() const {
        int total = 0;
        for (const auto& corridor : corridors) {
            for (const auto& [itemId, quantity] : corridor) {
                total += quantity;
            }
        }
        return total;
    }
    
    void clear() {
        numOrders = 0;
        numItems = 0;
        numCorridors = 0;
        LB = 0;
        UB = 0;
        orders.clear();
        corridors.clear();
    }
};