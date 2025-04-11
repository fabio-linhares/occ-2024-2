#ifndef OBJECTIVE_FUNCTION_H
#define OBJECTIVE_FUNCTION_H

#include "data_structures.h"
#include "config_structures.h"
#include <vector>

// Carregar a configuração da função objetivo
ObjectiveConfig loadObjectiveConfig();

// Calcular o valor da função objetivo para uma solução
double calcularRazao(const std::vector<int>& pedidos, const std::vector<int>& corredores, const Instance& instancia);

#endif // OBJECTIVE_FUNCTION_H