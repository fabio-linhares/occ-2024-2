#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "input/input_parser.h"  
#include "core/solution.h"
#include "config/constraints_manager.h"  // Adicionando gerenciador de restrições
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <iomanip>
#include <unordered_map>
#include <set>

// Include header for initial solution processing
#include "modules/solucao_inicial.h"

// Função para verificações detalhadas de viabilidade
inline bool validateSolutionConstraints(const Warehouse& warehouse, Solution& solution) {
    bool isValid = true;
    std::cout << "    Verificando restrições da solução..." << std::endl;
    
    // 1. Verificar limites de wave (LB/UB)
    int totalItems = solution.getTotalItems();
    if (totalItems < warehouse.LB || totalItems > warehouse.UB) {
        std::cout << "    VIOLAÇÃO: Total de itens (" << totalItems 
                  << ") fora dos limites (LB: " << warehouse.LB << ", UB: " << warehouse.UB << ")" << std::endl;
        isValid = false;
    }
    
    // 2. Verificar completude dos pedidos
    const auto& selectedOrders = solution.getSelectedOrders();
    const auto& visitedCorridors = solution.getVisitedCorridors();
    
    // Mapear itens disponíveis nos corredores visitados
    std::unordered_map<int, int> availableItems;
    for (int corridorId : visitedCorridors) {
        for (const auto& item : warehouse.corridors[corridorId]) {
            int itemId = item.first;
            int qty = item.second;
            availableItems[itemId] += qty;
        }
    }
    
    // Verificar se todos os itens dos pedidos estão disponíveis
    for (int orderId : selectedOrders) {
        bool orderComplete = true;
        
        // Verifica cada item do pedido
        for (const auto& item : warehouse.orders[orderId]) {
            int itemId = item.first;
            int requiredQty = item.second;
            
            if (availableItems.find(itemId) == availableItems.end() || 
                availableItems[itemId] < requiredQty) {
                std::cout << "    VIOLAÇÃO: Pedido #" << orderId << " - Item #" << itemId 
                          << " insuficiente (necessário: " << requiredQty 
                          << ", disponível: " << (availableItems.count(itemId) ? availableItems[itemId] : 0)
                          << ")" << std::endl;
                orderComplete = false;
                isValid = false;
            }
            
            // Deduzir a quantidade utilizada para este pedido
            if (availableItems.count(itemId)) {
                availableItems[itemId] -= requiredQty;
            }
        }
        
        if (!orderComplete) {
            std::cout << "    VIOLAÇÃO: Pedido #" << orderId << " está incompleto" << std::endl;
        }
    }
    
    // 3. Verificar corredores distintos (sem duplicatas)
    std::set<int> uniqueCorridors(visitedCorridors.begin(), visitedCorridors.end());
    if (uniqueCorridors.size() != visitedCorridors.size()) {
        std::cout << "    VIOLAÇÃO: Corredores duplicados na solução" << std::endl;
        isValid = false;
    }
    
    // 4. Verificar pedidos distintos (sem duplicatas)
    std::set<int> uniqueOrders(selectedOrders.begin(), selectedOrders.end());
    if (uniqueOrders.size() != selectedOrders.size()) {
        std::cout << "    VIOLAÇÃO: Pedidos duplicados na solução" << std::endl;
        isValid = false;
    }
    
    // 5. Verificar através do ConstraintsManager (se disponível)
    try {
        ConstraintsManager constraintsManager;
        if (!constraintsManager.validate(solution, warehouse)) {
            std::cout << "    VIOLAÇÃO: ConstraintsManager reportou violações adicionais" << std::endl;
            isValid = false;
        }
    } catch (const std::exception& e) {
        std::cout << "    Aviso: Falha ao validar usando ConstraintsManager: " << e.what() << std::endl;
    }
    
    // Definir flag de viabilidade e retornar resultado
    solution.setFeasible(isValid);
    return isValid;
}

inline bool preprocess(const Warehouse& warehouse, Solution& solution) {
    // Iniciar cronômetro
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Chamada para o processamento da solução inicial
    std::cout << "    Gerando solução inicial relaxada..." << std::endl;
    
    // Aqui chamamos a função de solução inicial do arquivo solucao_inicial.cpp
    gerarSolucaoInicial(warehouse, solution);
    
    // Atualizar a solução com os dados da Warehouse
    solution.updateCorridors(warehouse);
    
    // Verificar TODAS as restrições usando a nova função
    if (!validateSolutionConstraints(warehouse, solution)) {
        std::cout << "    Solução inicial não atende a todas as restrições." << std::endl;
        // Não retornamos false aqui para permitir análise da solução inviável
    }
    
    // Exibir estatísticas da solução inicial gerada
    double initialObjectiveValue = solution.calculateObjectiveValue(warehouse);
    std::cout << "    Solução Inicial - Valor da função objetivo: " << std::fixed << std::setprecision(2) 
              << initialObjectiveValue << std::endl;
    std::cout << "    Solução Inicial - Total de itens: " << solution.getTotalItems() << std::endl;
    std::cout << "    Solução Inicial - Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
    std::cout << "    Solução Inicial - Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
    std::cout << "    Solução Inicial - Viável: " << (solution.isFeasible() ? "Sim" : "Não") << std::endl;
    
    // Parar cronômetro e calcular o tempo decorrido
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "    Tempo de execução real: " << duration.count() << " ms" << std::endl;
    
    // Processamento completo, mas pode ser inviável
    std::cout << "    Pré-processamento concluído " 
              << (solution.isFeasible() ? "com sucesso." : "mas solução é INVIÁVEL.") << std::endl;
    
    // Retornar verdadeiro se a solução for viável
    return solution.isFeasible();
}

#endif // PREPROCESS_H