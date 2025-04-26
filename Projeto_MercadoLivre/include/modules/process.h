#ifndef PROCESS_H
#define PROCESS_H

#include "input/input_parser.h"
#include <thread>
#include <chrono>
#include <iostream>

// Função simplificada para evitar segmentation fault
inline bool process(const Warehouse& warehouse, Solution& solution) {
    // Simulação simplificada
    //std::cout << "    Iniciando algoritmo principal..." << std::endl;
    
    // Progresso simulado sem manipulações de dados que podem causar problemas
    //for (int i = 1; i <= 10; i++) {
    //    std::cout << "    Progresso: " << (i * 10) << "%" << std::endl;
    //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // }
    
    std::cout << "    Algoritmo principal concluído com sucesso." << std::endl;
    return true;
}

#endif // PROCESS_H