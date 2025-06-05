#include "verificar_instancias.h"
#include <iostream>
#include <iomanip>
#include <climits>  // Added for INT_MAX
#include "parser.h"
#include "armazem.h"

void verificarInstancias(const std::string& filePath) {
    std::cout << "Verificando instância: " << filePath << std::endl;
    try {
        // Carregar a instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(filePath);
        
        // Exibir informações básicas
        std::cout << "\n=== Informações Básicas da Instância ===\n";
        std::cout << "Número de pedidos: " << backlog.numPedidos << std::endl;
        std::cout << "Número de itens: " << deposito.numItens << std::endl;
        std::cout << "Número de corredores: " << deposito.numCorredores << std::endl;
        std::cout << "Limites da wave: LB=" << backlog.wave.LB << ", UB=" << backlog.wave.UB << std::endl;
        
        // Estatísticas adicionais
        int totalUnidades = 0;
        int maxItensPorPedido = 0;
        int minItensPorPedido = INT_MAX;
        
        for (int p = 0; p < backlog.numPedidos; p++) {
            int itensPedido = backlog.pedido[p].size();
            maxItensPorPedido = std::max(maxItensPorPedido, itensPedido);
            minItensPorPedido = std::min(minItensPorPedido, itensPedido);
            
            for (const auto& [itemId, quantidade] : backlog.pedido[p]) {
                totalUnidades += quantidade;
            }
        }
        
        std::cout << "\n=== Estatísticas dos Pedidos ===\n";
        std::cout << "Total de unidades solicitadas: " << totalUnidades << std::endl;
        std::cout << "Máximo de tipos de itens por pedido: " << maxItensPorPedido << std::endl;
        std::cout << "Mínimo de tipos de itens por pedido: " << minItensPorPedido << std::endl;
        
        // Exibir alguns pedidos como exemplo
        std::cout << "\n=== Exemplos de Pedidos ===\n";
        int pedidosParaMostrar = std::min(3, backlog.numPedidos);
        for (int p = 0; p < pedidosParaMostrar; p++) {
            std::cout << "Pedido " << p << ": " << backlog.pedido[p].size() << " tipos de itens\n";
            int contador = 0;
            for (const auto& [itemId, quantidade] : backlog.pedido[p]) {
                std::cout << "  Item " << itemId << ": " << quantidade << " unidades\n";
                if (++contador >= 3) {
                    std::cout << "  ... (e mais " << (backlog.pedido[p].size() - 3) << " tipos de itens)\n";
                    break;
                }
            }
        }
        
        // Exibir alguns corredores como exemplo
        std::cout << "\n=== Exemplos de Corredores ===\n";
        int corredoresParaMostrar = std::min(3, deposito.numCorredores);
        for (int c = 0; c < corredoresParaMostrar; c++) {
            std::cout << "Corredor " << c << ": " << deposito.corredor[c].size() << " tipos de itens\n";
            int contador = 0;
            for (const auto& [itemId, quantidade] : deposito.corredor[c]) {
                std::cout << "  Item " << itemId << ": " << quantidade << " unidades\n";
                if (++contador >= 3) {
                    std::cout << "  ... (e mais " << (deposito.corredor[c].size() - 3) << " tipos de itens)\n";
                    break;
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Erro ao verificar instância: " << e.what() << std::endl;
    }
}