#pragma once

#include <vector>
#include <unordered_set>
#include <algorithm>
#include "armazem.h"
#include "localizador_itens.h"

/**
 * @brief Estrutura para análise e ordenação de pedidos por relevância
 */
struct AnalisadorRelevancia {
    /**
     * @brief Estrutura para armazenar informações de relevância de um pedido
     */
    struct InfoPedido {
        int pedidoId;
        int numItens;            // Quantidade de tipos de itens diferentes
        int numUnidades;         // Quantidade total de unidades
        int numCorredoresMinimo; // Número mínimo de corredores necessários
        double pontuacaoRelevancia; // Pontuação calculada de relevância
    };
    
    std::vector<InfoPedido> infoPedidos;
    
    /**
     * @brief Construtor
     * @param numPedidos Número total de pedidos no backlog
     */
    AnalisadorRelevancia(int numPedidos) : infoPedidos(numPedidos) {}
    
    /**
     * @brief Inicializa a estrutura a partir do backlog e do localizador de itens
     * @param backlog Referência ao objeto Backlog
     * @param localizador Referência ao objeto LocalizadorItens
     */
    void construir(const Backlog& backlog, const LocalizadorItens& localizador) {
        for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
            InfoPedido& info = infoPedidos[pedidoId];
            info.pedidoId = pedidoId;
            info.numItens = backlog.pedido[pedidoId].size();
            
            // Calcular número total de unidades
            info.numUnidades = 0;
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                info.numUnidades += quantidade;
            }
            
            // Calcular número mínimo de corredores necessários
            std::unordered_set<int> corredoresNecessarios;
            for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
                
                int quantidadeRestante = quantidadeSolicitada;
                // Ordenar corredores por quantidade disponível (decrescente)
                std::vector<std::pair<int, int>> corredoresOrdenados(
                    corredoresComItem.begin(), corredoresComItem.end());
                std::sort(corredoresOrdenados.begin(), corredoresOrdenados.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                
                for (const auto& [corredorId, quantidadeDisponivel] : corredoresOrdenados) {
                    if (quantidadeRestante <= 0) break;
                    
                    corredoresNecessarios.insert(corredorId);
                    quantidadeRestante -= std::min(quantidadeRestante, quantidadeDisponivel);
                }
            }
            
            info.numCorredoresMinimo = corredoresNecessarios.size();
            
            // Calcular pontuação de relevância (mais itens, menos corredores = melhor)
            info.pontuacaoRelevancia = (info.numItens * info.numUnidades) / 
                                      (double)std::max(1, info.numCorredoresMinimo);
        }
    }
    
    /**
     * @brief Obtém pedidos ordenados por relevância (do mais relevante para o menos)
     * @return Vetor de IDs de pedidos ordenados por relevância
     */
    std::vector<int> getPedidosOrdenadosPorRelevancia() const {
        std::vector<int> pedidosOrdenados(infoPedidos.size());
        for (int i = 0; i < infoPedidos.size(); i++) {
            pedidosOrdenados[i] = i;
        }
        
        std::sort(pedidosOrdenados.begin(), pedidosOrdenados.end(),
            [this](int a, int b) {
                return infoPedidos[a].pontuacaoRelevancia > infoPedidos[b].pontuacaoRelevancia;
            });
        
        return pedidosOrdenados;
    }
};