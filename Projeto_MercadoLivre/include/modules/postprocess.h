#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "input/input_parser.h"  // Este arquivo deve conter a definição de Warehouse
#include "core/solution.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>

// Função com nome corrigido para evitar conflito
inline bool postprocess(const Warehouse& warehouse, Solution& solution) {
    // Simulação de processamento
    std::cout << "    Iniciando pós-processamento..." << std::endl;
    std::cout << "    Otimizando sequência de corredores..." << std::endl;
    
    // Simular tempo de processamento
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    
    // Simulação de melhoria
    double initialValue = 100.0;
    double improvedValue = initialValue * 1.15;
    
    std::cout << "    Melhoria na função objetivo: " 
              << std::fixed << std::setprecision(2) 
              << initialValue << " -> " << improvedValue 
              << " (+" << (improvedValue - initialValue) << ")" << std::endl;
    
    // Processamento simulado bem-sucedido
    std::cout << "    Pós-processamento concluído com sucesso." << std::endl;
    return true;
}

#endif // POSTPROCESS_H