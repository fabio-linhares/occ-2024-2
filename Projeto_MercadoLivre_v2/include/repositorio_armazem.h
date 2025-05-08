#pragma once

#include "armazem.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

/**
 * @brief Classe que centraliza operações complexas envolvendo Deposito e Backlog
 * 
 * Esta classe implementa o padrão Repositório e fornece métodos otimizados
 * para operações que envolvem ambas as estruturas, evitando duplicação de código
 * e garantindo consistência.
 */
class RepositorioArmazem {
private:
    const Deposito& deposito;
    const Backlog& backlog;
    
    // Cache para consultas frequentes
    mutable std::unordered_map<int, std::vector<int>> cachePedidosPorCorredor;
    mutable std::unordered_map<int, std::vector<int>> cacheCorredoresPorPedido;
    mutable std::unordered_map<std::string, double> cacheCompatibilidade;

public:
    /**
     * @brief Construtor
     * @param dep Referência ao depósito
     * @param back Referência ao backlog
     */
    RepositorioArmazem(const Deposito& dep, const Backlog& back) 
        : deposito(dep), backlog(back) {}
    
    /**
     * @brief Obter todos os pedidos que precisam de um corredor específico
     * @param corredorId ID do corredor
     * @return Vetor de IDs de pedidos que precisam deste corredor
     */
    std::vector<int> getPedidosPorCorredor(int corredorId) const;
    
    /**
     * @brief Calcula pedidos compatíveis com um determinado pedido
     * @param pedidoId ID do pedido de referência
     * @param limiteCompatibilidade Valor mínimo de compatibilidade (0.0-1.0)
     * @return Vetor de pares {pedidoId, compatibilidade} ordenados por compatibilidade
     */
    std::vector<std::pair<int, double>> getPedidosCompativeis(int pedidoId, double limiteCompatibilidade = 0.3) const;
    
    /**
     * @brief Verifica se um conjunto de pedidos pode formar uma wave válida
     * @param pedidosIds IDs dos pedidos
     * @return true se os pedidos formam uma wave válida, false caso contrário
     */
    bool validarWave(const std::vector<int>& pedidosIds) const;
    
    /**
     * @brief Obtém conjunto mínimo de corredores para atender um conjunto de pedidos
     * @param pedidosIds IDs dos pedidos
     * @return Conjunto de IDs de corredores necessários
     */
    std::unordered_set<int> getCorredoresMinimos(const std::vector<int>& pedidosIds) const;
    
    /**
     * @brief Calcula a eficiência de uma wave (unidades/corredores)
     * @param pedidosIds IDs dos pedidos na wave
     * @return Valor de eficiência da wave
     */
    double calcularEficienciaWave(const std::vector<int>& pedidosIds) const;
    
    /**
     * @brief Limpa todos os caches
     */
    void limparCache() const;
};