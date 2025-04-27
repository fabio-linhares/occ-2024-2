#include "modules/solucao_inicial.h"
#include "modules/selecao_otimizada.h"
#include "modules/cria_auxiliares.h"
#include "modules/refinamento_local.h"
#include "core/solution.h"
#include "input/input_parser.h"
#include "core/warehouse.h"
#include <iostream>
#include <chrono>
#include <stdexcept>

// Implementação da função principal
bool gerarSolucaoInicial(const Warehouse& warehouse, Solution& solution) {
    auto inicio_total = std::chrono::high_resolution_clock::now();
    std::cout << "    Construindo solução inicial com algoritmo otimizado..." << std::endl;
    
    // Obter o número de threads disponíveis
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    
    std::cout << "    Utilizando " << numThreads << " threads para processamento paralelo" << std::endl;
    
    try {
        // Criar estruturas auxiliares diretamente aqui, sem tentar recuperá-las da solução
        AuxiliaryStructures aux;
        
        // Verificar se já temos estruturas auxiliares na solução
        // Se não tivermos, criamos novas
        if (!cria_auxiliares(warehouse, solution)) {
            std::cerr << "Erro ao criar estruturas auxiliares" << std::endl;
            return false;
        }
        
        // A partir daqui, vamos usar nossas próprias estruturas em vez de tentar recuperar da solução
        
        // Inicializar estruturas otimizadas
        inicializarEstruturasAprimoradas(aux, warehouse);
        
        // Calcular métricas avançadas
        calcularMetricasAvancadas(aux);
        
        // Criar nova solução a partir do zero (para evitar problemas de estado)
        solution = Solution();
        
        // Aplicar seleção otimizada com as estruturas que acabamos de criar
        std::vector<std::pair<int, double>> pedidos_priorizados;
        calcularPrioridadePedidos(aux, pedidos_priorizados);
        
        // Realizar a seleção de pedidos com base nas estruturas criadas
        bool atingiuLB = selecionarPedidosOtimizado(warehouse, aux, solution);
        
        // Complementar se necessário para atingir LB
        if (!atingiuLB) {
            selecionarPedidosComplementares(warehouse, aux, solution);
        }
        
        // Atualizar métricas da solução
        solution.calculateObjectiveValue(warehouse);
        
        // Exibir resultados
        std::cout << "    Solução inicial construída com sucesso:" << std::endl;
        std::cout << "      - Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
        std::cout << "      - Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
        std::cout << "      - Total de itens: " << solution.getTotalItems() << std::endl;
        std::cout << "      - Valor objetivo: " << solution.getObjectiveValue() << std::endl;
        
        auto fim_total = std::chrono::high_resolution_clock::now();
        auto duracao_total = std::chrono::duration_cast<std::chrono::milliseconds>(fim_total - inicio_total).count();
        std::cout << "    Tempo total de construção da solução: " << duracao_total << "ms" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Erro durante a geração da solução inicial: " << e.what() << std::endl;
        return false;
    }
}