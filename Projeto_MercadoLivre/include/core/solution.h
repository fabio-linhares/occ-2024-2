#pragma once
#include <vector>
#include <set>
#include <any>
#include <unordered_map>
#include <string>
#include <stdexcept>  // Adicionado para std::runtime_error
#include "core/warehouse.h"

class Solution {
private:
    std::vector<int> selectedOrders;
    std::vector<int> visitedCorridors;
    double objectiveValue;
    bool feasible;
    int totalItems;
    
    // Mapa para armazenar dados auxiliares genéricos
    std::unordered_map<std::string, std::any> auxiliaryData;

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
    
    // Métodos para gerenciar dados auxiliares
    template<typename T>
    void setAuxiliaryData(const std::string& key, const T& value) {
        auxiliaryData[key] = value;
    }
    
    template<typename T>
    T getAuxiliaryData(const std::string& key) const {
        if (auxiliaryData.find(key) == auxiliaryData.end()) {
            throw std::runtime_error("Chave não encontrada nos dados auxiliares: " + key);
        }
        return std::any_cast<T>(auxiliaryData.at(key));
    }
    
    bool hasAuxiliaryData(const std::string& key) const {
        return auxiliaryData.find(key) != auxiliaryData.end();
    }
};