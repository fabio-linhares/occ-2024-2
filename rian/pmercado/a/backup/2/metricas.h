
#ifndef METRICAS_H

#define METRICAS_H



#include "problema.h"



struct MetricasInstancia {

    int numero_itens;

    int numero_corredores;

    int numero_pedidos;

    double densidade_matriz_cobertura;

};



MetricasInstancia calcularMetricas(const Problema& problema);



#endif // METRICAS_H

