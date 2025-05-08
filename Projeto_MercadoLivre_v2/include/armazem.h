#pragma once

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <algorithm>
#include <unordered_set>

// Adicione esta linha - declaração avançada (forward declaration)
struct Backlog;

/**
 * @brief Estrutura para armazenar informações sobre wave
 * 
 * A wave representa os limites operacionais para processamento de pedidos
 * com limite inferior (LB) e superior (UB) de unidades.
 */
struct WaveInfo {
    int32_t LB; // Limite inferior
    int32_t UB; // Limite superior
    
    // Construtor com valores padrão
    WaveInfo(int32_t lb = 0, int32_t ub = 0) : LB(lb), UB(ub) {
        if (ub < lb && ub != 0) {
            throw std::invalid_argument("UB deve ser maior ou igual a LB");
        }
    }
    
    // Verificar se um valor está dentro dos limites
    bool dentroDosLimites(int32_t valor) const {
        return valor >= LB && valor <= UB;
    }

    // Verifica se um conjunto de pedidos está dentro dos limites da wave
    bool validarConjuntoPedidos(const std::vector<int>& pedidosIds, const Backlog& backlog) const;

    // Calcula a capacidade restante dentro da wave (útil para algoritmos gulosos)
    int calcularCapacidadeRestante(int unidadesAtuais) const {
        return unidadesAtuais < LB ? LB - unidadesAtuais : UB - unidadesAtuais;
    }
};

/**
 * @brief Estrutura para armazenar informações sobre o depósito
 * 
 * O depósito contém corredores, cada um com vários itens em determinadas quantidades.
 */
struct Deposito {
    int32_t numItens;
    int32_t numCorredores;
    std::vector<std::unordered_map<int32_t, int32_t>> corredor; // corredor[corredorId][itemId] = quantidade
    
    // Construtor com inicialização adequada
    Deposito(int32_t nItens = 0, int32_t nCorredores = 0) 
        : numItens(nItens), numCorredores(nCorredores) {
        corredor.resize(numCorredores);
    }
    
    // Obter quantidade disponível de um item em um corredor
    int32_t getQuantidadeItem(int32_t corredorId, int32_t itemId) const {
        if (corredorId < 0 || corredorId >= numCorredores) {
            return 0;
        }
        
        auto it = corredor[corredorId].find(itemId);
        return (it != corredor[corredorId].end()) ? it->second : 0;
    }
    
    /**
     * @brief Verifica se um corredor possui um item específico
     * 
     * Este método verifica com segurança se um determinado corredor possui um item específico.
     * Ele também faz verificações de limites para garantir que o corredorId é válido.
     * 
     * @param corredorId ID do corredor a ser verificado (deve estar entre 0 e numCorredores-1)
     * @param itemId ID do item a ser procurado
     * @return true se o corredor contiver o item, false caso contrário ou se o corredorId for inválido
     */
    bool corredorPossuiItem(int32_t corredorId, int32_t itemId) const {
        if (corredorId < 0 || corredorId >= numCorredores) {
            return false;
        }
        
        return corredor[corredorId].find(itemId) != corredor[corredorId].end();
    }
    
    // Calcular quantos corredores têm um determinado item
    int32_t contarCorredoresComItem(int32_t itemId) const {
        int32_t count = 0;
        for (int32_t i = 0; i < numCorredores; ++i) {
            if (corredorPossuiItem(i, itemId)) {
                count++;
            }
        }
        return count;
    }
    
    // Processar itens em um corredor usando método auxiliar
    void processarItensCorredor(int32_t corredorId, int32_t itemId) {
        if (corredorPossuiItem(corredorId, itemId)) {
            int quantidade = getQuantidadeItem(corredorId, itemId);
            // Processamento
        }
    }

    // Obter lista de corredores que possuem um item
    std::vector<int> getCorredoresComItem(int itemId) const {
        std::vector<int> corredoresComItem;
        for (int32_t i = 0; i < numCorredores; ++i) {
            if (corredorPossuiItem(i, itemId)) {
                corredoresComItem.push_back(i);
            }
        }
        return corredoresComItem;
    }

    // Obter total de estoque de um item no depósito
    int getTotalEstoqueItem(int itemId) const {
        int totalEstoque = 0;
        for (int32_t i = 0; i < numCorredores; ++i) {
            totalEstoque += getQuantidadeItem(i, itemId);
        }
        return totalEstoque;
    }

    // Verifica se há estoque suficiente para atender um conjunto de pedidos
    bool verificarDisponibilidadeConjunto(const std::vector<int>& pedidosIds, const Backlog& backlog) const;

    /**
     * @brief Calcula a densidade de itens por corredor
     * 
     * Este método calcula a proporção de itens únicos por corredor em relação ao total de itens.
     * Útil para análise de layout e otimização de picking.
     * 
     * @return Vetor com valores de densidade para cada corredor (entre 0.0 e 1.0)
     */
    std::vector<double> calcularDensidadeCorredores() const {
        std::vector<double> densidade(numCorredores);
        for (int c = 0; c < numCorredores; c++) {
            densidade[c] = static_cast<double>(corredor[c].size()) / numItens;
        }
        return densidade;
    }

    // Identifica os corredores mais importantes (com mais itens únicos)
    std::vector<int> getCorredoresPrioritarios(int limite) const {
        // Cria vetor de pares {corredorId, numItens}
        std::vector<std::pair<int, int>> corredoresInfo;
        for (int c = 0; c < numCorredores; c++) {
            corredoresInfo.push_back({c, static_cast<int>(corredor[c].size())});
        }
        
        // Ordena por número de itens (decrescente)
        std::sort(corredoresInfo.begin(), corredoresInfo.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Extrai os IDs dos corredores mais importantes
        std::vector<int> resultado;
        int numRetorno = std::min(limite, static_cast<int>(corredoresInfo.size()));
        for (int i = 0; i < numRetorno; i++) {
            resultado.push_back(corredoresInfo[i].first);
        }
        return resultado;
    }
};

/**
 * @brief Estrutura para armazenar informações sobre o backlog de pedidos
 * 
 * O backlog contém pedidos pendentes e informações da wave para processamento.
 */
struct Backlog {
    int32_t numPedidos;
    std::vector<std::unordered_map<int32_t, int32_t>> pedido; // pedido[pedidoId][itemId] = quantidade
    WaveInfo wave;

private:
    // Cache para cálculos frequentes
    mutable std::unordered_map<int, int> cacheUnidadesPedido;
    mutable std::unordered_map<int, std::unordered_set<int>> cacheCorredoresNecessarios;

public:
    // Construtor com inicialização adequada
    Backlog(int32_t nPedidos = 0, const WaveInfo& waveInfo = WaveInfo()) 
        : numPedidos(nPedidos), wave(waveInfo) {
        pedido.resize(numPedidos);
    }
    
    // Obter quantidade solicitada de um item em um pedido
    int32_t getQuantidadeItem(int32_t pedidoId, int32_t itemId) const {
        if (pedidoId < 0 || pedidoId >= numPedidos) {
            return 0;
        }
        
        auto it = pedido[pedidoId].find(itemId);
        return (it != pedido[pedidoId].end()) ? it->second : 0;
    }
    
    /**
     * @brief Calcula o total de unidades em um pedido
     * 
     * Este método calcula a soma de todas as quantidades de itens em um pedido específico.
     * O resultado é armazenado em cache para melhorar o desempenho em chamadas subsequentes.
     * 
     * @param pedidoId ID do pedido (deve estar entre 0 e numPedidos-1)
     * @return Total de unidades no pedido, ou 0 se o pedidoId for inválido
     */
    int32_t calcularTotalUnidades(int32_t pedidoId) const {
        if (pedidoId < 0 || pedidoId >= numPedidos) {
            return 0;
        }
        
        // Verificar se já está em cache
        auto it = cacheUnidadesPedido.find(pedidoId);
        if (it != cacheUnidadesPedido.end()) {
            return it->second;
        }
        
        // Calcular e armazenar em cache
        int32_t total = 0;
        for (const auto& [itemId, quantidade] : pedido[pedidoId]) {
            total += quantidade;
        }
        cacheUnidadesPedido[pedidoId] = total;
        return total;
    }
    
    // Verificar se um pedido contém um item específico
    bool pedidoContemItem(int32_t pedidoId, int32_t itemId) const {
        if (pedidoId < 0 || pedidoId >= numPedidos) {
            return false;
        }
        
        return pedido[pedidoId].find(itemId) != pedido[pedidoId].end();
    }

    // Obter lista de pedidos que possuem um item
    std::vector<int> getPedidosComItem(int itemId) const {
        std::vector<int> pedidosComItem;
        for (int32_t i = 0; i < numPedidos; ++i) {
            if (pedidoContemItem(i, itemId)) {
                pedidosComItem.push_back(i);
            }
        }
        return pedidosComItem;
    }

    // Versão otimizada com cache
    std::unordered_set<int> getCorredoresNecessarios(int pedidoId, const Deposito& deposito) const {
        if (pedidoId < 0 || pedidoId >= numPedidos) return {};
        
        // Verificar se já está em cache
        auto it = cacheCorredoresNecessarios.find(pedidoId);
        if (it != cacheCorredoresNecessarios.end()) {
            return it->second;
        }
        
        // Calcular e armazenar em cache
        std::unordered_set<int> corredores;
        for (const auto& [itemId, _] : pedido[pedidoId]) {
            auto corredoresItem = deposito.getCorredoresComItem(itemId);
            for (int c : corredoresItem) {
                corredores.insert(c);
            }
        }
        cacheCorredoresNecessarios[pedidoId] = corredores;
        return corredores;
    }

    // Calcula a "compatibilidade" entre dois pedidos (% de corredores compartilhados)
    double calcularCompatibilidade(int pedido1, int pedido2, const Deposito& deposito) const {
        auto corredores1 = getCorredoresNecessarios(pedido1, deposito);
        auto corredores2 = getCorredoresNecessarios(pedido2, deposito);
        
        if (corredores1.empty() || corredores2.empty()) return 0.0;
        
        // Conta interseção
        int compartilhados = 0;
        for (int c : corredores1) {
            if (corredores2.find(c) != corredores2.end()) {
                compartilhados++;
            }
        }
        
        // União = A + B - interseção
        int uniao = corredores1.size() + corredores2.size() - compartilhados;
        return static_cast<double>(compartilhados) / uniao;
    }

    // Filtra pedidos que estão dentro de um intervalo de unidades
    std::vector<int> filtrarPedidosPorTamanho(int minUnidades, int maxUnidades) const {
        std::vector<int> resultado;
        for (int p = 0; p < numPedidos; p++) {
            int totalUnidades = calcularTotalUnidades(p);
            if (totalUnidades >= minUnidades && totalUnidades <= maxUnidades) {
                resultado.push_back(p);
            }
        }
        return resultado;
    }

    // Método para limpar cache (importante quando dados mudam)
    void limparCache() const {
        cacheUnidadesPedido.clear();
        cacheCorredoresNecessarios.clear();
    }
};

// Implementação do método validarConjuntoPedidos
inline bool WaveInfo::validarConjuntoPedidos(const std::vector<int>& pedidosIds, const Backlog& backlog) const {
    int totalUnidades = 0;
    for (int pedidoId : pedidosIds) {
        totalUnidades += backlog.calcularTotalUnidades(pedidoId);
    }
    return dentroDosLimites(totalUnidades);
}

// Implementação do método verificarDisponibilidadeConjunto
inline bool Deposito::verificarDisponibilidadeConjunto(const std::vector<int>& pedidosIds, const Backlog& backlog) const {
    std::unordered_map<int, int> demandaTotal;
    
    for (int pedidoId : pedidosIds) {
        if (pedidoId < 0 || pedidoId >= backlog.numPedidos) continue;
        
        for (const auto& [itemId, qtd] : backlog.pedido[pedidoId]) {
            demandaTotal[itemId] += qtd;
        }
    }
    
    // Verificar se há estoque suficiente
    for (const auto& [itemId, qtdTotal] : demandaTotal) {
        if (getTotalEstoqueItem(itemId) < qtdTotal) {
            return false;
        }
    }
    
    return true;
}