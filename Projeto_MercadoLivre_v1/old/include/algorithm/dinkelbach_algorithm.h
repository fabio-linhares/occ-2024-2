#ifndef DINKELBACH_ALGORITHM_H
#define DINKELBACH_ALGORITHM_H

#include "algorithm/optimization_algorithm.h"
#include "modules/cria_auxiliares.h"
#include <vector>
#include <random>

class DinkelbachAlgorithm : public OptimizationAlgorithm {
public:
    DinkelbachAlgorithm();
    virtual ~DinkelbachAlgorithm() = default;
    
    // Interface da classe base
    virtual Solution solve(const Warehouse& warehouse) override;
    
    // Método de otimização a partir de uma solução existente
    Solution optimize(const Warehouse& warehouse, 
                      const Solution& initialSolution,
                      int maxIter = 1000,
                      double timeLimit = 360.0);
    
    // Método para resolver a partir de uma solução existente
    bool solveFromExisting(const Warehouse& warehouse, Solution& solution);
    
private:
    // Parâmetros do algoritmo
    double epsilon;          // Critério de parada para Dinkelbach
    int maxIterations;       // Número máximo de iterações
    int maxNoImprovement;    // Número máximo de iterações sem melhoria
    double initialTemp;      // Temperatura inicial para SA
    double coolingRate;      // Taxa de resfriamento para SA
    
    // Métodos para Dinkelbach
    double calculateRatio(const Solution& solution);
    bool iterativeDinkelbach(const Warehouse& warehouse, Solution& solution);
    bool iterativeDinkelbach(const Warehouse& warehouse, Solution& solution, double timeLimit);
    
    // Métodos para busca local e perturbação
    bool localSearch(const Warehouse& warehouse, Solution& solution, double currentRatio);
    void perturbSolution(const Warehouse& warehouse, Solution& solution, double temperature);
    
    // Movimentos básicos
    bool trySwapOrders(const Warehouse& warehouse, Solution& solution, int orderToRemove, int orderToAdd);
    bool tryAddOrder(const Warehouse& warehouse, Solution& solution, int orderToAdd);
    bool tryRemoveOrder(const Warehouse& warehouse, Solution& solution, int orderToRemove);
    double simulateMovementImpact(const Warehouse& warehouse, const Solution& solution, 
                                  int orderToRemove, int orderToAdd);
    std::mt19937 rng;
};

#endif // DINKELBACH_ALGORITHM_H