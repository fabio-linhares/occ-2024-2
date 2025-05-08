#ifndef SOLUCAO_INICIAL_H
#define SOLUCAO_INICIAL_H

#include "core/solution.h"
#include "input/input_parser.h"
#include "modules/cria_auxiliares.h"

// Função principal para criar uma solução inicial de qualidade
// Usando o nome esperado pelo app_controller.cpp
bool gerarSolucaoInicial(const Warehouse& warehouse, Solution& solution);

#endif // SOLUCAO_INICIAL_H