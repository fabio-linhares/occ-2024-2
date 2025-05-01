#pragma once
#include "core/warehouse.h"
#include "core/solution.h"

class OptimizationAlgorithm {
public:
    virtual ~OptimizationAlgorithm() = default;
    
    // Método principal para resolver o problema
    virtual Solution solve(const Warehouse& warehouse) = 0;
    
    // Método para otimizar a partir de uma solução inicial 
    virtual Solution optimize(const Warehouse& warehouse, 
                             const Solution& initialSolution,
                             int maxIterations = 1000,
                             double timeLimit = 300.0) = 0;
};