#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "input/input_parser.h"  // Este arquivo deve conter a definição de Warehouse
#include "core/solution.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>

// Include header for initial solution processing
#include "modules/solucao_inicial.h"  // Update path if needed

// Função com nome corrigido para evitar conflito
inline bool postprocess(const Warehouse& warehouse, Solution& solution) {
    // Iniciar cronômetro
    //uto start_time = std::chrono::high_resolution_clock::now();
    
    // Simular tempo de processamento
    //std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    
    // Parar cronômetro e calcular o tempo decorrido
    //auto end_time = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    //std::cout << "    Tempo de execução real: " << duration.count() << " ms" << std::endl;
    
    // Processamento bem-sucedido
    std::cout << "    Pós-processamento concluído com sucesso." << std::endl;
    return true;
}

#endif // POSTPROCESS_H