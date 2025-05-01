#include "config/objective_function.h"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

ObjectiveFunction::ObjectiveFunction() : isMaximization(true) {
    // Configurar avaliador padrão (eficiência de coleta = itens/corredores)
    configureEvaluator();
}

bool ObjectiveFunction::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;
    
    std::string line, key, value;
    
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
            
            if (key == "NOME") {
                name = value;
            } else if (key == "DESCRICAO") {
                description = value;
            } else if (key == "EXPRESSAO") {
                expression = value;
            } else if (key == "TIPO") {
                isMaximization = (value == "MAX");
            }
        }
    }
    
    // Configurar a função avaliadora baseada nas configurações lidas
    configureEvaluator();
    
    return true;
}

double ObjectiveFunction::evaluate(const Solution& solution, const Warehouse& warehouse) const {
    if (!evaluator) {
        std::cerr << "AVISO: Avaliador da função objetivo não configurado!" << std::endl;
        return 0.0;
    }
    
    return evaluator(solution, warehouse);
}

void ObjectiveFunction::configureEvaluator() {
    // Implementar o avaliador padrão para a função objetivo
    // No caso do desafio: max ∑(o∈O') ∑(i∈I(o)) u(oi) / |A'|
    evaluator = [this](const Solution& solution, const Warehouse& warehouse) {
        const auto& orders = solution.getSelectedOrders();
        const auto& corridors = solution.getVisitedCorridors();
        
        // Se não há corredores, a eficiência é zero
        if (corridors.empty()) {
            return 0.0;
        }
        
        // Calcular o número total de itens coletados
        int totalItems = solution.getTotalItems();
        
        // Calcular a eficiência (itens/corredores)
        double efficiency = static_cast<double>(totalItems) / corridors.size();
        
        // Se estamos minimizando, invertemos o valor
        return isMaximization ? efficiency : -efficiency;
    };
}

std::string ObjectiveFunction::getName() const {
    return name;
}

std::string ObjectiveFunction::getDescription() const {
    return description;
}

std::string ObjectiveFunction::getExpression() const {
    return expression;
}

bool ObjectiveFunction::isMaximize() const {
    return isMaximization;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    size_t last = str.find_last_not_of(" \t");
    return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, last - first + 1);
}