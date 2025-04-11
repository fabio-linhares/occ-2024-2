#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "data_structures.h"
#include <string>

// Validar uma solução
bool validarSolucao(const Solution& solucao, const Instance& instancia);

// Ler uma solução de um arquivo
Solution lerSolucao(const std::string& filepath, const Instance& instancia);

// Escrever uma solução em um arquivo
void escreverSolucao(const std::string& filepath, const Solution& solucao);

#endif // VALIDATOR_H