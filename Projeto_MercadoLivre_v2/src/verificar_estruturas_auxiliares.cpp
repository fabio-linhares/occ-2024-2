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
    try {
        // Carregar a instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(filePath);
        
        std::cout << "\n=== Verificando estruturas auxiliares para: " << filePath << " ===\n";
        
        // === Localizador de Itens ===
        std::cout << "\n--- Localizador de Itens ---\n";
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);
        
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
        
        // === Verificador de Disponibilidade ===
        std::cout << "\n--- Verificador de Disponibilidade ---\n";
        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);
        
        for (int i = 0; i < itemsToShow; i++) {
            std::cout << "Item " << i << ": " << verificador.estoqueTotal[i] << " unidades disponíveis\n";
        }
        if (deposito.numItens > itemsToShow) {
            std::cout << "... e mais " << (deposito.numItens - itemsToShow) << " itens\n";
        }
        
        // === Analisador de Relevância ===
        std::cout << "\n--- Analisador de Relevância ---\n";
        AnalisadorRelevancia analisador(backlog.numPedidos);
        
        // CORREÇÃO: Iterar pedidos e calcular relevância em vez de chamar construir
        for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
            if (verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
                analisador.calcularRelevancia(pedidoId, backlog, localizador);
            }
        }
        
        // CORREÇÃO: Usar ordenarPorRelevancia em vez de getPedidosOrdenadosPorRelevancia
        auto pedidosOrdenados = analisador.ordenarPorRelevancia();
        
        // Mostrar os pedidos ordenados por relevância
        std::cout << "Pedidos ordenados por relevância (top 10):\n";
        int contador = 0;
        for (int pedidoId : pedidosOrdenados) {
            auto& info = analisador.infoPedidos[pedidoId];
            std::cout << "Pedido #" << pedidoId 
                      << " - Itens: " << info.numItens
                      << ", Unidades: " << info.numUnidades
                      << ", Corredores: " << info.numCorredoresMinimo
                      << ", Relevância: " << std::fixed << std::setprecision(2) << info.pontuacaoRelevancia << "\n";
            
            contador++;
            if (contador >= 10) break;
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
        
        std::cout << "\nVerificação de estruturas auxiliares concluída.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Erro ao verificar estruturas auxiliares: " << e.what() << std::endl;
    }
}