#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <string>
#include <map>
#include <vector>

// Estrutura para representar um pedido
struct Pedido {
    int id;
    std::map<int, int> itens;  // item_id -> quantidade
    int total_itens;
};

// Estrutura para representar um corredor
struct Corredor {
    int id;
    std::map<int, int> itens;  // item_id -> quantidade
};

// Estrutura para representar uma instância do problema
struct Instance {
    int num_pedidos;
    int num_itens;
    int num_corredores;
    std::vector<Pedido> pedidos;
    std::vector<Corredor> corredores;
    int lb;  // Limite inferior
    int ub;  // Limite superior
};

// Estrutura para representar uma solução
struct Solution {
    std::vector<int> pedidos_selecionados;
    std::vector<int> corredores_visitados;
    double valor_objetivo;
};

#endif // DATA_STRUCTURES_H