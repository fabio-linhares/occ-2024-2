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
    explicit AnalisadorRelevancia(int numPedidos);
    
    /**
     * @brief Calcula a relevância de um pedido com base na sua eficiência
     * @param pedidoId ID do pedido
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     */
    void calcularRelevancia(int pedidoId, const Backlog& backlog, const LocalizadorItens& localizador);
    
    /**
     * @brief Ordena os pedidos por relevância (decrescente)
     * @return Vector de IDs de pedidos ordenados por relevância
     */
    std::vector<int> ordenarPorRelevancia() const;
};