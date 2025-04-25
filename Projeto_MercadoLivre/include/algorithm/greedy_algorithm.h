#pragma once
#include "algorithm/optimization_algorithm.h"
#include <map>
#include <vector>
#include <utility>

class GreedyAlgorithm : public OptimizationAlgorithm {
private:
    // Estruturas auxiliares para acelerar o algoritmo
    std::map<int, std::vector<int>> itemToCorridors; // Mapa de item para corredores
    std::map<int, double> orderEfficiency;           // Eficiência de cada pedido
    
public:
    GreedyAlgorithm();
    
    // Constrói as estruturas auxiliares
    void buildAuxiliaryStructures(const Warehouse& warehouse);
    
    // Calcula a eficiência de um pedido (itens/corredores)
    double calculateOrderEfficiency(int orderId, const Warehouse& warehouse);
    
    // Implementações da interface
    Solution solve(const Warehouse& warehouse) override;
    Solution optimize(const Warehouse& warehouse, 
                     const Solution& initialSolution,
                     int maxIterations = 1000,
                     double timeLimit = 300.0) override;
};