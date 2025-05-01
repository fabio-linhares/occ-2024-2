#include "input/input_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>

Warehouse InputParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Não foi possível abrir o arquivo: " + filePath);
    }
    
    Warehouse warehouse;
    std::string line;
    
    try {
        // === Primeira linha: número de pedidos, itens e corredores ===
        if (!std::getline(file, line)) {
            throw std::runtime_error("Arquivo vazio ou corrompido");
        }
        
        std::istringstream headerLine(line);
        int numOrders, numItems, numCorridors;
        
        if (!(headerLine >> numOrders >> numItems >> numCorridors)) {
            throw std::runtime_error("Formato inválido na primeira linha. Esperado: numOrders numItems numCorridors");
        }
        
        // Validar os valores lidos
        if (numOrders <= 0) {
            throw std::runtime_error("Número de pedidos deve ser positivo: " + std::to_string(numOrders));
        }
        if (numItems <= 0) {
            throw std::runtime_error("Número de itens deve ser positivo: " + std::to_string(numItems));
        }
        if (numCorridors <= 0) {
            throw std::runtime_error("Número de corredores deve ser positivo: " + std::to_string(numCorridors));
        }
        
        // Definir os valores na estrutura
        warehouse.numOrders = numOrders;
        warehouse.numItems = numItems;
        warehouse.numCorridors = numCorridors;
        
        // Inicializar estruturas com tamanho correto
        warehouse.orders.resize(numOrders);
        warehouse.corridors.resize(numCorridors);
        
        // === Leitura dos pedidos ===
        for (int orderId = 0; orderId < numOrders; orderId++) {
            if (!std::getline(file, line)) {
                throw std::runtime_error("Fim inesperado do arquivo ao ler pedido " + std::to_string(orderId));
            }
            
            std::istringstream orderLine(line);
            int numItemsInOrder;
            
            if (!(orderLine >> numItemsInOrder)) {
                throw std::runtime_error("Formato inválido ao ler número de itens no pedido " + std::to_string(orderId));
            }
            
            if (numItemsInOrder < 0) {
                throw std::runtime_error("Número de itens não pode ser negativo no pedido " + std::to_string(orderId));
            }
            
            // Ler cada par (itemId, quantidade)
            for (int i = 0; i < numItemsInOrder; i++) {
                int itemId, quantity;
                
                if (!(orderLine >> itemId >> quantity)) {
                    throw std::runtime_error("Formato inválido ao ler item " + std::to_string(i) + 
                                           " do pedido " + std::to_string(orderId));
                }
                
                // Validar itemId
                if (itemId < 0 || itemId >= numItems) {
                    std::cerr << "AVISO: ID de item inválido " << itemId << " no pedido " 
                              << orderId << " (ignorando)" << std::endl;
                    continue;
                }
                
                // Validar quantidade
                if (quantity <= 0) {
                    std::cerr << "AVISO: Quantidade inválida " << quantity 
                              << " para item " << itemId << " no pedido " 
                              << orderId << " (ignorando)" << std::endl;
                    continue;
                }
                
                // Adicionar ao mapa de itens deste pedido
                warehouse.orders[orderId][itemId] = quantity;
            }
        }
        
        // === Leitura dos corredores ===
        for (int corridorId = 0; corridorId < numCorridors; corridorId++) {
            if (!std::getline(file, line)) {
                throw std::runtime_error("Fim inesperado do arquivo ao ler corredor " + std::to_string(corridorId));
            }
            
            std::istringstream corridorLine(line);
            int numItemsInCorridor;
            
            if (!(corridorLine >> numItemsInCorridor)) {
                throw std::runtime_error("Formato inválido ao ler número de itens no corredor " + std::to_string(corridorId));
            }
            
            if (numItemsInCorridor < 0) {
                throw std::runtime_error("Número de itens não pode ser negativo no corredor " + std::to_string(corridorId));
            }
            
            // Ler cada par (itemId, quantidade)
            for (int i = 0; i < numItemsInCorridor; i++) {
                int itemId, quantity;
                
                if (!(corridorLine >> itemId >> quantity)) {
                    throw std::runtime_error("Formato inválido ao ler item " + std::to_string(i) + 
                                           " do corredor " + std::to_string(corridorId));
                }
                
                // Validar itemId
                if (itemId < 0 || itemId >= numItems) {
                    std::cerr << "AVISO: ID de item inválido " << itemId << " no corredor " 
                              << corridorId << " (ignorando)" << std::endl;
                    continue;
                }
                
                // Validar quantidade
                if (quantity <= 0) {
                    std::cerr << "AVISO: Quantidade inválida " << quantity 
                              << " para item " << itemId << " no corredor " 
                              << corridorId << " (ignorando)" << std::endl;
                    continue;
                }
                
                // Adicionar ao mapa de itens deste corredor
                warehouse.corridors[corridorId][itemId] = quantity;
            }
        }
        
        // === Última linha: LB e UB ===
        if (!std::getline(file, line)) {
            throw std::runtime_error("Fim inesperado do arquivo ao ler LB e UB");
        }
        
        std::istringstream boundsLine(line);
        int LB, UB;
        
        if (!(boundsLine >> LB >> UB)) {
            throw std::runtime_error("Formato inválido ao ler LB e UB");
        }
        
        // Validar LB e UB
        if (LB < 0) {
            throw std::runtime_error("LB não pode ser negativo: " + std::to_string(LB));
        }
        
        if (UB < LB) {
            throw std::runtime_error("UB deve ser maior ou igual a LB: LB=" + 
                                   std::to_string(LB) + ", UB=" + std::to_string(UB));
        }
        
        warehouse.LB = LB;
        warehouse.UB = UB;
        
        return warehouse;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Erro ao ler arquivo " + filePath + ": " + e.what());
    }
}