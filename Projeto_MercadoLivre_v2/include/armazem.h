#pragma once

#include <vector>
#include <map>

/**
 * @brief Estrutura para armazenar informações sobre wave
 */
struct WaveInfo {
    int LB; // Limite inferior
    int UB; // Limite superior
};

/**
 * @brief Estrutura para armazenar informações sobre o depósito
 */
struct Deposito {
    int numItens;
    int numCorredores;
    std::vector<std::map<int, int>> corredor; // corredor[corredorId][itemId] = quantidade
};

/**
 * @brief Estrutura para armazenar informações sobre o backlog de pedidos
 */
struct Backlog {
    int numPedidos;
    std::vector<std::map<int, int>> pedido; // pedido[pedidoId][itemId] = quantidade
    WaveInfo wave;
};