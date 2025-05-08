#pragma once
#include <string>
#include <functional>
#include <fstream>
#include <algorithm>
#include "core/warehouse.h"
#include "core/solution.h"

class ObjectiveFunction {
public:
    // Função para calcular o valor objetivo
    using EvaluatorFunction = std::function<double(const Solution&, const Warehouse&)>;

    ObjectiveFunction();
    
    // Carregar de arquivo
    bool loadFromFile(const std::string& filePath);
    
    // Avaliar solução
    double evaluate(const Solution& solution, const Warehouse& warehouse) const;

    // Getters para informações da função objetivo
    std::string getName() const;
    std::string getDescription() const;
    std::string getExpression() const;
    bool isMaximize() const;
    
private:
    EvaluatorFunction evaluator;
    std::string name;
    std::string description;
    std::string expression;
    bool isMaximization;

    // Função para configurar o avaliador
    void configureEvaluator();

    // Função para remover espaços em branco
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t");
        size_t end = str.find_last_not_of(" \t");
        return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }
};