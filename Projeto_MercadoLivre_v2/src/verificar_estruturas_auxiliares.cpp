#include "verificar_estruturas_auxiliares.h"
#include <iostream>
#include <iomanip>
#include "parser.h"
#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "analisador_relevancia.h"
#include "gestor_waves.h"

void verificarEstruturasAuxiliares(const std::string& filePath) {
    std::cout << "Verificando estruturas auxiliares para a instância: " << filePath << std::endl;
    try {
        // Carregar a instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(filePath);
        
        std::cout << "\n=== Informações Básicas da Instância ===\n";
        std::cout << "Número de pedidos: " << backlog.numPedidos << std::endl;
        std::cout << "Número de itens: " << deposito.numItens << std::endl;
        std::cout << "Número de corredores: " << deposito.numCorredores << std::endl;
        std::cout << "Limites da wave: LB=" << backlog.wave.LB << ", UB=" << backlog.wave.UB << std::endl;
        
        // Inicializar estruturas auxiliares
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);
        
        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);
        
        AnalisadorRelevancia analisador(backlog.numPedidos);
        analisador.construir(backlog, localizador);
        
        // Exibir informações do Localizador de Itens
        std::cout << "\n=== Localizador de Itens ===\n";
        int itemsToShow = std::min(10, deposito.numItens); // Mostrar no máximo 10 itens
        for (int i = 0; i < itemsToShow; i++) {
            const auto& corredores = localizador.getCorredoresComItem(i);
            std::cout << "Item " << i << " está em " << corredores.size() << " corredores: ";
            int count = 0;
            for (const auto& [corredorId, quantidade] : corredores) {
                std::cout << "C" << corredorId << "(" << quantidade << ") ";
                if (++count >= 5) { // Limitar a 5 corredores por linha
                    std::cout << "...";
                    break;
                }
            }
            std::cout << std::endl;
        }
        if (deposito.numItens > itemsToShow) {
            std::cout << "... e mais " << (deposito.numItens - itemsToShow) << " itens\n";
        }
        
        // Exibir informações do Verificador de Disponibilidade
        std::cout << "\n=== Verificador de Disponibilidade ===\n";
        for (int i = 0; i < itemsToShow; i++) {
            std::cout << "Item " << i << ": " << verificador.estoqueTotal[i] << " unidades disponíveis\n";
        }
        if (deposito.numItens > itemsToShow) {
            std::cout << "... e mais " << (deposito.numItens - itemsToShow) << " itens\n";
        }
        
        // Exibir informações do Analisador de Relevância
        std::cout << "\n=== Analisador de Relevância ===\n";
        std::cout << "Top 10 pedidos mais relevantes:\n";
        std::cout << std::setw(8) << "Pedido" << std::setw(10) << "Tipos" << std::setw(12) << "Unidades" 
                  << std::setw(12) << "Corredores" << std::setw(15) << "Pontuação" << std::endl;
        
        auto pedidosOrdenados = analisador.getPedidosOrdenadosPorRelevancia();
        int pedidosToShow = std::min(10, backlog.numPedidos);
        for (int i = 0; i < pedidosToShow; i++) {
            int pedidoId = pedidosOrdenados[i];
            const auto& info = analisador.infoPedidos[pedidoId];
            std::cout << std::setw(8) << pedidoId 
                      << std::setw(10) << info.numItens 
                      << std::setw(12) << info.numUnidades 
                      << std::setw(12) << info.numCorredoresMinimo 
                      << std::setw(15) << std::fixed << std::setprecision(2) << info.pontuacaoRelevancia 
                      << std::endl;
        }
        if (backlog.numPedidos > pedidosToShow) {
            std::cout << "... e mais " << (backlog.numPedidos - pedidosToShow) << " pedidos\n";
        }
        
        // Selecionar e exibir a melhor wave
        std::cout << "\n=== Seleção de Wave Ótima ===\n";
        GestorWaves gestor(deposito, backlog);
        auto melhorWave = gestor.selecionarMelhorWave();
        
        std::cout << "Melhor wave encontrada:\n";
        std::cout << "  Número de pedidos: " << melhorWave.pedidosIds.size() << std::endl;
        std::cout << "  Total de unidades: " << melhorWave.totalUnidades << " (LB=" << backlog.wave.LB 
                  << ", UB=" << backlog.wave.UB << ")" << std::endl;
        std::cout << "  Número de corredores necessários: " << melhorWave.corredoresNecessarios.size() 
                  << " de " << deposito.numCorredores << std::endl;
        
        std::cout << "  Pedidos na wave: ";
        int waveItemsToShow = std::min(10, static_cast<int>(melhorWave.pedidosIds.size()));
        for (int i = 0; i < waveItemsToShow; i++) {
            std::cout << melhorWave.pedidosIds[i] << " ";
        }
        if (melhorWave.pedidosIds.size() > waveItemsToShow) {
            std::cout << "... e mais " << (melhorWave.pedidosIds.size() - waveItemsToShow) << " pedidos";
        }
        std::cout << std::endl;
        
        // Exibir corredores necessários para a wave
        std::cout << "  Corredores necessários: ";
        int corridorsToShow = std::min(10, static_cast<int>(melhorWave.corredoresNecessarios.size()));
        int count = 0;
        for (int corredorId : melhorWave.corredoresNecessarios) {
            std::cout << corredorId << " ";
            if (++count >= corridorsToShow) break;
        }
        if (melhorWave.corredoresNecessarios.size() > corridorsToShow) {
            std::cout << "... e mais " << (melhorWave.corredoresNecessarios.size() - corridorsToShow) << " corredores";
        }
        std::cout << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Erro ao verificar estruturas auxiliares: " << e.what() << std::endl;
    }
}