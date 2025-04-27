#ifndef PROCESS_H
#define PROCESS_H

#include "input/input_parser.h"
#include "core/solution.h"
#include <thread>
#include <chrono>
#include <iostream>

// Apenas declarar a função, sem implementação
bool process(const Warehouse& warehouse, Solution& solution, double timeLimit = 300.0);

#endif // PROCESS_H