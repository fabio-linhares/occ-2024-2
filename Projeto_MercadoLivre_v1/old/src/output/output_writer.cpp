#include "output/output_writer.h"
#include <fstream>
#include <iostream>

bool OutputWriter::writeSolution(const Solution& solution, const std::string& filePath) const {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Não foi possível abrir o arquivo para escrita: " << filePath << std::endl;
            return false;
        }
        
        // Formato do arquivo de saída:
        // Primeira linha: número de pedidos na wave
        // Próximas n linhas: índices dos pedidos selecionados (indexados a partir de 1)
        // Linha seguinte: número de corredores visitados
        // Próximas m linhas: índices dos corredores visitados (indexados a partir de 1)
        
        const auto& orders = solution.getSelectedOrders();
        const auto& corridors = solution.getVisitedCorridors();
        
        // Escrever número de pedidos
        file << orders.size() << std::endl;
        
        // Escrever pedidos (convertendo para 1-indexed)
        for (int orderId : orders) {
            file << (orderId + 1) << std::endl;
        }
        
        // Escrever número de corredores
        file << corridors.size() << std::endl;
        
        // Escrever corredores (convertendo para 1-indexed)
        for (int corridorId : corridors) {
            file << (corridorId + 1) << std::endl;
        }
        
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Erro ao escrever solução: " << e.what() << std::endl;
        return false;
    }
}