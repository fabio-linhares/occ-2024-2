#include "repositorio_armazem.h"
#include <algorithm>
#include <sstream>

std::vector<int> RepositorioArmazem::getPedidosPorCorredor(int corredorId) const {
    auto it = cachePedidosPorCorredor.find(corredorId);
    if (it != cachePedidosPorCorredor.end()) {
        return it->second;
    }
    
    std::vector<int> pedidos;
    for (int p = 0; p < backlog.numPedidos; p++) {
        auto corredoresNecessarios = backlog.getCorredoresNecessarios(p, deposito);
        if (corredoresNecessarios.find(corredorId) != corredoresNecessarios.end()) {
            pedidos.push_back(p);
        }
    }
    
    cachePedidosPorCorredor[corredorId] = pedidos;
    return pedidos;
}

std::vector<std::pair<int, double>> RepositorioArmazem::getPedidosCompativeis(
    int pedidoId, double limiteCompatibilidade) const {
    
    std::vector<std::pair<int, double>> resultado;
    
    for (int p = 0; p < backlog.numPedidos; p++) {
        if (p == pedidoId) continue;
        
        // Gerar chave única para o par de pedidos
        std::string chave = (pedidoId < p) ? 
            std::to_string(pedidoId) + ":" + std::to_string(p) :
            std::to_string(p) + ":" + std::to_string(pedidoId);
            
        double compatibilidade;
        auto it = cacheCompatibilidade.find(chave);
        
        if (it != cacheCompatibilidade.end()) {
            compatibilidade = it->second;
        } else {
            compatibilidade = backlog.calcularCompatibilidade(pedidoId, p, deposito);
            cacheCompatibilidade[chave] = compatibilidade;
        }
        
        if (compatibilidade >= limiteCompatibilidade) {
            resultado.push_back({p, compatibilidade});
        }
    }
    
    // Ordenar por compatibilidade (decrescente)
    std::sort(resultado.begin(), resultado.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return resultado;
}

bool RepositorioArmazem::validarWave(const std::vector<int>& pedidosIds) const {
    // Verificar limites da wave
    if (!backlog.wave.validarConjuntoPedidos(pedidosIds, backlog)) {
        return false;
    }
    
    // Verificar disponibilidade de estoque
    return deposito.verificarDisponibilidadeConjunto(pedidosIds, backlog);
}

std::unordered_set<int> RepositorioArmazem::getCorredoresMinimos(const std::vector<int>& pedidosIds) const {
    std::unordered_set<int> corredores;
    
    for (int pedidoId : pedidosIds) {
        auto corredoresPedido = backlog.getCorredoresNecessarios(pedidoId, deposito);
        corredores.insert(corredoresPedido.begin(), corredoresPedido.end());
    }
    
    return corredores;
}

double RepositorioArmazem::calcularEficienciaWave(const std::vector<int>& pedidosIds) const {
    if (pedidosIds.empty()) return 0.0;
    
    int totalUnidades = 0;
    for (int pedidoId : pedidosIds) {
        totalUnidades += backlog.calcularTotalUnidades(pedidoId);
    }
    
    auto corredores = getCorredoresMinimos(pedidosIds);
    if (corredores.empty()) return 0.0;
    
    return static_cast<double>(totalUnidades) / corredores.size();
}

void RepositorioArmazem::limparCache() const {
    cachePedidosPorCorredor.clear();
    cacheCorredoresPorPedido.clear();
    cacheCompatibilidade.clear();
}