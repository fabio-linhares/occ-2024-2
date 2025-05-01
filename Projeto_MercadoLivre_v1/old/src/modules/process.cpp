#include "modules/process.h"
#include "modules/solucao_inicial.h"
#include "modules/solucao_valida_inicial.h"
#include "algorithm/dinkelbach_algorithm.h"
#include "utils/time_utils.h"
#include <iostream>
#include <chrono>
#include <map>

bool process(const Warehouse& warehouse, Solution& solution, double timeLimit) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "Iniciando processamento principal..." << std::endl;
    
    // 1. Gerar solução inicial VÁLIDA
    std::cout << "    Gerando solução inicial válida..." << std::endl;
    if (!gerarSolucaoInicialValida(warehouse, solution)) {
        std::cerr << "    Falha ao gerar solução inicial válida" << std::endl;
        return false;
    }
    
    // Verificar limites de tempo
    if (isTimeExpired(startTime, timeLimit)) {
        std::cout << "Tempo limite atingido, interrompendo processamento" << std::endl;
        return true; // Retornamos true pois temos uma solução válida
    }
    
    // 2. Aplicar otimização para melhorar a solução (mantendo a viabilidade)
    // Esta etapa pode ser implementada posteriormente
    
    return true;
}

bool processSingleInstance(const std::string& inputFile, const std::string& outputFile) {
    try {
        // Código existente...
        
        // VALIDAÇÃO FINAL antes de retornar
        validarSolucaoFinal(warehouse, solution);
        
        // Resto do código...
        return true;
    } catch (...) {
        // Tratamento de erro existente...
    }
}

// Nova função para validação final
void validarSolucaoFinal(const Warehouse& warehouse, Solution& solution) {
    std::cout << "\n=== VALIDAÇÃO FINAL DA SOLUÇÃO ===\n";
    
    // 1. Calcular total de itens
    int totalItens = 0;
    // Uso orders diretamente em vez de getOrders()
    for (int pedidoId : solution.getSelectedOrders()) {
        for (const auto& [itemId, quantidade] : warehouse.orders[pedidoId]) {
            totalItens += quantidade;
        }
    }
    
    // 2. Verificar LB/UB
    bool lbOk = (totalItens >= warehouse.LB);
    bool ubOk = (totalItens <= warehouse.UB);
    
    // 3. Verificar disponibilidade
    std::map<int, int> demanda;
    std::map<int, int> disponivel;
    
    // Calcular demanda
    for (int pedidoId : solution.getSelectedOrders()) {
        for (const auto& [itemId, quantidade] : warehouse.orders[pedidoId]) {
            demanda[itemId] += quantidade;
        }
    }
    
    // Calcular disponibilidade
    // Usar getVisitedCorridors() em vez de getCorridors()
    for (int corridorId : solution.getVisitedCorridors()) {
        for (const auto& [itemId, quantidade] : warehouse.corridors[corridorId]) {
            disponivel[itemId] += quantidade;
        }
    }
    
    // Verificar se há disponibilidade para todos os itens
    bool disponibilidadeOk = true;
    for (const auto& [itemId, necessario] : demanda) {
        if (disponivel[itemId] < necessario) {
            disponibilidadeOk = false;
            std::cout << "ERRO: Disponibilidade insuficiente para item " << itemId 
                      << ". Necessário: " << necessario 
                      << ", Disponível: " << disponivel[itemId] << std::endl;
        }
    }
    
    // 4. Verificar IDs
    bool idsOk = true;
    for (int pedidoId : solution.getSelectedOrders()) {
        if (pedidoId < 0 || pedidoId >= warehouse.numOrders) {
            idsOk = false;
            std::cout << "ERRO: ID de pedido inválido: " << pedidoId << std::endl;
        }
    }
    
    for (int corridorId : solution.getVisitedCorridors()) {
        if (corridorId < 0 || corridorId >= warehouse.numCorridors) {
            idsOk = false;
            std::cout << "ERRO: ID de corredor inválido: " << corridorId << std::endl;
        }
    }
    
    // Resultado da validação
    std::cout << "- LB (" << warehouse.LB << "): " << (lbOk ? "OK" : "FALHA") << "\n";
    std::cout << "- UB (" << warehouse.UB << "): " << (ubOk ? "OK" : "FALHA") << "\n";
    std::cout << "- Disponibilidade: " << (disponibilidadeOk ? "OK" : "FALHA") << "\n";
    std::cout << "- IDs válidos: " << (idsOk ? "OK" : "FALHA") << "\n";
    
    bool solucaoValida = lbOk && ubOk && disponibilidadeOk && idsOk;
    std::cout << "RESULTADO FINAL: " << (solucaoValida ? "SOLUÇÃO VÁLIDA" : "SOLUÇÃO INVÁLIDA") << "\n";
    
    // Definir status de viabilidade
    solution.setFeasible(solucaoValida);
}