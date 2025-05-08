#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include "armazem.h"

// Declaração antecipada para evitar inclusão circular
class LocalizadorItens;

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
    bool verificarDisponibilidade(const std::unordered_map<int, int>& pedido) const;
    
    /**
     * @brief Verifica se há estoque suficiente para um conjunto de pedidos
     * @param pedidosIds Vetor de IDs dos pedidos
     * @param backlog Dados do backlog
     * @return true se há estoque suficiente, false caso contrário
     */
    bool verificarDisponibilidadeConjunto(
        const std::vector<int>& pedidosIds,
        const Backlog& backlog) const;
        
    /**
     * @brief Tenta reparar uma solução inviável
     * @param pedidosWave Lista de pedidos atual
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     * @return Lista corrigida de pedidos
     */
    std::vector<int> repararSolucao(
        const std::vector<int>& pedidosWave,
        int LB,
        int UB,
        const Backlog& backlog,
        const LocalizadorItens& localizador) const;
    
    /**
     * @brief Calcula o número de corredores únicos necessários para um conjunto de pedidos
     * @param pedidosIds IDs dos pedidos
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     * @return Número de corredores únicos
     */
    int calcularNumCorredoresUnicos(
        const std::vector<int>& pedidosIds,
        const Backlog& backlog,
        const LocalizadorItens& localizador) const;
    
    /**
     * @brief Verifica se um conjunto de pedidos respeita os limites LB e UB
     * @param pedidosIds IDs dos pedidos
     * @param backlog Dados do backlog
     * @param LB Limite inferior
     * @param UB Limite superior
     * @return true se os limites são respeitados, false caso contrário
     */
    bool verificarLimites(
        const std::vector<int>& pedidosIds,
        const Backlog& backlog,
        int LB,
        int UB) const;
};