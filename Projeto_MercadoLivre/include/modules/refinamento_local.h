#ifndef REFINAMENTO_LOCAL_H
#define REFINAMENTO_LOCAL_H

#include "core/solution.h"
#include "input/input_parser.h"
#include "modules/cria_auxiliares.h"

// Realiza busca local para melhorar a solução
bool aplicarBuscaLocal(const Warehouse& warehouse, 
                      AuxiliaryStructures& aux, 
                      Solution& solution,
                      int max_iteracoes = 20);

#endif // REFINAMENTO_LOCAL_H