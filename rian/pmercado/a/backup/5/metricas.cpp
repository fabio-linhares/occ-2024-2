#include "metricas.h"

MetricasInstancia calcularMetricas(const Problema& problema) {
    MetricasInstancia metricas;
    // Implementar o cálculo das métricas
    metricas.numero_itens = problema.i;
    metricas.numero_corredores = problema.a;
    metricas.numero_pedidos = problema.pedidos.size();

    // Exemplo de cálculo da densidade da matriz de cobertura
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

    return metricas;
}