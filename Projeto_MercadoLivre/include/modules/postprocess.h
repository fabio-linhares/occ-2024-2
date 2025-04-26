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
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Chamada para o processamento da solução inicial
    std::cout << "    Gerando solução inicial..." << std::endl;
    
    // Aqui chamamos a função de solução inicial do arquivo solucao_inicial.cpp
    // Assumindo que existe uma função como gerarSolucaoInicial
    gerarSolucaoInicial(warehouse, solution);
    
    // Verificar se a solução inicial é viável
    if (!solution.isFeasible()) {
        std::cout << "    Solução inicial não viável." << std::endl;
        return false;
    }
    // Atualizar a solução com os dados da Warehouse
    solution.updateCorridors(warehouse);
    
    // Exibir estatísticas da solução inicial gerada
    double initialObjectiveValue = solution.calculateObjectiveValue(warehouse);
    std::cout << "    Solução Inicial - Valor da função objetivo: " << std::fixed << std::setprecision(2) 
              << initialObjectiveValue << std::endl;
    std::cout << "    Solução Inicial - Total de itens: " << solution.getTotalItems() << std::endl;
    std::cout << "    Solução Inicial - Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
    std::cout << "    Solução Inicial - Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
    std::cout << "    Solução Inicial - Viável: " << (solution.isFeasible() ? "Sim" : "Não") << std::endl;
    std::cout << "    Tempo de geração da solução inicial: " 
              << std::fixed << std::setprecision(2) 
              << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count() 
              << " ms" << std::endl;
    
    // Parar cronômetro e calcular o tempo decorrido
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "    Tempo de execução real: " << duration.count() << " ms" << std::endl;
    
    // Processamento bem-sucedido
    std::cout << "    Pós-processamento concluído com sucesso." << std::endl;
    return true;
}

#endif // POSTPROCESS_H