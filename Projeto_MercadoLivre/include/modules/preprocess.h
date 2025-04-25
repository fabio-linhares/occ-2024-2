#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "input/input_parser.h"  // Este arquivo deve conter a definição de Warehouse
#include "core/solution.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <random>

// Função com nome corrigido para evitar conflito
inline bool preprocess(const Warehouse& warehouse, Solution& solution) {
    // Simulação de processamento
    std::cout << "    Iniciando pré-processamento dos dados..." << std::endl;
    
    // Número aleatório de pedidos filtrados (simulação)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(warehouse.numOrders / 5, warehouse.numOrders / 2);
    int filteredOrders = distrib(gen);
    
    std::cout << "    Filtrando pedidos incompatíveis..." << std::endl;
    std::cout << "    Reduzindo espaço de busca: " << filteredOrders 
              << " pedidos pré-selecionados." << std::endl;
    
    // Simular tempo de processamento
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Processamento simulado bem-sucedido
    std::cout << "    Pré-processamento concluído com sucesso." << std::endl;
    return true;
}

#endif // PREPROCESS_H