#include "metricas.h"
#include <numeric>
#include <cmath>

MetricasInstancia calcularMetricas(const Problema& problema) {
    MetricasInstancia metricas;
    
    metricas.numero_itens = problema.i;
    metricas.numero_corredores = problema.a;
    metricas.numero_pedidos = problema.o;

    // Cálculo da densidade da matriz de cobertura
    int total_elementos = problema.i * problema.a;
    int elementos_cobertos = 0;
    for (int i = 0; i < problema.i; ++i) {
        for (int j = 0; j < problema.a; ++j) {
            if (problema.matriz_cobertura[i][j]) {
                elementos_cobertos++;
            }
        }
    }
    metricas.densidade_matriz_cobertura = (double)elementos_cobertos / total_elementos;

    // Calcular densidade média dos corredores
    int total_items_in_corridors = 0;
    for (const auto& corredor : problema.corredores) {
        total_items_in_corridors += corredor.estoque.size();
    }
    metricas.densidade_media_corredores = (double)total_items_in_corridors / problema.a;

    // Calcular variância dos itens por pedido
    std::vector<int> itens_por_pedido;
    for (const auto& pedido : problema.pedidos) {
        itens_por_pedido.push_back(pedido.total_itens);
    }
    
    // Calcular média
    double mean = std::accumulate(itens_por_pedido.begin(), itens_por_pedido.end(), 0.0) / itens_por_pedido.size();
    
    // Calcular variância
    double variance = 0.0;
    for (int itens : itens_por_pedido) {
        variance += (itens - mean) * (itens - mean);
    }
    variance /= itens_por_pedido.size();
    
    metricas.variancia_itens_por_pedido = variance;

    return metricas;
}