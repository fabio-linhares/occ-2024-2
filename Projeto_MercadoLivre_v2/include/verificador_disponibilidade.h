#pragma once

#include <vector>
#include <map>
#include "armazem.h"

/**
 * @brief Estrutura para verificação rápida de disponibilidade de itens
 */
struct VerificadorDisponibilidade {
    // itemId -> quantidade total disponível em todos os corredores
    std::vector<int> estoqueTotal;
    
    /**
     * @brief Construtor
     * @param numItens Número total de itens no depósito
     */
    VerificadorDisponibilidade(int numItens) : estoqueTotal(numItens, 0) {}
    
    /**
     * @brief Inicializa a estrutura a partir do depósito
     * @param deposito Referência ao objeto Deposito
     */
    void construir(const Deposito& deposito);
    
    /**
     * @brief Verifica se há estoque suficiente para um pedido
     * @param pedido Mapa de itens e quantidades solicitadas
     * @return true se há estoque suficiente, false caso contrário
     */
    bool verificarDisponibilidade(const std::map<int, int>& pedido) const;
};