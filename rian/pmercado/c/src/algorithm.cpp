#include "../include/algorithm.h"
#include "../include/config_manager.h"
#include "../include/objective_function.h"
#include "../include/constraints.h"
#include <cmath>
#include <algorithm>
#include <iostream>

AlgorithmConfig loadAlgorithmConfig() {
    return ConfigManager::getInstance().getAlgorithmConfig();
}

// Implementação simplificada do algoritmo de Dinkelbach
Solution dinkelbachAlgorithm(const Instance& instancia, double epsilon, int maxIterations) {
    AlgorithmConfig config = loadAlgorithmConfig();
    if (epsilon <= 0) epsilon = config.epsilon;
    if (maxIterations <= 0) maxIterations = config.max_iterations;
    
    Solution melhorSolucao;
    melhorSolucao.valor_objetivo = 0.0;
    
    double q = 0.0;  // Valor inicial para q
    int iteracao = 0;
    
    while (iteracao < maxIterations) {
        // Aqui viria a implementação real do algoritmo Dinkelbach
        // Esta é uma implementação simplificada para demonstração
        
        // Simular encontrar uma solução para F(q) = max {N(x) - q * D(x)}
        Solution solucaoAtual;
        
        // Codigo que encontraria os melhores pedidos e corredores
        // ... (algoritmo real de otimização)
        
        // Aqui apenas populamos com dados de exemplo
        solucaoAtual.pedidos_selecionados = {0, 1, 2};  // Exemplo
        solucaoAtual.corredores_visitados = {0, 1};     // Exemplo
        
        // Calcular N(x) e D(x)
        int N = 0;  // Número total de itens
        for (int pedido_id : solucaoAtual.pedidos_selecionados) {
            N += instancia.pedidos[pedido_id].total_itens;
        }
        
        int D = solucaoAtual.corredores_visitados.size();  // Número de corredores
        
        // Verificar se é uma solução válida
        if (D > 0 && verificarTodasRestricoes(solucaoAtual.pedidos_selecionados, 
                                              solucaoAtual.corredores_visitados, 
                                              instancia)) {
            
            double valorAtual = static_cast<double>(N) / D;
            
            // Verificar se encontramos uma solução melhor
            if (valorAtual > melhorSolucao.valor_objetivo) {
                melhorSolucao = solucaoAtual;
                melhorSolucao.valor_objetivo = valorAtual;
            }
            
            // Calcular F(q)
            double F = N - q * D;
            
            // Verificar convergência
            if (std::abs(F) < epsilon) {
                break;
            }
            
            // Atualizar q
            q = static_cast<double>(N) / D;
        }
        
        iteracao++;
    }
    
    return melhorSolucao;
}
