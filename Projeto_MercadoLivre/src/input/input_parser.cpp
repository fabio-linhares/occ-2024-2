#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "input/input_parser.h"

Warehouse InputParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Não foi possível abrir o arquivo: " + filePath);
    }
    
    Warehouse warehouse;
    std::string line;
    
    // Ler primeira linha com números de pedidos, itens e corredores
    if (!std::getline(file, line)) {
        throw std::runtime_error("Arquivo vazio ou formato inválido");
    }
    
    std::istringstream headerLine(line);
    if (!(headerLine >> warehouse.numOrders >> warehouse.numItems >> warehouse.numCorridors)) {
        throw std::runtime_error("Formato inválido na primeira linha do arquivo");
    }
    
    // Validar valores iniciais
    if (warehouse.numOrders <= 0 || warehouse.numItems <= 0 || warehouse.numCorridors <= 0) {
        throw std::runtime_error("Valores inválidos para numOrders, numItems ou numCorridors");
    }
    
    // Ler pedidos
    warehouse.orders.resize(warehouse.numOrders);
    for (int i = 0; i < warehouse.numOrders; i++) {
        if (!std::getline(file, line)) {
            throw std::runtime_error("Erro ao ler dados do pedido " + std::to_string(i));
        }
        
        std::istringstream orderLine(line);
        int numItemsInOrder;
        if (!(orderLine >> numItemsInOrder)) {
            throw std::runtime_error("Formato inválido para o número de itens no pedido " + std::to_string(i));
        }
        
        if (numItemsInOrder <= 0) {
            throw std::runtime_error("Número inválido de itens para o pedido " + std::to_string(i));
        }
        
        for (int j = 0; j < numItemsInOrder; j++) {
            int itemId, quantity;
            if (!(orderLine >> itemId >> quantity)) {
                throw std::runtime_error("Formato inválido nos itens do pedido " + std::to_string(i));
            }
            
            // Verificar se o item é válido
            if (itemId < 0 || itemId >= warehouse.numItems) {
                throw std::runtime_error("ID de item inválido no pedido " + std::to_string(i) + 
                                        ": " + std::to_string(itemId));
            }
            
            // Verificar se a quantidade é válida
            if (quantity <= 0) {
                throw std::runtime_error("Quantidade inválida para o item " + 
                                       std::to_string(itemId) + " no pedido " + 
                                       std::to_string(i) + ": " + std::to_string(quantity));
            }
            
            warehouse.orders[i].push_back(std::make_pair(itemId, quantity));
        }
        
        // Verificar se não há dados extras na linha (formato inválido)
        int extraData;
        if (orderLine >> extraData) {
            throw std::runtime_error("Dados extras encontrados no pedido " + std::to_string(i));
        }
    }
    
    // Ler corredores
    warehouse.corridors.resize(warehouse.numCorridors);
    for (int i = 0; i < warehouse.numCorridors; i++) {
        if (!std::getline(file, line)) {
            throw std::runtime_error("Erro ao ler dados do corredor " + std::to_string(i));
        }
        
        std::istringstream corridorLine(line);
        int numItemsInCorridor;
        if (!(corridorLine >> numItemsInCorridor)) {
            throw std::runtime_error("Formato inválido para o número de itens no corredor " + std::to_string(i));
        }
        
        if (numItemsInCorridor < 0) {
            throw std::runtime_error("Número inválido de itens para o corredor " + std::to_string(i));
        }
        
        for (int j = 0; j < numItemsInCorridor; j++) {
            int itemId, quantity;
            if (!(corridorLine >> itemId >> quantity)) {
                throw std::runtime_error("Formato inválido nos itens do corredor " + std::to_string(i));
            }
            
            // Verificar se o item é válido
            if (itemId < 0 || itemId >= warehouse.numItems) {
                throw std::runtime_error("ID de item inválido no corredor " + std::to_string(i) + 
                                        ": " + std::to_string(itemId));
            }
            
            // Verificar se a quantidade é válida
            if (quantity <= 0) {
                throw std::runtime_error("Quantidade inválida para o item " + 
                                       std::to_string(itemId) + " no corredor " + 
                                       std::to_string(i) + ": " + std::to_string(quantity));
            }
            
            warehouse.corridors[i].push_back(std::make_pair(itemId, quantity));
        }
        
        // Verificar se não há dados extras na linha (formato inválido)
        int extraData;
        if (corridorLine >> extraData) {
            throw std::runtime_error("Dados extras encontrados no corredor " + std::to_string(i));
        }
    }
    
    // Ler a última linha com LB e UB
    // Lendo todo o resto do arquivo para encontrar a última linha não vazia
    std::string lastValidLine;
    
    while (std::getline(file, line)) {
        // Remover espaços em branco
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.find_last_not_of(" \t\r\n") != std::string::npos)
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Se a linha não for vazia, guarde-a
        if (!line.empty()) {
            // Se encontrarmos alguma linha não-numérica após os corredores, é um erro de formato
            std::istringstream testLine(line);
            int testVal;
            if (!(testLine >> testVal)) {
                throw std::runtime_error("Formato inválido após definição dos corredores");
            }
            
            lastValidLine = line;
        }
    }
    
    // Processar a última linha válida para obter LB e UB
    if (!lastValidLine.empty()) {
        std::istringstream iss(lastValidLine);
        int lb, ub;
        if (iss >> lb >> ub) {
            warehouse.LB = lb;
            warehouse.UB = ub;
            
            // Verificar valores de LB e UB
            if (lb < 0 || ub < lb) {
                throw std::runtime_error("Valores inválidos para LB (" + std::to_string(lb) + 
                                       ") e UB (" + std::to_string(ub) + ")");
            }
        } else {
            std::cerr << "AVISO: Limites LB e UB não encontrados no arquivo. Usando valores padrão.\n";
            warehouse.LB = 1;
            warehouse.UB = warehouse.numCorridors;
        }
    } else {
        std::cerr << "AVISO: Limites LB e UB não encontrados no arquivo. Usando valores padrão.\n";
        warehouse.LB = 1;
        warehouse.UB = warehouse.numCorridors;
    }
    
    return warehouse;
}