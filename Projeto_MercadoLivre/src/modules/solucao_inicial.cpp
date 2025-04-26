#include "modules/solucao_inicial.h"
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <iostream>
#include <thread>
#include <vector>
#include <execution>  // C++17 ou posterior

bool gerarSolucaoInicial(const Warehouse& warehouse, Solution& solution) {
    std::cout << "Construindo solução inicial..." << std::endl;
    
    // Obter estruturas auxiliares
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    
    // Inicializar contadores
    int totalItemsAtual = 0;
    
    // Implementar redução paralela para encontrar valores máximos
    unsigned int numThreads = std::thread::hardware_concurrency();
    std::vector<double> maxEfficiencyPerThread(numThreads, 0.0);
    std::vector<double> maxDensityPerThread(numThreads, 0.0);

    auto findMaxValues = [&](int threadId, int start, int end) {
        for (int i = start; i < end; i++) {
            maxEfficiencyPerThread[threadId] = std::max(
                maxEfficiencyPerThread[threadId], 
                aux.weights.orderEfficiencyRatio[i]
            );
            maxDensityPerThread[threadId] = std::max(
                maxDensityPerThread[threadId], 
                aux.weights.orderUnitDensity[i]
            );
        }
    };

    // Executar em paralelo e combinar resultados
    std::vector<std::thread> threads;
    int batchSize = warehouse.numOrders / numThreads;

    for (unsigned int t = 0; t < numThreads; t++) {
        int start = t * batchSize;
        int end = (t == numThreads - 1) ? warehouse.numOrders : (t + 1) * batchSize;
        threads.push_back(std::thread(findMaxValues, t, start, end));
    }

    for (auto& t : threads) {
        t.join();
    }

    double maxEfficiency = *std::max_element(maxEfficiencyPerThread.begin(), maxEfficiencyPerThread.end());
    double maxDensity = *std::max_element(maxDensityPerThread.begin(), maxDensityPerThread.end());
    
    // Calcular scores combinados
    std::vector<std::pair<int, double>> combinedScores(warehouse.numOrders);

    // Função para processar um lote de pedidos
    auto processOrders = [&](int start, int end) {
        for (int i = start; i < end; i++) {
            double normEfficiency = maxEfficiency > 0 ? 
                aux.weights.orderEfficiencyRatio[i] / maxEfficiency : 0;
            double normDensity = maxDensity > 0 ? 
                aux.weights.orderUnitDensity[i] / maxDensity : 0;
            
            double score = 0.7 * normEfficiency + 0.3 * normDensity;
            combinedScores[i] = {i, score};
        }
    };

    // Dividir o trabalho entre threads
    threads.clear();
    for (unsigned int t = 0; t < numThreads; t++) {
        int start = t * batchSize;
        int end = (t == numThreads - 1) ? warehouse.numOrders : (t + 1) * batchSize;
        threads.push_back(std::thread(processOrders, start, end));
    }

    // Aguardar conclusão
    for (auto& t : threads) {
        t.join();
    }
    
    // Ordenar por score combinado (do maior para o menor)
    std::sort(std::execution::par, combinedScores.begin(), combinedScores.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Primeira fase: adicionar pedidos mais eficientes até atingir LB
    std::cout << "Fase 1: Adicionando pedidos eficientes..." << std::endl;
    for (const auto& [orderIdx, score] : combinedScores) {
        // Pular pedidos com score zero
        if (score <= 0) continue;
        
        // Verificar se adicionar este pedido não ultrapassa UB
        int novoTotal = totalItemsAtual + aux.totalItemsPerOrder[orderIdx];
        if (novoTotal <= warehouse.UB) {
            solution.addOrder(orderIdx, warehouse);
            totalItemsAtual = solution.getTotalItems();
            
            std::cout << "  Adicionado pedido #" << orderIdx 
                      << " (Score: " << score << ", Total: " << totalItemsAtual << ")" << std::endl;
        }
        
        // Verificar se atingimos LB
        if (totalItemsAtual >= warehouse.LB)
            break;
    }
    
    // Segunda fase (opcional): se não atingimos LB, usar estratégia de diversidade
    if (totalItemsAtual < warehouse.LB) {
        std::cout << "Fase 2: Adicionando pedidos para aumentar diversidade..." << std::endl;
        
        // Monitorar itens já cobertos
        std::unordered_set<int> coveredItems;
        for (int orderIdx : solution.getSelectedOrders()) {
            for (int itemId : aux.itemsInOrder[orderIdx]) {
                coveredItems.insert(itemId);
            }
        }
        
        // Processar pedidos restantes, priorizando os que adicionam mais itens novos
        for (int orderIdx = 0; orderIdx < warehouse.numOrders; orderIdx++) {
            // Pular pedidos já incluídos
            if (std::find(solution.getSelectedOrders().begin(), 
                         solution.getSelectedOrders().end(), orderIdx) != solution.getSelectedOrders().end())
                continue;
            
            // Contar novos itens
            int novosItens = 0;
            for (int itemId : aux.itemsInOrder[orderIdx]) {
                if (coveredItems.count(itemId) == 0) novosItens++;
            }
            
            // Adicionar se traz novos itens e respeita UB
            if (novosItens > 0 && 
                totalItemsAtual + aux.totalItemsPerOrder[orderIdx] <= warehouse.UB) {
                
                solution.addOrder(orderIdx, warehouse);
                totalItemsAtual = solution.getTotalItems();
                
                // Atualizar itens cobertos
                for (int itemId : aux.itemsInOrder[orderIdx]) {
                    coveredItems.insert(itemId);
                }
                
                std::cout << "  Adicionado pedido #" << orderIdx 
                          << " (Novos itens: " << novosItens << ", Total: " << totalItemsAtual << ")" << std::endl;
            }
            
            // Verificar se atingimos LB
            if (totalItemsAtual >= warehouse.LB)
                break;
        }
    }
    
    // Verificar factibilidade
    bool factivel = (totalItemsAtual >= warehouse.LB && totalItemsAtual <= warehouse.UB);
    solution.setFeasible(factivel);
    
    std::cout << "Solução inicial construída:" << std::endl;
    std::cout << "  Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
    std::cout << "  Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
    std::cout << "  Total de itens: " << totalItemsAtual << " (LB: " << warehouse.LB << ", UB: " << warehouse.UB << ")" << std::endl;
    std::cout << "  Função objetivo: " << solution.getObjectiveValue() << std::endl;
    std::cout << "  Factibilidade: " << (factivel ? "SIM" : "NÃO") << std::endl;
    
    return factivel;
}