#pragma once
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <map>
#include "core/warehouse.h"
#include "core/solution.h"

class ConstraintsManager {
public:
    // Função para validar uma restrição
    using ConstraintValidator = std::function<bool(const Solution&, const Warehouse&)>;
    
    ConstraintsManager();
    
    // Carregar de arquivo
    bool loadFromFile(const std::string& filePath);
    
    // Validar todas as restrições
    bool validate(const Solution& solution, const Warehouse& warehouse) const;
    
    // Validar uma restrição específica
    bool validateConstraint(const std::string& constraintName, 
                           const Solution& solution, 
                           const Warehouse& warehouse) const;
    
    // Obter descrições das restrições
    const std::vector<std::string>& getConstraintDescriptions() const;
    
    // Obter nomes das restrições
    const std::vector<std::string>& getConstraintNames() const;
    
private:
    std::vector<std::string> constraintNames;
    std::vector<std::string> constraintDescriptions;
    std::vector<std::string> constraintExpressions;
    std::vector<std::string> constraintTypes;
    std::map<std::string, ConstraintValidator> validators;
    
    // Adiciona um validador à coleção
    void addValidator(const std::string& name, const std::string& type);
    
    // Implementações específicas de validadores
    static bool validateBoundConstraint(const Solution& solution, const Warehouse& warehouse);
    static bool validateCapacityConstraint(const Solution& solution, const Warehouse& warehouse);
    static bool validateStructuralConstraint(const Solution& solution, const Warehouse& warehouse);
    
    // Função para remover espaços em branco
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t");
        size_t end = str.find_last_not_of(" \t");
        return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }
};