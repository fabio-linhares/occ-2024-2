#pragma once

#include <vector>
#include <unordered_map>
#include <set>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

struct Pedido {
    int index;
    std::vector<std::pair<int, int>> itens; // (item_id, quantidade)
    int total_itens;
};

struct Corredor {
    int index;
    std::vector<std::pair<int, int>> estoque; // (item_id, quantidade)
};

struct PrioridadeProduto {
    int id;
    double valor_prioridade;
    int demanda_total;
    int num_corredores_disponivel;
};

struct PrioridadeCorredor {
    int id;
    double valor_prioridade;
    std::vector<int> produtos_exclusivos;
    int cobertura_total;
};

struct Problema {
    int o, i, a; // Número de orders, items, aisles
    std::vector<Pedido> pedidos;
    std::vector<Corredor> corredores;
    std::unordered_map<int, std::vector<int>> item_para_corredores; // Mapeia cada item para os corredores que o contém
    std::unordered_map<int, std::unordered_map<int, int>> item_quantidade_corredores; // Mapeia item para corredor e sua quantidade
    std::vector<std::vector<int>> pedido_itens_unicos; // Lista de itens únicos para cada pedido
    std::vector<std::vector<bool>> matriz_cobertura; // Matriz que indica se um corredor cobre um item de um pedido
    int LB, UB; // Lower Bound e Upper Bound da solução

    // Novas estruturas para priorização
    std::vector<PrioridadeProduto> produtos_priorizados;
    std::vector<PrioridadeCorredor> corredores_priorizados;
};

Problema parseEntrada(const std::string& caminho_entrada);