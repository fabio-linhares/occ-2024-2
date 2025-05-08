#pragma once

#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"

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

private:
    std::vector<InfoPedido> infoPedidos;

    /**
     * @brief Ordena os pedidos por relevância (decrescente)
     * @return Vector de IDs de pedidos ordenados por relevância
     */
    std::vector<int> ordenarPorRelevancia() const;

    /**
     * @brief Ordena os pedidos por relevância (decrescente) utilizando paralelismo
     * @return Vector de IDs de pedidos ordenados por relevância
     */
    std::vector<int> ordenarPorRelevanciaParalelo() const;

public:
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
     * @param forcarRecalculo Indica se o cálculo deve ser forçado mesmo que já tenha sido realizado
     */
    void calcularRelevancia(int pedidoId, const Backlog& backlog, const LocalizadorItens& localizador, bool forcarRecalculo = false);

    /**
     * @brief Calcula a relevância de múltiplos pedidos em lote
     * @param pedidosIds Vector de IDs dos pedidos
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     */
    void calcularRelevanciaEmLote(const std::vector<int>& pedidosIds, const Backlog& backlog, const LocalizadorItens& localizador);

    /**
     * @brief Definir estratégia clara para uso de paralelismo
     */
    enum class EstrategiaOrdenacao { SEQUENCIAL, PARALELO };
    std::vector<int> ordenarPedidos(EstrategiaOrdenacao estrategia = EstrategiaOrdenacao::SEQUENCIAL) const;

    /**
     * @brief Obtém as informações de relevância de um pedido
     * @param pedidoId ID do pedido
     * @return Referência constante para as informações do pedido
     * @throws std::out_of_range Se o pedidoId estiver fora dos limites válidos
     */
    const InfoPedido& getInfoPedido(int pedidoId) const {
        if (pedidoId < 0 || pedidoId >= infoPedidos.size())
            throw std::out_of_range("Índice de pedido inválido");
        return infoPedidos[pedidoId];
    }

    /**
     * @brief Analisa todos os pedidos e calcula sua relevância
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     * @param verificador Verificador de disponibilidade
     */
    void analisarTodosPedidos(const Backlog& backlog, 
                              const LocalizadorItens& localizador,
                              const VerificadorDisponibilidade& verificador) {
        // Verificar se as estruturas auxiliares foram inicializadas corretamente
        if (localizador.itemParaCorredor.empty() || verificador.estoqueTotal.empty()) {
            throw std::runtime_error("Estruturas auxiliares não inicializadas corretamente");
        }

        for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
            if (verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
                calcularRelevancia(pedidoId, backlog, localizador);
            }
        }
    }

    /**
     * @brief Versão melhorada: receber apenas os pedidos já filtrados
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     * @param pedidosDisponiveis Vector de IDs de pedidos disponíveis
     */
    void analisarPedidos(const Backlog& backlog,
                         const LocalizadorItens& localizador,
                         const std::vector<int>& pedidosDisponiveis) {
        for (int pedidoId : pedidosDisponiveis) {
            calcularRelevancia(pedidoId, backlog, localizador);
        }
    }

    /**
     * @brief Obter top-N pedidos mais relevantes
     * @param n Número de pedidos a serem retornados
     * @return Vector de IDs dos top-N pedidos mais relevantes
     */
    std::vector<int> obterTopPedidos(int n) const;

    /**
     * @brief Filtrar pedidos com relevância acima de um limiar
     * @param limiarMinimo Valor mínimo de relevância
     * @return Vector de IDs de pedidos com relevância acima do limiar
     */
    std::vector<int> filtrarPorRelevancia(double limiarMinimo) const;

    /**
     * @brief Atualiza a relevância de um pedido se necessário
     * @param pedidoId ID do pedido
     * @param backlog Dados do backlog
     * @param localizador Localizador de itens
     */
    void atualizarRelevanciaSeNecessario(int pedidoId, const Backlog& backlog, const LocalizadorItens& localizador);

    /**
     * @brief Verifica se a relevância de um pedido foi atualizada
     * @param pedidoId ID do pedido
     * @param backlog Dados do backlog
     * @return true se a relevância foi atualizada, false caso contrário
     */
    bool relevanciaAtualizada(int pedidoId, const Backlog& backlog) const;
};