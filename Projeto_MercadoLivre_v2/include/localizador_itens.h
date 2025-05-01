#pragma once

#include <vector>
#include <unordered_map>
#include "armazem.h"

/**
 * @brief Estrutura para localização rápida de itens nos corredores
 */
struct LocalizadorItens {
    // itemId -> {corredorId -> quantidade}
    std::vector<std::unordered_map<int, int>> itemParaCorredor;
    
    /**
     * @brief Construtor
     * @param numItens Número total de itens no depósito
     */
    LocalizadorItens(int numItens) : itemParaCorredor(numItens) {}
    
    /**
     * @brief Inicializa a estrutura a partir do depósito
     * @param deposito Referência ao objeto Deposito
     */
    void construir(const Deposito& deposito);
    
    /**
     * @brief Obtém todos os corredores que contêm um item específico
     * @param itemId ID do item a ser localizado
     * @return Mapa de corredores e suas quantidades disponíveis
     */
    const std::unordered_map<int, int>& getCorredoresComItem(int itemId) const;
};