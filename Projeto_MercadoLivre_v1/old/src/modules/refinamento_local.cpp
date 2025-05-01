#include "modules/refinamento_local.h"
#include <iostream>
#include <algorithm>

bool aplicarBuscaLocal(const Warehouse& warehouse, 
                      AuxiliaryStructures& aux, 
                      Solution& solution,
                      int max_iteracoes) {
    std::cout << "Aplicando busca local (max " << max_iteracoes << " iterações)..." << std::endl;
    
    // Implementação básica para resolver o erro de compilação
    double valor_inicial = solution.getObjectiveValue();
    bool melhorou = false;
    
    // Iterações de refinamento
    for (int i = 0; i < max_iteracoes; i++) {
        bool melhorou_iteracao = false;
        
        // Obter pedidos selecionados
        const auto& pedidos_selecionados = solution.getSelectedOrders();
        
        // Testar trocar um pedido por outro
        for (int p_out : pedidos_selecionados) {
            // Tentar remover
            Solution solucao_temp = solution;
            try {
                solucao_temp.removeOrder(p_out, warehouse);
            } catch (...) {
                continue;
            }
            
            // Se ficar abaixo de LB, pular
            if (solucao_temp.getTotalItems() < warehouse.LB) {
                continue;
            }
            
            // Testar adicionar outro pedido
            for (const auto& pedido : aux.pedidos_aprimorado) {
                // Pular pedidos já selecionados
                if (std::find(pedidos_selecionados.begin(), pedidos_selecionados.end(), 
                             pedido.id) != pedidos_selecionados.end()) {
                    continue;
                }
                
                // Verificar limite superior
                if (solucao_temp.getTotalItems() + pedido.total_itens > warehouse.UB) {
                    continue;
                }
                
                // Tentar adicionar
                Solution candidata = solucao_temp;
                try {
                    candidata.addOrder(pedido.id, warehouse);
                } catch (...) {
                    continue;
                }
                
                // Verificar se melhora
                if (candidata.getObjectiveValue() > solution.getObjectiveValue()) {
                    // Aplicar melhoria
                    solution = candidata;
                    melhorou = true;
                    melhorou_iteracao = true;
                    break;
                }
            }
            
            if (melhorou_iteracao) {
                break;  // Começar nova iteração
            }
        }
        
        if (!melhorou_iteracao) {
            break;  // Sem melhorias, parar
        }
    }
    
    if (melhorou) {
        std::cout << "Solução melhorada com busca local: " 
                  << valor_inicial << " -> " << solution.getObjectiveValue() << std::endl;
    } else {
        std::cout << "Busca local não encontrou melhorias." << std::endl;
    }
    
    return melhorou;
}