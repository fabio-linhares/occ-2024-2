#include "input/input_parser.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <unordered_set>

// Implementação da função de validação
bool validarInstancia(const Warehouse& warehouse) {
    std::cout << "Validando instância carregada..." << std::endl;
    
    // 1. Validar limites básicos
    if (warehouse.numOrders <= 0 || warehouse.numItems <= 0 || warehouse.numCorridors <= 0) {
        std::cerr << "ERRO: Valores inválidos para numOrders, numItems ou numCorridors" << std::endl;
        return false;
    }
    
    // 2. Validar pedidos
    for (int p = 0; p < warehouse.numOrders; p++) {
        for (const auto& [itemId, quantidade] : warehouse.orders[p]) {
            if (itemId < 0 || itemId >= warehouse.numItems) {
                std::cerr << "ERRO: Pedido " << p << " contém item inválido: " << itemId << std::endl;
                return false;
            }
            if (quantidade <= 0) {
                std::cerr << "AVISO: Pedido " << p << " contém item " << itemId 
                         << " com quantidade inválida: " << quantidade << std::endl;
                // Não falhar por isso, apenas avisar
            }
        }
    }
    
    // 3. Validar corredores
    for (int c = 0; c < warehouse.numCorridors; c++) {
        for (const auto& [itemId, quantidade] : warehouse.corridors[c]) {
            if (itemId < 0 || itemId >= warehouse.numItems) {
                std::cerr << "ERRO: Corredor " << c << " contém item inválido: " << itemId << std::endl;
                return false;
            }
            if (quantidade <= 0) {
                std::cerr << "AVISO: Corredor " << c << " contém item " << itemId 
                         << " com quantidade inválida: " << quantidade << std::endl;
                // Não falhar por isso, apenas avisar
            }
        }
    }
    
    // 4. Validar LB/UB
    if (warehouse.LB < 0) {
        std::cerr << "ERRO: LB inválido: " << warehouse.LB << std::endl;
        return false;
    }
    if (warehouse.UB < warehouse.LB) {
        std::cerr << "ERRO: UB (" << warehouse.UB << ") menor que LB (" << warehouse.LB << ")" << std::endl;
        return false;
    }
    
    std::cout << "Instância validada com sucesso!" << std::endl;
    return true;
}

Warehouse InputParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Não foi possível abrir o arquivo: " + filePath);
    }
    
    Warehouse warehouse;
    std::string line;
    
    // Ler primeira linha com números de pedidos, itens e corredores
    if (!std::getline(file, line)) {
        throw std::runtime_error("Arquivo vazio ou corrompido");
    }
    
    std::istringstream headerLine(line);
    int numOrders, numItems, numCorridors;
    
    // CORREÇÃO: Ler explicitamente em variáveis locais primeiro
    if (!(headerLine >> numOrders >> numItems >> numCorridors)) {
        throw std::runtime_error("Primeira linha inválida: deve conter 3 números inteiros");
    }
    
    // Verificar valores
    if (numOrders <= 0 || numItems <= 0 || numCorridors <= 0) {
        throw std::runtime_error("Valores inválidos para numOrders, numItems ou numCorridors");
    }
    
    // Definir as propriedades da warehouse APÓS validação
    warehouse.numOrders = numOrders;
    warehouse.numItems = numItems;
    warehouse.numCorridors = numCorridors;
    
    std::cout << "Lendo instância com " << warehouse.numOrders << " pedidos, " 
              << warehouse.numItems << " itens e " << warehouse.numCorridors 
              << " corredores" << std::endl;
    
    // Inicializar estruturas com tamanho fixo
    warehouse.orders.resize(warehouse.numOrders);
    warehouse.corridors.resize(warehouse.numCorridors);
    
    // Ler pedidos - NUNCA adicionar IDs inválidos
    for (int i = 0; i < warehouse.numOrders; ++i) {
        if (!std::getline(file, line)) {
            throw std::runtime_error("Arquivo terminado inesperadamente ao ler pedidos");
        }
        
        std::istringstream orderLine(line);
        int numItemsInOrder;
        
        if (!(orderLine >> numItemsInOrder)) {
            throw std::runtime_error("Formato inválido ao ler número de itens no pedido " + std::to_string(i));
        }
        
        for (int j = 0; j < numItemsInOrder; j++) {
            int itemId, quantity;
            if (!(orderLine >> itemId >> quantity)) {
                throw std::runtime_error("Formato inválido ao ler item " + std::to_string(j) + 
                                         " do pedido " + std::to_string(i));
            }
            
            // VALIDAÇÃO ESTRITA: ignorar item se itemId inválido
            if (itemId < 0 || itemId >= warehouse.numItems) {
                std::cerr << "AVISO: Ignorando item com ID inválido " << itemId 
                         << " no pedido " << i << std::endl;
                continue;
            }
            
            if (quantity <= 0) {
                std::cerr << "AVISO: Quantidade inválida " << quantity 
                         << " para item " << itemId << " no pedido " << i << std::endl;
                continue;
            }
            
            warehouse.orders[i][itemId] = quantity;
        }
    }
    
    // Ler corredores
    for (int i = 0; i < warehouse.numCorridors; ++i) {
        if (!std::getline(file, line)) {
            throw std::runtime_error("Arquivo terminado inesperadamente ao ler corredores");
        }
        
        std::istringstream corridorLine(line);
        int numItemsInCorridor;
        
        if (!(corridorLine >> numItemsInCorridor)) {
            throw std::runtime_error("Formato inválido ao ler número de itens no corredor " + std::to_string(i));
        }
        
        for (int j = 0; j < numItemsInCorridor; j++) {
            int itemId, quantity;
            if (!(corridorLine >> itemId >> quantity)) {
                throw std::runtime_error("Formato inválido ao ler item " + std::to_string(j) + 
                                         " do corredor " + std::to_string(i));
            }
            
            // VALIDAÇÃO ESTRITA: ignorar item se itemId inválido
            if (itemId < 0 || itemId >= warehouse.numItems) {
                std::cerr << "AVISO: Ignorando item com ID inválido " << itemId 
                         << " no corredor " << i << std::endl;
                continue;
            }
            
            if (quantity <= 0) {
                std::cerr << "AVISO: Quantidade inválida " << quantity 
                         << " para item " << itemId << " no corredor " << i << std::endl;
                continue;
            }
            
            warehouse.corridors[i][itemId] = quantity;
        }
    }
    
    // Ler a última linha com LB e UB
    if (!std::getline(file, line)) {
        throw std::runtime_error("Arquivo terminado inesperadamente ao ler LB e UB");
    }
    
    std::istringstream lbubLine(line);
    
    // CORREÇÃO: Garantir que lemos exatamente 2 valores e nada mais
    if (!(lbubLine >> warehouse.LB >> warehouse.UB)) {
        throw std::runtime_error("Última linha inválida: deve conter 2 números inteiros (LB e UB)");
    }
    
    // Verificar se há mais conteúdo na linha (o que seria inválido)
    std::string extraContent;
    if (lbubLine >> extraContent) {
        throw std::runtime_error("Última linha com formato inválido: contém dados extras");
    }
    
    // Validar limites
    if (warehouse.LB < 0 || warehouse.UB < warehouse.LB) {
        throw std::runtime_error("Valores inválidos para LB ou UB");
    }
    
    std::cout << "Limites da instância: LB=" << warehouse.LB << ", UB=" << warehouse.UB << std::endl;
    
    // Validar a instância carregada
    if (!validarInstancia(warehouse)) {
        throw std::runtime_error("Instância inválida após parser: " + filePath);
    }
    
    return warehouse;
}