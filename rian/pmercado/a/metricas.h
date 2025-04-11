#ifndef METRICAS_H
#define METRICAS_H

#include "problema.h"

struct MetricasInstancia {
    int numero_itens;
    int numero_corredores;
    int numero_pedidos;
    double densidade_matriz_cobertura;
    double variancia_itens_por_pedido;  // Campo adicionado
    double densidade_media_corredores;  // Campo adicionado
};

MetricasInstancia calcularMetricas(const Problema& problema);

#endif // METRICAS_H

