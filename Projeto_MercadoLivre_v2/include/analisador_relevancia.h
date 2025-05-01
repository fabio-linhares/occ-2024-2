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
    void construir(const Backlog& backlog, const LocalizadorItens& localizador);
    
    /**
     * @brief Obtém pedidos ordenados por relevância (do mais relevante para o menos)
     * @return Vetor de IDs de pedidos ordenados por relevância
     */
    std::vector<int> getPedidosOrdenadosPorRelevancia() const;
};