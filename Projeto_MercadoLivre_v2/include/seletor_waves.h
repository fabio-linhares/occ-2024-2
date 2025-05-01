#pragma once

#include <vector>
#include <unordered_set>
#include "armazem.h"
#include "localizador_itens.h"
#include "analisador_relevancia.h"

/**
 * @brief Estrutura para seleção eficiente de waves
 */
struct SeletorWaves {
    /**
     * @brief Estrutura para representar uma wave candidata
     */
    struct WaveCandidata {
        std::vector<int> pedidosIds;
        int totalUnidades;
        std::unordered_set<int> corredoresNecessarios;
    };
    
    /**
     * @brief Seleciona uma wave ótima usando os pedidos ordenados por relevância
     * @param backlog Referência ao objeto Backlog
     * @param pedidosOrdenados Vetor de IDs de pedidos ordenados por relevância
     * @param analisador Referência ao objeto AnalisadorRelevancia
     * @param localizador Referência ao objeto LocalizadorItens
     * @return Uma WaveCandidata contendo a melhor seleção de pedidos
     */
    WaveCandidata selecionarWaveOtima(
        const Backlog& backlog,
        const std::vector<int>& pedidosOrdenados,
        const AnalisadorRelevancia& analisador,
        const LocalizadorItens& localizador);
};