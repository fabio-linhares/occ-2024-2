#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "data_structures.h"
#include "config_structures.h"
#include <vector>

// Carregar as configurações das restrições
std::vector<ConstraintConfig> loadConstraintConfigs();

// Verificar limite inferior de itens
bool verificarLimiteInferior(const std::vector<int>& pedidos, const Instance& instancia, int lb);

// Verificar limite superior de itens
bool verificarLimiteSuperior(const std::vector<int>& pedidos, const Instance& instancia, int ub);

// Verificar disponibilidade de itens
bool verificarDisponibilidade(const std::vector<int>& pedidos, const std::vector<int>& corredores, const Instance& instancia);

// Verificar todas as restrições
bool verificarTodasRestricoes(const std::vector<int>& pedidos, const std::vector<int>& corredores, const Instance& instancia);

#endif // CONSTRAINTS_H