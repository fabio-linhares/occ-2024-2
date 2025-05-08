#include "seletor_waves.h"

SeletorWaves::WaveCandidata SeletorWaves::selecionarWaveOtima(
    const Backlog& backlog,
    const std::vector<int>& pedidosOrdenados,
    const AnalisadorRelevancia& analisador,
    const LocalizadorItens& localizador) {
    
    WaveCandidata melhorWave;
    melhorWave.totalUnidades = 0;
    
    WaveCandidata waveAtual;
    waveAtual.totalUnidades = 0;
    
    for (int pedidoId : pedidosOrdenados) {
        // Verificar se adicionar este pedido excederia o limite superior (UB)
        int unidadesPedido = analisador.getInfoPedido(pedidoId).numUnidades;
        if (waveAtual.totalUnidades + unidadesPedido > backlog.wave.UB) {
            // Se já atingimos o limite inferior (LB), esta wave é válida
            if (waveAtual.totalUnidades >= backlog.wave.LB) {
                // Se for melhor que a melhor wave encontrada até agora
                if (melhorWave.totalUnidades == 0 || 
                    waveAtual.corredoresNecessarios.size() < melhorWave.corredoresNecessarios.size()) {
                    melhorWave = waveAtual;
                }
            }
            continue;
        }
        
        // Adicionar pedido à wave atual
        waveAtual.pedidosIds.push_back(pedidoId);
        waveAtual.totalUnidades += unidadesPedido;
        
        // Atualizar corredores necessários
        for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
            int quantidadeRestante = quantidadeSolicitada;
            const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
            
            std::vector<std::pair<int, int>> corredoresOrdenados(
                corredoresComItem.begin(), corredoresComItem.end());
            std::sort(corredoresOrdenados.begin(), corredoresOrdenados.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (const auto& [corredorId, quantidadeDisponivel] : corredoresOrdenados) {
                if (quantidadeRestante <= 0) break;
                
                waveAtual.corredoresNecessarios.insert(corredorId);
                quantidadeRestante -= std::min(quantidadeRestante, quantidadeDisponivel);
            }
        }
        
        // Se a wave atual já é válida e melhor que a melhor encontrada
        if (waveAtual.totalUnidades >= backlog.wave.LB) {
            if (melhorWave.totalUnidades == 0 || 
                waveAtual.corredoresNecessarios.size() < melhorWave.corredoresNecessarios.size()) {
                melhorWave = waveAtual;
            }
        }
    }
    
    return melhorWave;
}