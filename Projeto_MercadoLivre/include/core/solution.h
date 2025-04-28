#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>
#include <string>
#include <any>
#include <map>
#include <algorithm>
#include "core/warehouse.h"

class Solution {
private:
    std::vector<int> selectedOrders;  // IDs dos pedidos selecionados
    std::vector<int> visitedCorridors; // IDs dos corredores visitados
    int totalItems;                   // Total de itens coletados
    double objectiveValue;            // Valor da função objetivo
    bool feasible;                    // Indica se a solução é viável
    
    // Alteração principal: usar um mapa para dados auxiliares
    std::map<std::string, std::any> auxiliaryDataMap;

public:
    // Construtor
    Solution(); // Apenas declaração, sem implementação
    
    // Métodos de acesso
    std::vector<int> getSelectedOrders() const { return selectedOrders; }
    std::vector<int> getVisitedCorridors() const { return visitedCorridors; }
    int getTotalItems() const { return totalItems; }
    double getObjectiveValue() const { return objectiveValue; }
    bool isFeasible() const { return feasible; }
    
    // Métodos de modificação
    void addOrder(int orderId, const Warehouse& warehouse);
    void removeOrder(int orderId, const Warehouse& warehouse);
    void addVisitedCorridor(int corridorId);
    void setFeasible(bool value) { feasible = value; }
    
    // Métodos atualizados para dados auxiliares
    void setAuxiliaryData(const std::string& key, const std::any& data) { 
        auxiliaryDataMap[key] = data;
    }
    
    std::any getAuxiliaryData(const std::string& key) const {
        auto it = auxiliaryDataMap.find(key);
        if (it != auxiliaryDataMap.end()) {
            return it->second;
        }
        return std::any();
    }
    
    // Manter o método original para compatibilidade
    void setAuxiliaryData(const std::any& data) { 
        auxiliaryDataMap["default"] = data;
    }
    
    std::any getAuxiliaryData() const { 
        auto it = auxiliaryDataMap.find("default");
        if (it != auxiliaryDataMap.end()) {
            return it->second;
        }
        return std::any();
    }
    
    // Cálculo da função objetivo - Agora retorna double
    double calculateObjectiveValue(const Warehouse& warehouse);
    
    // Utilitários
    bool isOrderSelected(int orderId) const;
    void clear(); // Limpa a solução (remove todos os pedidos e corredores)
    
    // Validação da solução
    bool isValid(const Warehouse& warehouse) const;
    
    // Exportação/Importação
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename, const Warehouse& warehouse);
    
    // Atualiza corredores baseado nos pedidos selecionados
    void updateCorridors(const Warehouse& warehouse);
};

#endif // SOLUTION_H