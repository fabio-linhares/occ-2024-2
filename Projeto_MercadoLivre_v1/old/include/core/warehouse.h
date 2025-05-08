#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include <vector>
#include <map> // Para std::map

struct Warehouse {
    int numOrders;
    int numItems;
    int numCorridors;
    int LB;
    int UB;
    std::vector<std::map<int, int>> orders;     // Lista de pedidos
    std::vector<std::map<int, int>> corridors;  // Lista de corredores
};

#endif