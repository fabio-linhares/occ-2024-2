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
        int numItens;
        int numUnidades;
        int numCorredoresMinimo;
        std::unordered_set<int> corredoresNecessarios;
        double pontuacaoRelevancia;
    };
    
    std::vector<InfoPedido> infoPedidos;
    
    /**
     * @brief Construtor
     * @param numPedidos Número total de pedidos
     */
    AnalisadorRelevancia(int numPedidos) : infoPedidos(numPedidos) {
        for (int i = 0; i < numPedidos; i++) {
            infoPedidos[i].pedidoId = i;
            infoPedidos[i].numItens = 0;
            infoPedidos[i].numUnidades = 0;
            infoPedidos[i].numCorredoresMinimo = 0;
            infoPedidos[i].pontuacaoRelevancia = 0.0;
        }
    }
    
    /**
     * @brief Calcula a relevância de um pedido com base na sua eficiência
     * @param pedidoId ID do pedido
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     */
    void calcularRelevancia(int pedidoId, const Backlog& backlog, const LocalizadorItens& localizador) {
        InfoPedido& info = infoPedidos[pedidoId];
        info.pedidoId = pedidoId;
        info.numItens = backlog.pedido[pedidoId].size();
        info.numUnidades = 0;
        info.corredoresNecessarios.clear();
        
        // Calcular número total de unidades e corredores necessários
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            info.numUnidades += quantidade;
            
            // Adicionar todos os corredores onde este item está disponível
            for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                info.corredoresNecessarios.insert(corredorId);
            }
        }
        
        info.numCorredoresMinimo = info.corredoresNecessarios.size();
        
        // MODIFICADO: Foco direto na maximização do valor objetivo
        // Antes: (info.numItens * info.numUnidades) / (double)std::max(1, info.numCorredoresMinimo)
        info.pontuacaoRelevancia = info.numUnidades / (double)std::max(1, info.numCorredoresMinimo);
    }
    
    /**
     * @brief Ordena os pedidos por relevância (decrescente)
     * @return Vector de IDs de pedidos ordenados por relevância
     */
    std::vector<int> ordenarPorRelevancia() const {
        std::vector<int> pedidosOrdenados;
        pedidosOrdenados.reserve(infoPedidos.size());
        
        for (const auto& info : infoPedidos) {
            pedidosOrdenados.push_back(info.pedidoId);
        }
        
        std::sort(pedidosOrdenados.begin(), pedidosOrdenados.end(),
                 [this](int a, int b) {
                     return infoPedidos[a].pontuacaoRelevancia > infoPedidos[b].pontuacaoRelevancia;
                 });
        
        return pedidosOrdenados;
    }
};