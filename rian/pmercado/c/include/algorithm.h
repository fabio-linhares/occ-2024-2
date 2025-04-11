#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "data_structures.h"
#include "config_structures.h"

// Carregar a configuração do algoritmo
AlgorithmConfig loadAlgorithmConfig();

// Implementação do algoritmo de Dinkelbach
Solution dinkelbachAlgorithm(const Instance& instancia, double epsilon, int maxIterations);

#endif // ALGORITHM_H