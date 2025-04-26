#include "modules/process.h"
#include "modules/solucao_inicial.h"
#include "algorithm/dinkelbach_algorithm.h"
#include <iostream>
#include <chrono>

bool isTimeExpired(const std::chrono::high_resolution_clock::time_point& startTime, double timeLimit) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
    return elapsedTime >= timeLimit;
}

bool process(const Warehouse& warehouse, Solution& solution, double timeLimit) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "Iniciando processamento principal..." << std::endl;
    
    // 1. Verificar se já temos uma solução inicial
    if (solution.getSelectedOrders().empty()) {
        std::cout << "    Gerando solução inicial..." << std::endl;
        if (!gerarSolucaoInicial(warehouse, solution)) {
            std::cerr << "    Falha ao gerar solução inicial" << std::endl;
            return false;
        }
    }
    
    // Verificar tempo a cada iteração
    if (isTimeExpired(startTime, timeLimit)) {
        std::cout << "Tempo limite atingido, interrompendo processamento" << std::endl;
        return false;
    }
    
    // 2. Otimizar a solução usando o algoritmo Dinkelbach
    std::cout << "    Otimizando solução com algoritmo Dinkelbach..." << std::endl;
    DinkelbachAlgorithm dinkelbach;
    Solution optimizedSolution = dinkelbach.optimize(warehouse, solution);
    
    // Verificar tempo a cada iteração
    if (isTimeExpired(startTime, timeLimit)) {
        std::cout << "Tempo limite atingido, interrompendo processamento" << std::endl;
        return false;
    }
    
    // 3. Atualizar a solução original com a otimizada
    solution = optimizedSolution;
    
    std::cout << "Processamento principal concluído:" << std::endl;
    std::cout << "    Valor da função objetivo: " << solution.getObjectiveValue() << std::endl;
    std::cout << "    Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
    std::cout << "    Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
    std::cout << "    Total de itens: " << solution.getTotalItems() << std::endl;
    
    return true;
}