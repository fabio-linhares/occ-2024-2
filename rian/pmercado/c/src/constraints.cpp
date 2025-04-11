#include "../include/constraints.h"
#include "../include/config_manager.h"
#include <map>

std::vector<ConstraintConfig> loadConstraintConfigs() {
    return ConfigManager::getInstance().getConstraintConfigs();
}

bool verificarLimiteInferior(const std::vector<int>& pedidos, const Instance& instancia, int lb) {
    int total_itens_coletados = 0;
    for (int pedido_id : pedidos) {
        total_itens_coletados += instancia.pedidos[pedido_id].total_itens;
    }
    return total_itens_coletados >= lb;
}

bool verificarLimiteSuperior(const std::vector<int>& pedidos, const Instance& instancia, int ub) {
    int total_itens_coletados = 0;
    for (int pedido_id : pedidos) {
        total_itens_coletados += instancia.pedidos[pedido_id].total_itens;
    }
    return total_itens_coletados <= ub;
}

bool verificarDisponibilidade(const std::vector<int>& pedidos, const std::vector<int>& corredores, const Instance& instancia) {
    std::map<int, int> itens_demandados;
    for (int pedido_id : pedidos) {
        for (const auto& [item_id, quantidade] : instancia.pedidos[pedido_id].itens) {
            itens_demandados[item_id] += quantidade;
        }
    }
    
    std::map<int, int> itens_disponiveis;
    for (int corredor_id : corredores) {
        for (const auto& [item_id, quantidade] : instancia.corredores[corredor_id].itens) {
            itens_disponiveis[item_id] += quantidade;
        }
    }
    
    for (const auto& [item_id, quantidade_demandada] : itens_demandados) {
        int quantidade_disponivel = itens_disponiveis.count(item_id) ? itens_disponiveis.at(item_id) : 0;
        if (quantidade_demandada > quantidade_disponivel) {
            return false;
        }
    }
    
    return true;
}

bool verificarTodasRestricoes(const std::vector<int>& pedidos, const std::vector<int>& corredores, const Instance& instancia) {
    return verificarLimiteInferior(pedidos, instancia, instancia.lb) &&
           verificarLimiteSuperior(pedidos, instancia, instancia.ub) &&
           verificarDisponibilidade(pedidos, corredores, instancia);
}
