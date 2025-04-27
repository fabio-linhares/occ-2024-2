#include "modules/selecao_otimizada.h"
#include <limits>
#include <iostream>
#include <algorithm>

bool selecionarPedidosOtimizado(const Warehouse& warehouse, 
                              AuxiliaryStructures& aux,
                              Solution& solution) {
    std::cout << "    Executando seleção otimizada de pedidos..." << std::endl;
    
    // Estruturas para acompanhar estado
    std::vector<int> estoque_disponivel(warehouse.numItems, 0);
    for (int item_id = 0; item_id < warehouse.numItems; item_id++) {
        // Calcular disponibilidade total para cada item em todos os corredores
        for (int c = 0; c < warehouse.numCorridors; c++) {
            for (const auto& item_pair : warehouse.corridors[c]) {
                if (item_pair.first == item_id) {
                    estoque_disponivel[item_id] += item_pair.second;
                }
            }
        }
    }
    
    std::set<int> corredores_visitados;
    int total_itens_selecionados = 0;
    
    // Obter pedidos priorizados
    std::vector<std::pair<int, double>> pedidos_priorizados;
    calcularPrioridadePedidos(aux, pedidos_priorizados);
    
    int pedidos_analisados = 0;
    int pedidos_aceitos = 0;
    
    // Seleção gulosa considerando valor marginal
    for (const auto& p_info : pedidos_priorizados) {
        int p_id = p_info.first;
        const auto& pedido = aux.pedidos_aprimorado[p_id];
        pedidos_analisados++;
        
        // Verificar limite superior
        if (total_itens_selecionados + pedido.total_itens > warehouse.UB) {
            continue;
        }
        
        // Verificar disponibilidade
        bool disponivel = true;
        for (const auto& item_pair : pedido.itens) {
            if (estoque_disponivel[item_pair.first] < item_pair.second) {
                disponivel = false;
                break;
            }
        }
        
        if (!disponivel) continue;
        
        // Calcular corredores adicionais necessários
        std::set<int> novos_corredores;
        for (int corredor_id : pedido.corredores_necessarios) {
            if (corredores_visitados.find(corredor_id) == corredores_visitados.end()) {
                novos_corredores.insert(corredor_id);
            }
        }
        
        // Calcular valor marginal
        double valor_marginal;
        if (novos_corredores.empty()) {
            valor_marginal = std::numeric_limits<double>::max(); // "Grátis"
        } else {
            valor_marginal = static_cast<double>(pedido.total_itens) / novos_corredores.size();
        }
        
        // Decidir com base no valor marginal
        if (valor_marginal > 0.0) {
            // Adicionar pedido à solução
            solution.addOrder(p_id, warehouse);
            pedidos_aceitos++;
            
            // Atualizar estado
            for (const auto& item_pair : pedido.itens) {
                estoque_disponivel[item_pair.first] -= item_pair.second;
            }
            
            for (int corredor_id : pedido.corredores_necessarios) {
                corredores_visitados.insert(corredor_id);
            }
            
            total_itens_selecionados += pedido.total_itens;
        }
    }
    
    std::cout << "    Seleção concluída: analisados " << pedidos_analisados 
              << " pedidos, aceitos " << pedidos_aceitos << std::endl;
    std::cout << "    Total de itens: " << total_itens_selecionados 
              << ", Corredores: " << corredores_visitados.size() << std::endl;
    
    // Verificar LB
    return total_itens_selecionados >= warehouse.LB;
}

void selecionarPedidosComplementares(const Warehouse& warehouse, 
                                   AuxiliaryStructures& aux,
                                   Solution& solution) {
    std::cout << "    Complementando solução para atingir limite inferior (LB)..." << std::endl;
    
    // Obter estado atual
    std::vector<int> estoque_disponivel(warehouse.numItems, 0);
    for (int item_id = 0; item_id < warehouse.numItems; item_id++) {
        // Calcular disponibilidade total para cada item em todos os corredores
        for (int c = 0; c < warehouse.numCorridors; c++) {
            for (const auto& item_pair : warehouse.corridors[c]) {
                if (item_pair.first == item_id) {
                    estoque_disponivel[item_id] += item_pair.second;
                }
            }
        }
    }
    
    std::set<int> corredores_visitados;
    int total_itens = 0;
    
    // Atualizar com base na solução atual
    const auto& pedidos_selecionados = solution.getSelectedOrders();
    for (int p_id : pedidos_selecionados) {
        // Obter informações do pedido diretamente do warehouse
        for (const auto& item_pair : warehouse.orders[p_id]) {
            estoque_disponivel[item_pair.first] -= item_pair.second;
            total_itens += item_pair.second;
        }
        
        // Atualizar corredores visitados
        if (p_id < aux.pedidos_aprimorado.size()) {
            for (int corredor_id : aux.pedidos_aprimorado[p_id].corredores_necessarios) {
                corredores_visitados.insert(corredor_id);
            }
        }
    }
    
    // Se já atingiu LB, não é necessário complementar
    if (total_itens >= warehouse.LB) {
        std::cout << "    LB já atingido (" << total_itens << " >= " << warehouse.LB << ")." << std::endl;
        return;
    }
    
    std::cout << "    Necessário complementar: " << warehouse.LB - total_itens << " itens." << std::endl;
    
    // Buscar pedidos complementares
    std::vector<std::pair<int, double>> pedidos_complementares;
    
    for (size_t p_id = 0; p_id < aux.pedidos_aprimorado.size(); p_id++) {
        // Ignorar pedidos já selecionados
        if (std::find(pedidos_selecionados.begin(), pedidos_selecionados.end(), 
                      p_id) != pedidos_selecionados.end()) {
            continue;
        }
        
        const auto& pedido = aux.pedidos_aprimorado[p_id];
        
        // Verificar limite superior
        if (total_itens + pedido.total_itens > warehouse.UB) {
            continue;
        }
        
        // Verificar disponibilidade
        bool disponivel = true;
        for (const auto& item_pair : pedido.itens) {
            if (estoque_disponivel[item_pair.first] < item_pair.second) {
                disponivel = false;
                break;
            }
        }
        
        if (!disponivel) continue;
        
        // Calcular novos corredores
        int novos_corredores = 0;
        for (int corredor_id : pedido.corredores_necessarios) {
            if (corredores_visitados.find(corredor_id) == corredores_visitados.end()) {
                novos_corredores++;
            }
        }
        
        // Priorizar pedidos com menor adição de corredores
        double valor_comp;
        if (novos_corredores == 0) {
            valor_comp = std::numeric_limits<double>::max();
        } else {
            valor_comp = static_cast<double>(pedido.total_itens) / novos_corredores;
        }
        
        pedidos_complementares.push_back({static_cast<int>(p_id), valor_comp});
    }
    
    // Ordenar pedidos complementares
    std::sort(pedidos_complementares.begin(), pedidos_complementares.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    int complemento_adicionado = 0;
    
    // Adicionar pedidos até atingir LB
    for (const auto& p_comp : pedidos_complementares) {
        int p_id = p_comp.first;
        const auto& pedido = aux.pedidos_aprimorado[p_id];
        
        // Verificar novamente limite e disponibilidade
        if (total_itens + pedido.total_itens > warehouse.UB) {
            continue;
        }
        
        bool disponivel = true;
        for (const auto& item_pair : pedido.itens) {
            if (estoque_disponivel[item_pair.first] < item_pair.second) {
                disponivel = false;
                break;
            }
        }
        
        if (!disponivel) continue;
        
        // Adicionar pedido
        solution.addOrder(p_id, warehouse);
        complemento_adicionado++;
        
        // Atualizar estado
        for (const auto& item_pair : pedido.itens) {
            estoque_disponivel[item_pair.first] -= item_pair.second;
        }
        
        total_itens += pedido.total_itens;
        
        if (total_itens >= warehouse.LB) {
            break;
        }
    }
    
    std::cout << "    Complemento concluído: adicionados " << complemento_adicionado 
              << " pedidos adicionais." << std::endl;
    std::cout << "    Total após complemento: " << total_itens << " itens." << std::endl;
}