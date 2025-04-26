#include "algorithm/dinkelbach_algorithm.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <chrono>
#include <random>
#include <numeric>
#include "modules/cria_auxiliares.h"
#include "modules/solucao_inicial.h"
#include "core/warehouse.h"
#include "core/solution.h"
#include "algorithm/optimization_algorithm.h"
#include "modules/process.h"

  
// Implementação do construtor da classe DinkelbachAlgorithm
// Inicializa os parâmetros do algoritmo
DinkelbachAlgorithm::DinkelbachAlgorithm() 
    : epsilon(1e-6), maxIterations(1000), maxNoImprovement(100),
      initialTemp(100.0), coolingRate(0.97) {
    
    // Inicializar gerador de números aleatórios com semente baseada no tempo
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng = std::mt19937(seed);
}

// Implementação da interface da classe base
Solution DinkelbachAlgorithm::solve(const Warehouse& warehouse) {
    Solution emptySolution;
    // Criar uma solução inicial com o módulo gerarSolucaoInicial
    if (!gerarSolucaoInicial(warehouse, emptySolution)) {
        std::cerr << "Falha ao gerar solução inicial" << std::endl;
        return emptySolution;
    }
    
    // Otimizar a solução inicial
    return optimize(warehouse, emptySolution);
}

Solution DinkelbachAlgorithm::optimize(const Warehouse& warehouse, 
                                    const Solution& initialSolution,
                                    int maxIter,
                                    double timeLimit) {
    // Copiar a solução inicial
    Solution solution = initialSolution;
    
    // Configurar o número máximo de iterações
    this->maxIterations = maxIter;
    
    // Iniciar cronômetro
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Aplicar o algoritmo de Dinkelbach COM TEMPO LIMITE
    bool success = iterativeDinkelbach(warehouse, solution, timeLimit);
    
    // Verificar tempo decorrido
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        currentTime - startTime).count();
    
    std::cout << "Otimização " << (success ? "concluída" : "interrompida") 
              << " em " << elapsedSeconds << " segundos" << std::endl;
    std::cout << "Valor final da função objetivo: " << solution.getObjectiveValue() << std::endl;
    
    return solution;
}

bool DinkelbachAlgorithm::solveFromExisting(const Warehouse& warehouse, Solution& solution) {
    std::cout << "Iniciando algoritmo Dinkelbach + ILS/SA..." << std::endl;
    
    // Cronometrar execução
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Algoritmo principal de Dinkelbach
    bool success = iterativeDinkelbach(warehouse, solution);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Algoritmo concluído em " << duration.count() << "ms" << std::endl;
    std::cout << "Valor final da função objetivo: " << solution.getObjectiveValue() << std::endl;
    
    return success;
}

double DinkelbachAlgorithm::calculateRatio(const Solution& solution) {
    return solution.getObjectiveValue();
}

bool DinkelbachAlgorithm::iterativeDinkelbach(const Warehouse& warehouse, Solution& solution) {
    // Call the 3-parameter version with a default time limit
    double timeLimit = 360.0; // Default 6min time limit
    return iterativeDinkelbach(warehouse, solution, timeLimit);
}

bool DinkelbachAlgorithm::iterativeDinkelbach(const Warehouse& warehouse, Solution& solution, double timeLimit) {
    double bestRatio = calculateRatio(solution);
    double currentRatio = bestRatio;
    Solution bestSolution = solution;
    
    int iterations = 0;
    int noImprovementCount = 0;
    double temperature = initialTemp;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Resto da implementação...
    
    return true;
}

bool DinkelbachAlgorithm::localSearch(const Warehouse& warehouse, Solution& solution, double currentRatio) {
    bool improved = false;
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    
    // 1. Tentar trocar pedidos (swap)
    const auto& selectedOrders = solution.getSelectedOrders();
    std::vector<int> allOrders(warehouse.numOrders);
    std::iota(allOrders.begin(), allOrders.end(), 0);
    
    // Ordenar pedidos não selecionados por eficiência
    std::vector<int> unselectedOrders;
    for (int i = 0; i < warehouse.numOrders; i++) {
        if (std::find(selectedOrders.begin(), selectedOrders.end(), i) == selectedOrders.end()) {
            unselectedOrders.push_back(i);
        }
    }
    
    std::sort(unselectedOrders.begin(), unselectedOrders.end(), 
        [&aux](int a, int b) {
            return aux.weights.orderEfficiencyRatio[a] > aux.weights.orderEfficiencyRatio[b];
        });
    
    // Testar trocas com simulação prévia
    for (int orderToRemove : selectedOrders) {
        for (int orderToAdd : unselectedOrders) {
            double impact = simulateMovementImpact(warehouse, solution, orderToRemove, orderToAdd);
            
            // Se o impacto é positivo, fazer a troca
            if (impact > epsilon) {
                if (trySwapOrders(warehouse, solution, orderToRemove, orderToAdd)) {
                    improved = true;
                    // Recalcular razão após movimento bem-sucedido
                    double newRatio = calculateRatio(solution);
                    if (newRatio > currentRatio) {
                        return true; // First improvement strategy
                    }
                }
            }
        }
    }
    
    // 2. Tentar adicionar pedidos (se houver espaço)
    for (int orderToAdd : unselectedOrders) {
        if (tryAddOrder(warehouse, solution, orderToAdd)) {
            improved = true;
            double newRatio = calculateRatio(solution);
            if (newRatio > currentRatio) {
                return true;
            }
        }
    }
    
    // 3. Tentar remover pedidos (se estiver acima do LB)
    std::vector<std::pair<int, double>> orderContributions;
    for (int orderIdx : selectedOrders) {
        orderContributions.push_back({orderIdx, aux.weights.orderContributionScore[orderIdx]});
    }
    
    std::sort(orderContributions.begin(), orderContributions.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    for (const auto& [orderToRemove, score] : orderContributions) {
        if (tryRemoveOrder(warehouse, solution, orderToRemove)) {
            improved = true;
            double newRatio = calculateRatio(solution);
            if (newRatio > currentRatio) {
                return true;
            } else {
                // Se piorou, desfazer
                tryAddOrder(warehouse, solution, orderToRemove);
            }
        }
    }
    
    return improved;
}

double DinkelbachAlgorithm::simulateMovementImpact(const Warehouse& warehouse, 
                                                 const Solution& solution,
                                                 int orderToRemove, int orderToAdd) {
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    
    int currentItems = solution.getTotalItems();
    int currentCorridors = solution.getVisitedCorridors().size();
    
    // Simular removendo um pedido
    int itemsAfterRemoval = currentItems - aux.totalItemsPerOrder[orderToRemove];
    
    // Conjunto de corredores após remoção (simulação)
    std::unordered_set<int> corridorSet;
    
    // Identificar quais corredores seriam necessários após remoção
    for (int orderId : solution.getSelectedOrders()) {
        if (orderId != orderToRemove) {
            for (size_t corridor = 0; corridor < aux.orderCorridorCoverage[orderId].size(); ++corridor) {
                if (aux.orderCorridorCoverage[orderId].test(corridor)) {
                    corridorSet.insert(static_cast<int>(corridor));
                }
            }
        }
    }
    
    int corridorsAfterRemoval = corridorSet.size();
    
    // Simular adicionando novo pedido
    int itemsAfterAddition = itemsAfterRemoval + aux.totalItemsPerOrder[orderToAdd];
    
    // Adicionar corredores do novo pedido
    for (size_t corridor = 0; corridor < aux.orderCorridorCoverage[orderToAdd].size(); ++corridor) {
        if (aux.orderCorridorCoverage[orderToAdd].test(corridor)) {
            corridorSet.insert(static_cast<int>(corridor));
        }
    }
    
    int corridorsAfterAddition = corridorSet.size();
    
    // Verificar restrições
    if (itemsAfterAddition < warehouse.LB || itemsAfterAddition > warehouse.UB) {
        return -1.0; // Inviável
    }
    
    // Calcular impacto na função objetivo
    double currentRatio = static_cast<double>(currentItems) / currentCorridors;
    double newRatio = static_cast<double>(itemsAfterAddition) / corridorsAfterAddition;
    
    return newRatio - currentRatio;
}

// Implementação da perturbação (Simulated Annealing)
void DinkelbachAlgorithm::perturbSolution(const Warehouse& warehouse, 
                                        Solution& solution, 
                                        double temperature) {
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    const auto& selectedOrders = solution.getSelectedOrders();
    
    // Não perturbar se há poucos pedidos
    if (selectedOrders.size() <= 2) return;
    
    // 1. Com probabilidade baseada na temperatura, fazer movimentos aleatórios
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::uniform_int_distribution<int> orderDist(0, warehouse.numOrders - 1);
    std::uniform_int_distribution<int> selectedDist(0, selectedOrders.size() - 1);
    
    // Probabilidade de aceitar movimentos piores
    double acceptProbability = std::exp(-1.0 / temperature);
    
    // 2. Realizar movimentos de perturbação
    for (int i = 0; i < 3; i++) {  // Tentar 3 perturbações
        if (dist(rng) < acceptProbability) {
            // Selecionar pedido aleatório para remover
            int idxToRemove = selectedDist(rng);
            int orderToRemove = selectedOrders[idxToRemove];
            
            // Selecionar pedido aleatório para adicionar
            int orderToAdd;
            do {
                orderToAdd = orderDist(rng);
            } while (std::find(selectedOrders.begin(), selectedOrders.end(), orderToAdd) != selectedOrders.end());
            
            // Simular impacto
            double impact = simulateMovementImpact(warehouse, solution, orderToRemove, orderToAdd);
            
            // Aceitar movimento mesmo com impacto negativo, com probabilidade baseada na temperatura
            if (impact > 0 || dist(rng) < acceptProbability) {
                trySwapOrders(warehouse, solution, orderToRemove, orderToAdd);
            }
        }
    }
}

// Implementação das operações de movimento
bool DinkelbachAlgorithm::trySwapOrders(const Warehouse& warehouse, 
                                       Solution& solution, 
                                       int orderToRemove, 
                                       int orderToAdd) {
    // 1. Verificar se o pedido a adicionar já está na solução
    const auto& selectedOrders = solution.getSelectedOrders();
    if (std::find(selectedOrders.begin(), selectedOrders.end(), orderToAdd) != selectedOrders.end()) {
        return false; // orderToAdd já está na solução
    }
    
    // 2. Verificar se o pedido a remover está na solução
    if (std::find(selectedOrders.begin(), selectedOrders.end(), orderToRemove) == selectedOrders.end()) {
        return false; // orderToRemove não está na solução
    }
    
    // 3. Tentar remover e depois adicionar
    solution.removeOrder(orderToRemove, warehouse);
    solution.addOrder(orderToAdd, warehouse);
    
    // 4. Verificar factibilidade
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    int totalItems = solution.getTotalItems();
    
    if (totalItems < warehouse.LB || totalItems > warehouse.UB) {
        // Desfazer movimento se infactível
        solution.removeOrder(orderToAdd, warehouse);
        solution.addOrder(orderToRemove, warehouse);
        return false;
    }
    
    return true;
}

bool DinkelbachAlgorithm::tryAddOrder(const Warehouse& warehouse, 
                                     Solution& solution, 
                                     int orderToAdd) {
    // 1. Verificar se o pedido já está na solução
    const auto& selectedOrders = solution.getSelectedOrders();
    if (std::find(selectedOrders.begin(), selectedOrders.end(), orderToAdd) != selectedOrders.end()) {
        return false; // Já está na solução
    }
    
    // 2. Obter dados auxiliares
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    
    // 3. Verificar se adicionar este pedido mantém a factibilidade
    int currentItems = solution.getTotalItems();
    int orderItems = aux.totalItemsPerOrder[orderToAdd];
    
    if (currentItems + orderItems > warehouse.UB) {
        return false; // Ultrapassa o limite
    }
    
    // 4. Adicionar o pedido
    solution.addOrder(orderToAdd, warehouse);
    
    return true;
}

bool DinkelbachAlgorithm::tryRemoveOrder(const Warehouse& warehouse, 
                                        Solution& solution, 
                                        int orderToRemove) {
    // 1. Verificar se o pedido está na solução
    const auto& selectedOrders = solution.getSelectedOrders();
    if (std::find(selectedOrders.begin(), selectedOrders.end(), orderToRemove) == selectedOrders.end()) {
        return false; // Não está na solução
    }
    
    // 2. Obter dados auxiliares
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    
    // 3. Verificar se remover este pedido mantém a factibilidade
    int currentItems = solution.getTotalItems();
    int orderItems = aux.totalItemsPerOrder[orderToRemove];
    
    if (currentItems - orderItems < warehouse.LB) {
        return false; // Ficaria abaixo do limite
    }
    
    // 4. Remover o pedido
    solution.removeOrder(orderToRemove, warehouse);
    
    return true;
}