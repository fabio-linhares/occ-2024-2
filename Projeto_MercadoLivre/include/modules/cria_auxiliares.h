#ifndef CRIA_AUXILIARES_H
#define CRIA_AUXILIARES_H

#include "input/input_parser.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <bitset>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <cmath>
#include <iomanip>
#include <unordered_map>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <functional>

// Constantes para tamanhos máximos
const int MAX_ITEMS = 100000;
const int MAX_CORRIDORS = 10000;

// Estrutura de métricas de peso
struct WeightMetrics {
    std::vector<double> orderContributionScore;
    std::vector<double> orderEfficiencyRatio;
    std::vector<double> orderUnitDensity;
    std::vector<int> orderRank;
    std::vector<double> itemLeverageScore;
    std::vector<double> itemScarcityScore;
    std::vector<int> itemFrequency;
};

// Estrutura para execução paralela
struct ParallelExecutionData {
    unsigned int numThreads;
};

// Estrutura auxiliar principal
struct AuxiliaryStructures {
    // Tipos aninhados
    struct OrderStatistics {
        double meanEfficiency;
        double stdDevEfficiency;
        double coefficientOfVariation;
        double medianEfficiency;
        std::vector<double> efficiencyQuantiles;
        std::vector<double> efficiencyBins;
        std::vector<int> efficiencyDistribution;
    };

    struct ItemStatistics {
        double meanScarcity;
        double stdDevScarcity;
        double medianScarcity;
        double meanFrequency;
        double stdDevFrequency;
        std::vector<int> highScarcityItems;
        std::vector<int> statSignificantItems;
    };

    // Estruturas otimizadas para o algoritmo aprimorado
    struct ItemInfo {
        int id;
        int frequencia = 0;                      
        int disponibilidade_total = 0;           
        double escassez = 0.0;                     
        std::vector<std::pair<int, int>> corredores;  
        std::vector<int> pedidos_contendo;        
    };

    struct PedidoInfo {
        int id;
        int total_itens = 0;                    
        int num_itens_distintos = 0;            
        std::vector<std::pair<int, int>> itens;  
        std::vector<int> corredores_necessarios; 
        double eficiencia_base = 0.0;            
        double prioridade = 0.0;                
    };

    struct CorredorInfo {
        int id;
        std::vector<std::pair<int, int>> itens;  
        int total_itens_disponiveis = 0;        
        int num_itens_distintos = 0;            
        std::vector<int> pedidos_dependentes;    
    };

    // Variáveis membro
    std::vector<int> allOrders;
    std::unordered_set<int> allItems;
    std::unordered_set<int> allCorridors;
    std::vector<std::unordered_set<int>> itemsInOrder;
    std::vector<std::unordered_map<int, int>> orderQuantities;
    std::vector<int> totalItemsPerOrder;
    std::vector<int> numDiffItemsPerOrder;
    std::vector<std::bitset<MAX_ITEMS>> orderItemsBitset;
    std::vector<std::bitset<MAX_CORRIDORS>> orderCorridorCoverage;
    std::vector<std::unordered_set<int>> corridorsWithItem;
    std::vector<std::unordered_map<int, int>> corridorQuantities;
    std::vector<std::bitset<MAX_CORRIDORS>> itemCorridorsBitset;
    std::vector<int> numCorridorsNeededPerOrder;
    std::vector<std::pair<int, double>> orderEfficiency;
    WeightMetrics weights;
    std::vector<ItemInfo> itens_aprimorado;
    std::vector<PedidoInfo> pedidos_aprimorado;
    std::vector<CorredorInfo> corredores_aprimorado;
};

// Protótipo da função principal
inline bool cria_auxiliares(const Warehouse& warehouse, Solution& solution);

// Protótipos das funções de estatística - use inline para evitar múltiplas definições
inline void calculateOrderStatistics(AuxiliaryStructures& aux, AuxiliaryStructures::OrderStatistics& stats);
inline void calculateItemStatistics(AuxiliaryStructures& aux, AuxiliaryStructures::ItemStatistics& stats);

// Implementações...
inline bool cria_auxiliares(const Warehouse& warehouse, Solution& solution) {
    //std::cout << "    Construindo estruturas de dados auxiliares..." << std::endl;
    
    // Determinar o número de threads disponíveis
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;  // Padrão se não for detectado

    std::cout << "    Threads utilizadas: " << numThreads << std::endl;

    // Criar estruturas para a paralelização
    ParallelExecutionData parallelData;
    parallelData.numThreads = numThreads;

    // Iniciar a estrutura auxiliar
    AuxiliaryStructures aux;
    
    // 1. Criar conjuntos básicos
    // std::cout << "    Criando conjuntos básicos..." << std::endl;
    
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
    // std::cout << "    Processando " << warehouse.numOrders << " pedidos..." << std::endl;
    
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
    // std::cout << "    Mapeando " << aux.allItems.size() << " itens em " 
    //          << warehouse.numCorridors << " corredores..." << std::endl;
    
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
    // std::cout << "    Calculando coberturas de corredores por pedido..." << std::endl;
    
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
    std::cout << "    Estruturas auxiliares criadas com sucesso." << std::endl;
    //std::cout << "      - " << warehouse.numOrders << " pedidos" << std::endl;
    //std::cout << "      - " << aux.allItems.size() << " itens únicos" << std::endl;
    //std::cout << "      - " << warehouse.numCorridors << " corredores" << std::endl;
    
    // Top 5 pedidos mais eficientes
    //std::cout << "      - Top 5 pedidos mais eficientes:" << std::endl;
    //for (int i = 0; i < std::min(5, (int)aux.orderEfficiency.size()); i++) {
    //    if (aux.orderEfficiency[i].second > 0) {
    //        std::cout << "        #" << aux.orderEfficiency[i].first 
    //                  << ": " << std::fixed << std::setprecision(2) 
    //                  << aux.orderEfficiency[i].second << " itens/corredor" << std::endl;
    //    }
    //}
    
    return true;
}

// Implementação das funções de estatística (como definimos antes)
void calculateOrderStatistics(AuxiliaryStructures& aux, AuxiliaryStructures::OrderStatistics& stats) {
    std::vector<double> efficiencies;
    for (const auto& [orderIdx, efficiency] : aux.orderEfficiency) {
        if (efficiency > 0) {
            efficiencies.push_back(efficiency);
        }
    }
    
    if (efficiencies.empty()) return;
    
    // Média
    stats.meanEfficiency = std::accumulate(efficiencies.begin(), efficiencies.end(), 0.0) / efficiencies.size();
    
    // Desvio padrão
    double sumSq = 0.0;
    for (double eff : efficiencies) {
        sumSq += (eff - stats.meanEfficiency) * (eff - stats.meanEfficiency);
    }
    stats.stdDevEfficiency = std::sqrt(sumSq / efficiencies.size());
    
    // Coeficiente de variação
    stats.coefficientOfVariation = stats.stdDevEfficiency / stats.meanEfficiency;
    
    // Mediana (requer ordenação)
    std::vector<double> sortedEfficiencies = efficiencies;
    std::sort(sortedEfficiencies.begin(), sortedEfficiencies.end());
    
    size_t mid = sortedEfficiencies.size() / 2;
    if (sortedEfficiencies.size() % 2 == 0) {
        stats.medianEfficiency = (sortedEfficiencies[mid-1] + sortedEfficiencies[mid]) / 2.0;
    } else {
        stats.medianEfficiency = sortedEfficiencies[mid];
    }
    
    // Quantis (quartis)
    stats.efficiencyQuantiles.resize(3);
    stats.efficiencyQuantiles[0] = sortedEfficiencies[sortedEfficiencies.size() / 4]; // 25%
    stats.efficiencyQuantiles[1] = stats.medianEfficiency;                            // 50%
    stats.efficiencyQuantiles[2] = sortedEfficiencies[3 * sortedEfficiencies.size() / 4]; // 75%
    
    // Distribuição para histograma (10 bins)
    double minEff = sortedEfficiencies.front();
    double maxEff = sortedEfficiencies.back();
    double range = maxEff - minEff;
    
    int numBins = 10;
    stats.efficiencyDistribution.resize(numBins, 0);
    stats.efficiencyBins.resize(numBins + 1);
    
    for (int i = 0; i <= numBins; i++) {
        stats.efficiencyBins[i] = minEff + (range * i) / numBins;
    }
    
    for (double eff : efficiencies) {
        int bin = std::min(numBins - 1, static_cast<int>((eff - minEff) / range * numBins));
        stats.efficiencyDistribution[bin]++;
    }
}

void calculateItemStatistics(AuxiliaryStructures& aux, AuxiliaryStructures::ItemStatistics& stats) {
    std::vector<double> scarcities;
    std::vector<int> frequencies;
    std::vector<double> leverages;
    
    for (int itemId : aux.allItems) {
        if (aux.weights.itemScarcityScore[itemId] > 0) {
            scarcities.push_back(aux.weights.itemScarcityScore[itemId]);
            frequencies.push_back(aux.weights.itemFrequency[itemId]);
            leverages.push_back(aux.weights.itemLeverageScore[itemId]);
        }
    }
    
    if (scarcities.empty()) return;
    
    // Calcular estatísticas de escassez
    stats.meanScarcity = std::accumulate(scarcities.begin(), scarcities.end(), 0.0) / scarcities.size();
    
    double sumSq = 0.0;
    for (double s : scarcities) {
        sumSq += (s - stats.meanScarcity) * (s - stats.meanScarcity);
    }
    stats.stdDevScarcity = std::sqrt(sumSq / scarcities.size());
    
    // Mediana de escassez
    std::vector<double> sortedScarcities = scarcities;
    std::sort(sortedScarcities.begin(), sortedScarcities.end());
    
    size_t mid = sortedScarcities.size() / 2;
    if (sortedScarcities.size() % 2 == 0) {
        stats.medianScarcity = (sortedScarcities[mid-1] + sortedScarcities[mid]) / 2.0;
    } else {
        stats.medianScarcity = sortedScarcities[mid];
    }
    
    // Calcular estatísticas de frequência
    stats.meanFrequency = std::accumulate(frequencies.begin(), frequencies.end(), 0.0) / frequencies.size();
    
    sumSq = 0.0;
    for (int f : frequencies) {
        sumSq += (f - stats.meanFrequency) * (f - stats.meanFrequency);
    }
    stats.stdDevFrequency = std::sqrt(sumSq / frequencies.size());
    
    // Calcular estatísticas de leverage (ADICIONADO)
    double meanLeverage = std::accumulate(leverages.begin(), leverages.end(), 0.0) / leverages.size();
    
    sumSq = 0.0;
    for (double l : leverages) {
        sumSq += (l - meanLeverage) * (l - meanLeverage);
    }
    double stdDevLeverage = std::sqrt(sumSq / leverages.size());
    
    // Identificar itens com alta escassez (acima de um desvio padrão da média)
    double scarcityThreshold = stats.meanScarcity + stats.stdDevScarcity;
    
    for (int itemId : aux.allItems) {
        if (aux.weights.itemScarcityScore[itemId] > scarcityThreshold) {
            stats.highScarcityItems.push_back(itemId);
        }
    }
    
    // Ordenar itens de alta escassez por escassez decrescente
    std::sort(stats.highScarcityItems.begin(), stats.highScarcityItems.end(),
              [&aux](int a, int b) {
                  return aux.weights.itemScarcityScore[a] > aux.weights.itemScarcityScore[b];
              });
              
    // Identificar itens estatisticamente significativos (alta frequência e leverage)
    // Utilizamos um cálculo de Z-score combinado
    std::vector<std::pair<int, double>> itemScores;
    
    for (int itemId : aux.allItems) {
        double freqZScore = (aux.weights.itemFrequency[itemId] - stats.meanFrequency) / stats.stdDevFrequency;
        // Correção do cálculo do Z-score do leverage
        double leverageZScore = (aux.weights.itemLeverageScore[itemId] - meanLeverage) / stdDevLeverage;
        
        double combinedScore = (freqZScore + leverageZScore) / 2.0;
        
        if (combinedScore > 1.0) { // Um desvio padrão acima da média
            itemScores.push_back({itemId, combinedScore});
        }
    }
    
    // Ordenar por score combinado
    std::sort(itemScores.begin(), itemScores.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
              
    // Pegar os top N itens significativos (limite em 20)
    int maxSignificantItems = std::min(20, static_cast<int>(itemScores.size()));
    for (int i = 0; i < maxSignificantItems; i++) {
        stats.statSignificantItems.push_back(itemScores[i].first);
    }
}

inline void inicializarEstruturasAprimoradas(AuxiliaryStructures& aux, const Warehouse& warehouse) {
    // Redimensionar estruturas
    aux.itens_aprimorado.resize(warehouse.numItems);
    aux.pedidos_aprimorado.resize(warehouse.numOrders);
    aux.corredores_aprimorado.resize(warehouse.numCorridors);
    
    std::cout << "    Inicializando estruturas aprimoradas..." << std::endl;
    
    // Inicializar IDs
    for (int i = 0; i < warehouse.numItems; i++) {
        aux.itens_aprimorado[i].id = i;
    }
    
    for (int p = 0; p < warehouse.numOrders; p++) {
        aux.pedidos_aprimorado[p].id = p;
    }
    
    for (int c = 0; c < warehouse.numCorridors; c++) {
        aux.corredores_aprimorado[c].id = c;
    }
    
    // Passar os dados dos pedidos para a nova estrutura
    for (int p = 0; p < warehouse.numOrders; p++) {
        auto& pedido = aux.pedidos_aprimorado[p];
        
        for (const auto& item_pair : warehouse.orders[p]) {
            int item_id = item_pair.first;
            int quantidade = item_pair.second;
            
            // Atualizar pedido
            pedido.itens.push_back(item_pair);
            pedido.total_itens += quantidade;
            
            // Atualizar item (se estiver dentro dos limites)
            if (item_id < warehouse.numItems) {
                aux.itens_aprimorado[item_id].pedidos_contendo.push_back(p);
                aux.itens_aprimorado[item_id].frequencia++;
            }
        }
        
        pedido.num_itens_distintos = pedido.itens.size();
    }
    
    // Inicializar dados de corredores
    for (int c = 0; c < warehouse.numCorridors; c++) {
        auto& corredor = aux.corredores_aprimorado[c];
        
        for (const auto& item_pair : warehouse.corridors[c]) {
            int item_id = item_pair.first;
            int quantidade = item_pair.second;
            
            // Atualizar corredor
            corredor.itens.push_back(item_pair);
            corredor.total_itens_disponiveis += quantidade;
            
            // Atualizar item (se estiver dentro dos limites)
            if (item_id < warehouse.numItems) {
                aux.itens_aprimorado[item_id].corredores.push_back({c, quantidade});
                aux.itens_aprimorado[item_id].disponibilidade_total += quantidade;
            }
        }
        
        corredor.num_itens_distintos = corredor.itens.size();
    }
    
    std::cout << "    Estruturas básicas inicializadas." << std::endl;
}

inline void calcularMetricasAvancadas(AuxiliaryStructures& aux) {
    std::cout << "    Calculando métricas avançadas..." << std::endl;
    
    // 1. Mapear corredores necessários por pedido
    for (size_t p = 0; p < aux.pedidos_aprimorado.size(); p++) {
        std::set<int> corredores_necessarios;
        
        for (const auto& item_pair : aux.pedidos_aprimorado[p].itens) {
            int item_id = item_pair.first;
            
            // Verificar se item_id está nos limites
            if (item_id < aux.itens_aprimorado.size()) {
                for (const auto& corredor_pair : aux.itens_aprimorado[item_id].corredores) {
                    corredores_necessarios.insert(corredor_pair.first);
                }
            }
        }
        
        aux.pedidos_aprimorado[p].corredores_necessarios.assign(
            corredores_necessarios.begin(), 
            corredores_necessarios.end()
        );
    }
    
    // 2. Calcular eficiência base de cada pedido
    for (size_t p = 0; p < aux.pedidos_aprimorado.size(); p++) {
        auto& pedido = aux.pedidos_aprimorado[p];
        if (!pedido.corredores_necessarios.empty()) {
            pedido.eficiencia_base = static_cast<double>(pedido.total_itens) / 
                                     pedido.corredores_necessarios.size();
        }
    }
    
    // 3. Calcular escassez de itens
    double max_escassez = 0.0;
    for (auto& item : aux.itens_aprimorado) {
        if (item.disponibilidade_total > 0) {
            item.escassez = 1.0 / item.disponibilidade_total;
        } else {
            item.escassez = 10.0; // Valor alto para itens indisponíveis
        }
        max_escassez = std::max(max_escassez, item.escassez);
    }
    
    // 4. Normalizar valores de escassez
    if (max_escassez > 0) {
        for (size_t i = 0; i < aux.itens_aprimorado.size(); i++) {
            aux.itens_aprimorado[i].escassez /= max_escassez;
        }
    }
    
    // 5. Calcular pedidos dependentes por corredor
    for (size_t i = 0; i < aux.itens_aprimorado.size(); i++) {
        for (int pedido_id : aux.itens_aprimorado[i].pedidos_contendo) {
            for (const auto& corredor_pair : aux.itens_aprimorado[i].corredores) {
                int corredor_id = corredor_pair.first;
                // Verificar se corredor_id está nos limites
                if (corredor_id < aux.corredores_aprimorado.size()) {
                    // Verificar se este pedido já está na lista
                    auto& deps = aux.corredores_aprimorado[corredor_id].pedidos_dependentes;
                    if (std::find(deps.begin(), deps.end(), pedido_id) == deps.end()) {
                        deps.push_back(pedido_id);
                    }
                }
            }
        }
    }
    
    std::cout << "    Métricas avançadas calculadas." << std::endl;
}

inline void calcularPrioridadePedidos(AuxiliaryStructures& aux, 
                                     std::vector<std::pair<int, double>>& pedidos_priorizados) {
    std::cout << "    Calculando prioridade dos pedidos..." << std::endl;
    
    pedidos_priorizados.clear();
    
    for (const auto& pedido : aux.pedidos_aprimorado) {
        // Pular pedidos sem corredores ou itens (inviáveis)
        if (pedido.corredores_necessarios.empty() || pedido.itens.empty()) {
            continue;
        }
        
        // Calcular fator de raridade
        double fator_raridade = 0.0;
        for (const auto& item_pair : pedido.itens) {
            int item_id = item_pair.first;
            int quantidade = item_pair.second;
            // Verificar se item_id está nos limites
            if (item_id < aux.itens_aprimorado.size()) {
                fator_raridade += aux.itens_aprimorado[item_id].escassez * quantidade;
            }
        }
        
        if (!pedido.itens.empty()) {
            fator_raridade /= pedido.itens.size();
        }
        
        // Prioridade combinando eficiência base e raridade
        double prioridade = pedido.eficiencia_base * (1.0 + 0.5 * fator_raridade);
        
        // Armazenar prioridade calculada
        aux.pedidos_aprimorado[pedido.id].prioridade = prioridade;
        
        // Adicionar ao vetor de prioridades
        pedidos_priorizados.push_back({pedido.id, prioridade});
    }
    
    // Ordenar por prioridade decrescente
    std::sort(pedidos_priorizados.begin(), pedidos_priorizados.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "    Priorização de pedidos concluída." << std::endl;
}

#endif // CRIA_AUXILIARES_H