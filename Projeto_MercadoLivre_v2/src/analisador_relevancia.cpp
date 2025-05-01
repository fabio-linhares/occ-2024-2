#include "analisador_relevancia.h"

void AnalisadorRelevancia::construir(const Backlog& backlog, const LocalizadorItens& localizador) {
    for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
        InfoPedido& info = infoPedidos[pedidoId];
        info.pedidoId = pedidoId;
        info.numItens = backlog.pedido[pedidoId].size();
        
        // Calcular número total de unidades
        info.numUnidades = 0;
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            info.numUnidades += quantidade;
        }
        
        // Calcular número mínimo de corredores necessários
        std::unordered_set<int> corredoresNecessarios;
        for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
            // Para cada item, encontrar o corredor com mais unidades disponíveis
            const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
            
            int quantidadeRestante = quantidadeSolicitada;
            // Ordenar corredores por quantidade disponível (decrescente)
            std::vector<std::pair<int, int>> corredoresOrdenados(
                corredoresComItem.begin(), corredoresComItem.end());
            std::sort(corredoresOrdenados.begin(), corredoresOrdenados.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (const auto& [corredorId, quantidadeDisponivel] : corredoresOrdenados) {
                if (quantidadeRestante <= 0) break;
                
                corredoresNecessarios.insert(corredorId);
                quantidadeRestante -= std::min(quantidadeRestante, quantidadeDisponivel);
            }
        }
        
        info.numCorredoresMinimo = corredoresNecessarios.size();
        
        // Calcular pontuação de relevância (mais itens, menos corredores = melhor)
        // Fórmula: (numItens * numUnidades) / numCorredoresMinimo
        info.pontuacaoRelevancia = (info.numItens * info.numUnidades) / 
                                  (double)std::max(1, info.numCorredoresMinimo);
    }
}

std::vector<int> AnalisadorRelevancia::getPedidosOrdenadosPorRelevancia() const {
    std::vector<int> pedidosOrdenados(infoPedidos.size());
    for (int i = 0; i < infoPedidos.size(); i++) {
        pedidosOrdenados[i] = i;
    }
    
    std::sort(pedidosOrdenados.begin(), pedidosOrdenados.end(),
        [this](int a, int b) {
            return infoPedidos[a].pontuacaoRelevancia > infoPedidos[b].pontuacaoRelevancia;
        });
    
    return pedidosOrdenados;
}