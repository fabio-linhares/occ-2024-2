#include "input/input_parser.h"
#include "core/warehouse.h"
#include <iostream>
#include <string>
#include <iomanip>

int main(int argc, char** argv) {
    std::cout << "===== VERIFICADOR DO PARSER =====\n";
    
    // Definir o arquivo de teste
    std::string testFile = "data/input/test_instance.txt";
    if (argc > 1) {
        testFile = argv[1];
    }
    
    std::cout << "Arquivo de teste: " << testFile << "\n\n";
    
    try {
        // Ler a instância usando o parser
        InputParser parser;
        Warehouse warehouse = parser.parseFile(testFile);
        
        // Exibir informações básicas
        std::cout << "=== INFORMAÇÕES BÁSICAS ===\n";
        std::cout << "Número de pedidos: " << warehouse.numOrders << std::endl;
        std::cout << "Número de itens: " << warehouse.numItems << std::endl;
        std::cout << "Número de corredores: " << warehouse.numCorridors << std::endl;
        std::cout << "LB: " << warehouse.LB << ", UB: " << warehouse.UB << std::endl;
        
        // Exibir detalhes dos pedidos
        std::cout << "\n=== DETALHES DOS PEDIDOS ===\n";
        for (int p = 0; p < warehouse.numOrders; p++) {
            std::cout << "Pedido #" << p << ": ";
            if (warehouse.orders[p].empty()) {
                std::cout << "vazio" << std::endl;
            } else {
                int totalItens = 0;
                std::cout << warehouse.orders[p].size() << " itens diferentes { ";
                for (const auto& [itemId, qtd] : warehouse.orders[p]) {
                    std::cout << "(" << itemId << ": " << qtd << ") ";
                    totalItens += qtd;
                }
                std::cout << "}, Total: " << totalItens << " itens" << std::endl;
            }
        }
        
        // Exibir detalhes dos corredores
        std::cout << "\n=== DETALHES DOS CORREDORES ===\n";
        for (int c = 0; c < warehouse.numCorridors; c++) {
            std::cout << "Corredor #" << c << ": ";
            if (warehouse.corridors[c].empty()) {
                std::cout << "vazio" << std::endl;
            } else {
                std::cout << warehouse.corridors[c].size() << " itens diferentes { ";
                for (const auto& [itemId, qtd] : warehouse.corridors[c]) {
                    std::cout << "(" << itemId << ": " << qtd << ") ";
                }
                std::cout << "}" << std::endl;
            }
        }
        
        // Verificar IDs dos itens nos pedidos
        std::cout << "\n=== VERIFICAÇÃO DE IDs ===\n";
        bool pedidosValidos = true;
        for (int p = 0; p < warehouse.numOrders; p++) {
            for (const auto& [itemId, qtd] : warehouse.orders[p]) {
                if (itemId < 0 || itemId >= warehouse.numItems) {
                    std::cout << "ERRO: Pedido #" << p << " contém item inválido: " 
                              << itemId << std::endl;
                    pedidosValidos = false;
                }
            }
        }
        if (pedidosValidos) {
            std::cout << "✓ Todos os IDs de itens nos pedidos são válidos.\n";
        }
        
        // Verificar IDs dos itens nos corredores
        bool corredoresValidos = true;
        for (int c = 0; c < warehouse.numCorridors; c++) {
            for (const auto& [itemId, qtd] : warehouse.corridors[c]) {
                if (itemId < 0 || itemId >= warehouse.numItems) {
                    std::cout << "ERRO: Corredor #" << c << " contém item inválido: " 
                              << itemId << std::endl;
                    corredoresValidos = false;
                }
            }
        }
        if (corredoresValidos) {
            std::cout << "✓ Todos os IDs de itens nos corredores são válidos.\n";
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERRO ao verificar instância: " << e.what() << std::endl;
        return 1;
    }
}