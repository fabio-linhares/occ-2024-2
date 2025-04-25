#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include <vector>
#include <utility> // Para std::pair

struct Warehouse {
    int numOrders;
    int numItems;
    int numCorridors;
    int LB;
    int UB;
    std::vector<std::vector<std::pair<int, int>>> orders;     // Lista de pedidos
    std::vector<std::vector<std::pair<int, int>>> corridors;  // Lista de corredores
};

#endif