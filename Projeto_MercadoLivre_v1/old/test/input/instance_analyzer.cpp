#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <set>
#include "input/input_parser.h"

namespace fs = std::filesystem;

// Função para exibir um resumo de uma instância
void printInstanceSummary(const std::string& fileName, const Warehouse& warehouse) {
    std::cout << "==================================================" << std::endl;
    std::cout << "Arquivo: " << fileName << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "Número de pedidos: " << warehouse.numOrders << std::endl;
    std::cout << "Número de itens: " << warehouse.numItems << std::endl;
    std::cout << "Número de corredores: " << warehouse.numCorridors << std::endl;
    std::cout << "Limite inferior (LB): " << warehouse.LB << std::endl;
    std::cout << "Limite superior (UB): " << warehouse.UB << std::endl;
    
    // Mapa para contar a quantidade pedida de cada item
    std::map<int, int> orderedItems;
    
    // Estatísticas dos pedidos
    int totalItemsInOrders = 0;
    int maxItemsInOrder = 0;
    int totalOrderedUnits = 0;
    
    for (const auto& order : warehouse.orders) {
        totalItemsInOrders += order.size();
        maxItemsInOrder = std::max(maxItemsInOrder, static_cast<int>(order.size()));
        
        for (const auto& item : order) {
            orderedItems[item.first] += item.second;
            totalOrderedUnits += item.second;
        }
    }
    
    std::cout << "Média de itens diferentes por pedido: " << std::fixed << std::setprecision(2) 
              << static_cast<double>(totalItemsInOrders) / warehouse.numOrders << std::endl;
    std::cout << "Máximo de itens diferentes em um pedido: " << maxItemsInOrder << std::endl;
    std::cout << "Total de unidades solicitadas: " << totalOrderedUnits << std::endl;
    
    // Mapa para contar a quantidade disponível de cada item
    std::map<int, int> availableItems;
    
    // Estatísticas dos corredores
    int totalItemsInCorridors = 0;
    int maxItemsInCorridor = 0;
    int totalAvailableUnits = 0;
    
    for (const auto& corridor : warehouse.corridors) {
        totalItemsInCorridors += corridor.size();
        maxItemsInCorridor = std::max(maxItemsInCorridor, static_cast<int>(corridor.size()));
        
        for (const auto& item : corridor) {
            availableItems[item.first] += item.second;
            totalAvailableUnits += item.second;
        }
    }
    
    std::cout << "Média de itens diferentes por corredor: " << std::fixed << std::setprecision(2) 
              << static_cast<double>(totalItemsInCorridors) / warehouse.numCorridors << std::endl;
    std::cout << "Máximo de itens diferentes em um corredor: " << maxItemsInCorridor << std::endl;
    std::cout << "Total de unidades disponíveis: " << totalAvailableUnits << std::endl;
    
    // Verificar se todos os pedidos podem ser atendidos
    bool allOrdersCanBeFulfilled = true;
    std::vector<int> missingItems;
    
    for (const auto& [itemId, quantity] : orderedItems) {
        if (availableItems[itemId] < quantity) {
            allOrdersCanBeFulfilled = false;
            missingItems.push_back(itemId);
        }
    }
    
    std::cout << "Todos os pedidos podem ser atendidos? " 
              << (allOrdersCanBeFulfilled ? "Sim" : "Não") << std::endl;
    
    if (!allOrdersCanBeFulfilled) {
        std::cout << "Itens com quantidade insuficiente: ";
        for (size_t i = 0; i < missingItems.size() && i < 5; ++i) {
            std::cout << missingItems[i] << " ";
        }
        if (missingItems.size() > 5) {
            std::cout << "... (e outros " << missingItems.size() - 5 << " itens)";
        }
        std::cout << std::endl;
    }
    
    // Para cada pedido, calcular quais corredores são necessários e sua densidade
    std::vector<std::pair<int, double>> pedidosEficiencia; // (índice, densidade)

    for (int orderIdx = 0; orderIdx < warehouse.orders.size(); orderIdx++) {
        // Identificar quais corredores são necessários para este pedido
        std::set<int> corridorsNeeded;
        
        // Para cada item no pedido
        for (const auto& orderItem : warehouse.orders[orderIdx]) {
            int itemId = orderItem.first;
            
            // Encontrar em quais corredores este item está disponível
            for (int corrIdx = 0; corrIdx < warehouse.corridors.size(); corrIdx++) {
                for (const auto& corridorItem : warehouse.corridors[corrIdx]) {
                    if (corridorItem.first == itemId && corridorItem.second > 0) {
                        corridorsNeeded.insert(corrIdx);
                        break;
                    }
                }
            }
        }
        
        // Calcular a densidade (itens/corredores)
        double densidade = 0.0;
        if (!corridorsNeeded.empty()) {
            densidade = static_cast<double>(warehouse.orders[orderIdx].size()) / corridorsNeeded.size();
        } else {
            // Marcar pedidos sem corredores disponíveis como não atendíveis
            densidade = 0.0; // Atribuir zero em vez de infinito
        }
        pedidosEficiencia.push_back({orderIdx, densidade});
    }

    // Ordenar pedidos por densidade (do mais eficiente ao menos eficiente)
    std::sort(pedidosEficiencia.begin(), pedidosEficiencia.end(), 
        [](const auto& a, const auto& b) { 
            // Garante que pedidos com densidade 0 (impossíveis) fiquem por último
            if (a.second == 0) return false;
            if (b.second == 0) return true;
            return a.second > b.second; 
        });

    // Exibir os 5 pedidos mais eficientes, pulando os impossíveis
    std::cout << "Top 5 pedidos mais eficientes (itens/corredores):" << std::endl;
    int count = 0;
    for (int i = 0; i < pedidosEficiencia.size() && count < 5; i++) {
        if (pedidosEficiencia[i].second > 0) {
            std::cout << "  Pedido #" << pedidosEficiencia[i].first 
                      << ": " << std::fixed << std::setprecision(2) << pedidosEficiencia[i].second 
                      << " itens/corredor" << std::endl;
            count++;
        }
    }
    if (count == 0) {
        std::cout << "  Nenhum pedido pode ser atendido com os corredores disponíveis" << std::endl;
    }

    std::cout << "==================================================" << std::endl;
}

int main() {
    // Caminho para o diretório de instâncias
    std::string inputDir = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/input/";
    
    // Verificar se o diretório existe
    if (!fs::exists(inputDir)) {
        std::cerr << "Diretório de instâncias não encontrado: " << inputDir << std::endl;
        return 1;
    }
    
    // Coletar todos os arquivos de instâncias
    std::vector<std::string> instanceFiles;
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            instanceFiles.push_back(entry.path().string());
        }
    }
    
    // Ordenar os arquivos para apresentá-los em ordem
    std::sort(instanceFiles.begin(), instanceFiles.end());
    
    // Criar o parser
    InputParser parser;
    
    // Para cada arquivo, ler e mostrar resumo
    int validFiles = 0;
    for (const auto& file : instanceFiles) {
        try {
            Warehouse warehouse = parser.parseFile(file);
            std::string fileName = fs::path(file).filename().string();
            printInstanceSummary(fileName, warehouse);
            validFiles++;
        } catch (const std::exception& e) {
            std::cerr << "Erro ao processar arquivo " << file << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "\nResumo:" << std::endl;
    std::cout << "Total de arquivos processados: " << instanceFiles.size() << std::endl;
    std::cout << "Arquivos válidos: " << validFiles << std::endl;
    std::cout << "Arquivos com erro: " << instanceFiles.size() - validFiles << std::endl;
    
    return 0;
}