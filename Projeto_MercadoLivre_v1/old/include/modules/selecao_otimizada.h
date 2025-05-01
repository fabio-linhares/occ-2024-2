#ifndef SELECAO_OTIMIZADA_H
#define SELECAO_OTIMIZADA_H

#include "core/solution.h"
#include "input/input_parser.h"
#include "modules/cria_auxiliares.h"
#include <set>
#include <vector>
#include <limits>
#include <algorithm>
#include <iostream>

// Algoritmo principal de seleção otimizada
bool selecionarPedidosOtimizado(const Warehouse& warehouse, 
                              AuxiliaryStructures& aux,
                              Solution& solution);

// Função para complementar a solução até atingir LB
void selecionarPedidosComplementares(const Warehouse& warehouse, 
                                   AuxiliaryStructures& aux,
                                   Solution& solution);

// Cálculo de prioridade de pedidos
void calcularPrioridadePedidos(AuxiliaryStructures& aux, 
                              std::vector<std::pair<int, double>>& pedidos_priorizados);

#endif // SELECAO_OTIMIZADA_H