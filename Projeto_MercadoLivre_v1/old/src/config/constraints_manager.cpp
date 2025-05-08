#include "config/constraints_manager.h"
#include <iostream>

ConstraintsManager::ConstraintsManager() {
    // Inicialização padrão
}

bool ConstraintsManager::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;
    
    std::string line, key, value;
    std::string currentName, currentType;
    
    while (std::getline(file, line)) {
        // Ignorar comentários e linhas vazias
        if (line.empty() || line[0] == '#') continue;
        
        // Procurar por pares chave-valor (CHAVE: valor)
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            key = line.substr(0, colonPos);
            value = line.substr(colonPos + 1);
            
            // Remover espaços em branco
            key = trim(key);
            value = trim(value);
            
            if (key == "RESTRICAO") {
                // Nova restrição
                currentName = value;
                constraintNames.push_back(currentName);
            } else if (key == "DESCRICAO") {
                constraintDescriptions.push_back(value);
            } else if (key == "EXPRESSAO") {
                constraintExpressions.push_back(value);
            } else if (key == "TIPO") {
                currentType = value;
                constraintTypes.push_back(value);
                
                // Adicionar o validador apropriado para este tipo
                addValidator(currentName, currentType);
            }
        }
    }
    
    // Verificar se carregamos restrições
    return !constraintNames.empty();
}

void ConstraintsManager::addValidator(const std::string& name, const std::string& type) {
    if (type == "BOUND") {
        validators[name] = validateBoundConstraint;
    } else if (type == "CAPACITY") {
        validators[name] = validateCapacityConstraint;
    } else if (type == "STRUCTURAL") {
        validators[name] = validateStructuralConstraint;
    } else {
        std::cerr << "Tipo de restrição desconhecido: " << type << std::endl;
    }
}

bool ConstraintsManager::validate(const Solution& solution, const Warehouse& warehouse) const {
    // Verificar todas as restrições
    for (const auto& name : constraintNames) {
        if (!validateConstraint(name, solution, warehouse)) {
            return false;
        }
    }
    return true;
}

bool ConstraintsManager::validateConstraint(const std::string& constraintName, 
                                          const Solution& solution, 
                                          const Warehouse& warehouse) const {
    auto it = validators.find(constraintName);
    if (it != validators.end()) {
        return it->second(solution, warehouse);
    }
    
    // Restrição não encontrada
    std::cerr << "Restrição não encontrada: " << constraintName << std::endl;
    return false;
}

const std::vector<std::string>& ConstraintsManager::getConstraintDescriptions() const {
    return constraintDescriptions;
}

const std::vector<std::string>& ConstraintsManager::getConstraintNames() const {
    return constraintNames;
}

bool ConstraintsManager::validateBoundConstraint(const Solution& solution, const Warehouse& warehouse) {
    // Verificar se o total de itens está dentro dos limites LB e UB
    int totalItems = solution.getTotalItems();
    return (totalItems >= warehouse.LB && totalItems <= warehouse.UB);
}

bool ConstraintsManager::validateCapacityConstraint(const Solution& solution, const Warehouse& warehouse) {
    // Verificar se os corredores selecionados têm estoque suficiente
    
    // 1. Calcular a demanda total por item
    std::map<int, int> itemDemand;
    for (int orderId : solution.getSelectedOrders()) {
        for (const auto& itemPair : warehouse.orders[orderId]) {
            int itemId = itemPair.first;
            int quantity = itemPair.second;
            itemDemand[itemId] += quantity;
        }
    }
    
    // 2. Calcular a oferta total por item nos corredores selecionados
    std::map<int, int> itemSupply;
    for (int corridorId : solution.getVisitedCorridors()) {
        for (const auto& itemPair : warehouse.corridors[corridorId]) {
            int itemId = itemPair.first;
            int quantity = itemPair.second;
            itemSupply[itemId] += quantity;
        }
    }
    
    // 3. Verificar se a oferta atende à demanda para cada item
    for (const auto& demandPair : itemDemand) {
        int itemId = demandPair.first;
        int demand = demandPair.second;
        
        if (itemSupply[itemId] < demand) {
            return false; // Estoque insuficiente
        }
    }
    
    return true;
}

bool ConstraintsManager::validateStructuralConstraint(const Solution& solution, const Warehouse& warehouse) {
    // Esta restrição é implícita na modelagem - todos os pedidos são atendidos completamente
    // ou não são atendidos. Sempre retorna verdadeiro.
    return true;
}