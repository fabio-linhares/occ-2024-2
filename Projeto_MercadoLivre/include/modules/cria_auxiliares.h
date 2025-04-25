#ifndef CRIA_AUXILIARES_H
#define CRIA_AUXILIARES_H

#include "input/input_parser.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <algorithm>
#include <set>
#include <mutex>
#include <numeric>
#include <iomanip>  // Para std::fixed e std::setprecision

// Tamanho máximo para bitsets (ajuste conforme necessário)
constexpr size_t MAX_ITEMS = 15000;
constexpr size_t MAX_CORRIDORS = 500;

// Métricas de peso para análise avançada
struct WeightMetrics {
    // Métricas para pedidos
    std::vector<double> orderContributionScore;     // Contribuição estimada de cada pedido para a função objetivo
    std::vector<double> orderEfficiencyRatio;       // Razão itens/corredores para cada pedido
    std::vector<double> orderUnitDensity;           // Unidades por corredor para cada pedido
    std::vector<int> orderRank;                     // Ranking dos pedidos por eficiência
    
    // Métricas para itens
    std::vector<double> itemLeverageScore;          // Importância estratégica de cada item (presença em pedidos eficientes)
    std::vector<double> itemScarcityScore;          // Quão escasso é o item (demanda vs. disponibilidade)
    std::vector<int> itemFrequency;                 // Número de pedidos que solicita cada item
    
    // Métricas para corredores
    std::vector<double> corridorUtilityScore;       // Utilidade de cada corredor (cobertura de itens importantes)
    std::vector<double> corridorDensityScore;       // Densidade de itens por corredor
    std::vector<int> corridorRank;                  // Ranking dos corredores por utilidade
    
    // Métricas específicas para os limites LB/UB
    double targetItemsPerCorridor;                  // Razão ideal de itens/corredores baseada nos limites LB/UB
    std::vector<bool> isWithinLimits;               // Para cada pedido, indica se está dentro dos limites LB/UB
    
    // Métricas combinadas para sugestão de trocas
    std::vector<std::vector<double>> swapImpactMatrix;  // Impacto estimado de trocar um pedido por outro
};

// Estruturas auxiliares que serão armazenadas na solução
struct AuxiliaryStructures {
    // Conjuntos básicos
    std::vector<int> allOrders;           // O: Conjunto de todos os pedidos
    std::unordered_set<int> allItems;     // I: Conjunto de todos os itens
    std::unordered_set<int> allCorridors; // A: Conjunto de todos os corredores
    
    // Mapeamentos de pedidos para itens
    std::vector<std::unordered_set<int>> itemsInOrder;    // I(o): Itens em cada pedido
    std::vector<std::unordered_map<int, int>> orderQuantities; // u(oi): Quantidade de cada item por pedido
    
    // Mapeamentos de itens para corredores
    std::vector<std::unordered_set<int>> corridorsWithItem; // A(i): Corredores que contêm cada item
    std::vector<std::unordered_map<int, int>> corridorQuantities; // u(ai): Quantidade de cada item por corredor
    
    // Eficiência dos pedidos (itens/corredores necessários)
    std::vector<std::pair<int, double>> orderEfficiency;
    
    // Bitsets para operações rápidas de conjunto
    std::vector<std::bitset<MAX_ITEMS>> orderItemsBitset;      // Para cada pedido, quais itens contém
    std::vector<std::bitset<MAX_CORRIDORS>> itemCorridorsBitset; // Para cada item, quais corredores contém
    
    // Matriz de cobertura: se corridorCoverageMatrix[i][j] = true, então o corredor j contém pelo menos um item do pedido i
    std::vector<std::bitset<MAX_CORRIDORS>> orderCorridorCoverage;
    
    // Estatísticas úteis
    std::vector<int> totalItemsPerOrder;  // Número total de unidades por pedido
    std::vector<int> numDiffItemsPerOrder; // Número de itens diferentes por pedido
    std::vector<int> numCorridorsNeededPerOrder; // Número mínimo de corredores necessários por pedido

    // Pré-filtragem de pedidos
    std::vector<int> feasibleOrders;                 // Pedidos que são viáveis (podem ser atendidos)
    std::vector<int> infeasibleOrders;               // Pedidos que não podem ser atendidos
    std::unordered_map<int, std::set<int>> itemsLackingInventory;  // Para cada item, quais pedidos não podem ser atendidos devido a estoque insuficiente

    // Métricas de peso para análise avançada
    WeightMetrics weights;
};

// Matriz esparsa para o problema de set covering
// Cada linha representa um item, cada coluna um corredor
// Útil para algoritmos de programação inteira ou heurísticas gulosas
struct SetCoveringStructure {
    std::vector<std::vector<int>> itemToCorridor;  // Para cada item, quais corredores têm estoque
    std::vector<std::vector<int>> corridorToItem;  // Para cada corredor, quais itens contém
    std::vector<double> corridorWeights;           // Pesos de cada corredor (pode ser baseado no número de itens)
};

// Clustering de pedidos e corredores
struct ClusteringData {
    // Mede a similaridade entre pedidos (quanto mais itens em comum, maior a similaridade)
    std::vector<std::vector<double>> orderSimilarityMatrix;
    
    // Agrupamentos de pedidos que compartilham muitos corredores
    std::vector<std::vector<int>> orderClusters;
    
    // Agrupamentos de corredores que atendem grupos similares de pedidos
    std::vector<std::vector<int>> corridorClusters;
};

// Para algoritmos gulosos que adicionam pedidos/corredores incrementalmente
struct IncrementalStructures {
    // Acompanha o estado atual da solução
    std::unordered_set<int> selectedOrders;
    std::unordered_set<int> selectedCorridors;
    
    // Rastreia o impacto de adicionar cada pedido/corredor à solução atual
    std::vector<double> orderMarginalValue;  // Valor marginal de adicionar cada pedido
    std::vector<double> corridorMarginalValue; // Valor marginal de adicionar cada corredor
    
    // Rastreia pedidos cobertos parcialmente
    std::unordered_map<int, std::unordered_set<int>> orderCoveredItems; // Itens já cobertos por pedido
};

// Estruturas para paralelização
struct ParallelExecutionData {
    size_t numThreads;
    std::vector<std::thread> workers;
    std::mutex solutionMutex;  // Para proteger acessos à solução
    
    // Divisão de trabalho
    std::vector<std::vector<int>> orderPartitions;  // Divisão de pedidos entre threads
    std::vector<std::vector<int>> corridorPartitions;  // Divisão de corredores entre threads
};

// Estrutura para Busca Tabu (definição mantida, mas implementação será em outro arquivo)
struct TabuStructures {
    // Lista tabu (movimentos proibidos)
    std::vector<std::pair<int, int>> tabuList;      // (pedido removido, pedido adicionado)
    std::vector<int> tabuTenure;                    // Por quantas iterações o movimento está proibido
    
    // Memória de frequência (para diversificação)
    std::vector<int> selectionFrequency;            // Frequência com que cada pedido foi selecionado
    std::vector<int> removalFrequency;              // Frequência com que cada pedido foi removido
    
    // Melhores soluções encontradas
    std::vector<std::vector<int>> eliteSolutions;   // Armazena as N melhores soluções encontradas
    std::vector<double> eliteSolutionValues;        // Valores da função objetivo para as soluções de elite
    
    // Critérios de aspiração
    double aspirationThreshold;                     // Valor mínimo para aceitar um movimento tabu
};

inline bool cria_auxiliares(const Warehouse& warehouse, Solution& solution) {
    std::cout << "    Construindo estruturas de dados auxiliares..." << std::endl;
    
    // Determinar o número de threads disponíveis
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;  // Padrão se não for detectado

    std::cout << "    Utilizando " << numThreads << " threads para processamento paralelo" << std::endl;

    // Criar estruturas para a paralelização
    ParallelExecutionData parallelData;
    parallelData.numThreads = numThreads;

    // Iniciar a estrutura auxiliar
    AuxiliaryStructures aux;
    
    // 1. Criar conjuntos básicos
    std::cout << "    Criando conjuntos básicos..." << std::endl;
    
    // Conjunto de pedidos
    for (int i = 0; i < warehouse.numOrders; i++) {
        aux.allOrders.push_back(i);
    }
    
    // Inicializar estruturas de tamanho fixo
    aux.itemsInOrder.resize(warehouse.numOrders);
    aux.orderQuantities.resize(warehouse.numOrders);
    aux.totalItemsPerOrder.resize(warehouse.numOrders, 0);
    aux.numDiffItemsPerOrder.resize(warehouse.numOrders, 0);
    aux.orderItemsBitset.resize(warehouse.numOrders);
    aux.orderCorridorCoverage.resize(warehouse.numOrders);
    
    // 2. Processar pedidos e mapear itens
    std::cout << "    Processando " << warehouse.numOrders << " pedidos..." << std::endl;
    
    for (int orderIdx = 0; orderIdx < warehouse.numOrders; orderIdx++) {
        for (const auto& [itemId, quantity] : warehouse.orders[orderIdx]) {
            // Adicionar o item ao conjunto de todos os itens
            aux.allItems.insert(itemId);
            
            // Mapear o item ao pedido
            aux.itemsInOrder[orderIdx].insert(itemId);
            aux.orderQuantities[orderIdx][itemId] = quantity;
            
            // Atualizar contadores
            aux.totalItemsPerOrder[orderIdx] += quantity;
            aux.numDiffItemsPerOrder[orderIdx]++;
            
            // Atualizar bitset
            if (itemId < MAX_ITEMS) {
                aux.orderItemsBitset[orderIdx].set(itemId);
            }
        }
    }
    
    // 3. Inicializar estruturas de itens
    std::cout << "    Mapeando " << aux.allItems.size() << " itens em " 
              << warehouse.numCorridors << " corredores..." << std::endl;
    
    int maxItemId = 0;
    for (int itemId : aux.allItems) {
        maxItemId = std::max(maxItemId, itemId);
    }
    
    aux.corridorsWithItem.resize(maxItemId + 1);
    aux.corridorQuantities.resize(maxItemId + 1);
    aux.itemCorridorsBitset.resize(maxItemId + 1);
    
    // 4. Processar corredores e mapear itens
    for (int corridorIdx = 0; corridorIdx < warehouse.numCorridors; corridorIdx++) {
        aux.allCorridors.insert(corridorIdx);
        
        for (const auto& [itemId, quantity] : warehouse.corridors[corridorIdx]) {
            // Mapear o corredor ao item
            aux.corridorsWithItem[itemId].insert(corridorIdx);
            aux.corridorQuantities[itemId][corridorIdx] = quantity;
            
            // Atualizar bitset
            if (corridorIdx < MAX_CORRIDORS) {
                aux.itemCorridorsBitset[itemId].set(corridorIdx);
            }
        }
    }
    
    // 5. Calcular cobertura de corredores por pedido
    std::cout << "    Calculando coberturas de corredores por pedido..." << std::endl;
    
    for (int orderIdx = 0; orderIdx < warehouse.numOrders; orderIdx++) {
        std::bitset<MAX_CORRIDORS> orderCorridors;
        
        for (int itemId : aux.itemsInOrder[orderIdx]) {
            // União de todos os corredores que contêm os itens deste pedido
            orderCorridors |= aux.itemCorridorsBitset[itemId];
        }
        
        aux.orderCorridorCoverage[orderIdx] = orderCorridors;
        aux.numCorridorsNeededPerOrder.push_back(orderCorridors.count());
    }
    
    // 6. Calcular eficiência dos pedidos (itens diferentes / corredores necessários)
    std::cout << "    Calculando eficiência dos pedidos..." << std::endl;
    
    for (int orderIdx = 0; orderIdx < warehouse.numOrders; orderIdx++) {
        double efficiency = 0.0;
        int corridorsNeeded = aux.numCorridorsNeededPerOrder[orderIdx];
        
        if (corridorsNeeded > 0) {
            efficiency = static_cast<double>(aux.numDiffItemsPerOrder[orderIdx]) / corridorsNeeded;
        }
        
        aux.orderEfficiency.push_back({orderIdx, efficiency});
    }
    
    // Ordenar pedidos por eficiência (do mais eficiente ao menos eficiente)
    std::sort(aux.orderEfficiency.begin(), aux.orderEfficiency.end(), 
        [](const auto& a, const auto& b) { 
            if (a.second == 0) return false; // Pedidos impossíveis ficam por último
            if (b.second == 0) return true;
            return a.second > b.second; 
        });

    // Inicializar estruturas de pesos e métricas
    WeightMetrics weights;
    weights.orderContributionScore.resize(warehouse.numOrders, 0.0);
    weights.orderEfficiencyRatio.resize(warehouse.numOrders, 0.0);
    weights.orderUnitDensity.resize(warehouse.numOrders, 0.0);
    weights.orderRank.resize(warehouse.numOrders, 0);

    // Calcular métricas para cada pedido
    for (int orderIdx = 0; orderIdx < warehouse.numOrders; orderIdx++) {
        int totalItems = aux.totalItemsPerOrder[orderIdx];
        int diffItems = aux.numDiffItemsPerOrder[orderIdx];
        int corridorsNeeded = aux.numCorridorsNeededPerOrder[orderIdx];
        
        // Calcular razões apenas se o número de corredores necessários > 0
        if (corridorsNeeded > 0) {
            // Eficiência: itens diferentes por corredor
            weights.orderEfficiencyRatio[orderIdx] = static_cast<double>(diffItems) / corridorsNeeded;
            
            // Densidade: unidades totais por corredor
            weights.orderUnitDensity[orderIdx] = static_cast<double>(totalItems) / corridorsNeeded;
        }
        
        // Verificar se está dentro dos limites LB/UB
        bool withinLimits = (totalItems >= warehouse.LB && totalItems <= warehouse.UB);
        
        // Cálculo da contribuição para a função objetivo
        // Aqui presumimos que a função objetivo é maximizar itens/corredores
        double contribution = withinLimits ? weights.orderEfficiencyRatio[orderIdx] : 0.0;
        weights.orderContributionScore[orderIdx] = contribution;
    }

    // Criar ranking dos pedidos por eficiência
    std::vector<int> orderIndices(warehouse.numOrders);
    std::iota(orderIndices.begin(), orderIndices.end(), 0); // Preencher com 0, 1, 2, ...
    std::sort(orderIndices.begin(), orderIndices.end(), 
        [&weights](int a, int b) { 
            return weights.orderContributionScore[a] > weights.orderContributionScore[b];
        });

    // Armazenar o ranking
    for (int i = 0; i < orderIndices.size(); i++) {
        weights.orderRank[orderIndices[i]] = i;
    }

    // Calcular métricas para itens baseadas em sua importância estratégica
    maxItemId = *std::max_element(aux.allItems.begin(), aux.allItems.end());
    weights.itemLeverageScore.resize(maxItemId + 1, 0.0);
    weights.itemScarcityScore.resize(maxItemId + 1, 0.0);
    weights.itemFrequency.resize(maxItemId + 1, 0);

    for (int itemId : aux.allItems) {
        // Contar em quantos pedidos aparece
        int frequency = 0;
        double totalEfficiencyContribution = 0.0;
        
        for (int orderIdx = 0; orderIdx < warehouse.numOrders; orderIdx++) {
            if (aux.itemsInOrder[orderIdx].count(itemId) > 0) {
                frequency++;
                // Soma a eficiência dos pedidos que contêm este item
                totalEfficiencyContribution += weights.orderEfficiencyRatio[orderIdx];
            }
        }
        
        weights.itemFrequency[itemId] = frequency;
        
        // Leverage score = quanto este item contribui para pedidos eficientes
        weights.itemLeverageScore[itemId] = frequency > 0 ? 
            totalEfficiencyContribution / frequency : 0.0;
        
        // Calcular escassez do item (demanda total vs. oferta total)
        int totalDemand = 0;
        int totalSupply = 0;
        
        for (int orderIdx = 0; orderIdx < warehouse.numOrders; orderIdx++) {
            if (aux.orderQuantities[orderIdx].count(itemId) > 0) {
                totalDemand += aux.orderQuantities[orderIdx][itemId];
            }
        }
        
        for (auto& [corridorIdx, quantity] : aux.corridorQuantities[itemId]) {
            totalSupply += quantity;
        }
        
        // Escassez = demanda/oferta (1.0 significa bem equilibrado)
        weights.itemScarcityScore[itemId] = totalSupply > 0 ? 
            static_cast<double>(totalDemand) / totalSupply : 2.0; // 2.0 indica escassez extrema
    }

    // Adicionar à estrutura auxiliar
    aux.weights = weights;

    // 7. Armazenar as estruturas auxiliares na solução
    solution.setAuxiliaryData("structures", aux);
    
    // Exibir algumas estatísticas
    std::cout << "    Estruturas auxiliares criadas com sucesso:" << std::endl;
    std::cout << "      - " << warehouse.numOrders << " pedidos" << std::endl;
    std::cout << "      - " << aux.allItems.size() << " itens únicos" << std::endl;
    std::cout << "      - " << warehouse.numCorridors << " corredores" << std::endl;
    
    // Top 5 pedidos mais eficientes
    std::cout << "      - Top 5 pedidos mais eficientes:" << std::endl;
    for (int i = 0; i < std::min(5, (int)aux.orderEfficiency.size()); i++) {
        if (aux.orderEfficiency[i].second > 0) {
            std::cout << "        #" << aux.orderEfficiency[i].first 
                      << ": " << std::fixed << std::setprecision(2) 
                      << aux.orderEfficiency[i].second << " itens/corredor" << std::endl;
        }
    }
    
    return true;
}

#endif // CRIA_AUXILIARES_H