#include "../include/objective_function.h"
#include "../include/config_manager.h"

ObjectiveConfig loadObjectiveConfig() {
    return ConfigManager::getInstance().getObjectiveConfig();
}

double calcularRazao(const std::vector<int>& pedidos, const std::vector<int>& corredores, const Instance& instancia) {
    int total_itens_coletados = 0;
    for (int pedido_id : pedidos) {
        total_itens_coletados += instancia.pedidos[pedido_id].total_itens;
    }
    
    int num_corredores_visitados = corredores.size();
    if (num_corredores_visitados == 0) {
        return 0.0;
    }
    
    return static_cast<double>(total_itens_coletados) / num_corredores_visitados;
}
