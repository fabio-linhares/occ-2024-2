#ifndef SPO_SOLVER_H
#define SPO_SOLVER_H

#include <vector>
#include <map>
#include "config_reader.h"

struct Item {
    int id;
    int quantity;
};

struct Order {
    int id;
    std::vector<Item> items;
};

struct Aisle {
    int id;
    std::map<int, int> itemQuantities;
};

struct Instance {
    std::vector<Order> orders;
    std::vector<Aisle> aisles;
    int numPedidos;
    int numItens;
    int numCorredores;
    int LB;  // Limite inferior da wave
    int UB;  // Limite superior da wave
};

struct Solution {
    std::vector<int> selectedOrders;
    std::vector<int> visitedAisles;
};

// Protótipos das funções
Instance readInstance(const std::string& path);
Solution solveSPO(const Instance& instance, const Config& config);
void writeSolution(const Solution& solution, const std::string& path);
bool validateSolution(const Instance& instance, const Solution& solution, const Config& config);

// Adicione a declaração da função:

// Resolve o problema SPO usando o método de Dinkelbach
Solution solveSPO(const Instance& instance, const Config& config);

// Funções auxiliares (declaradas aqui, implementadas no .cpp)
Solution generateInitialSolution(const Instance& instance, const Config& config);  // Corrigido para incluir Config
double calculateRatio(const Solution& solution, const Instance& instance);
double calculateNumerator(const Solution& solution, const Instance& instance);
double calculateDenominator(const Solution& solution, const Instance& instance);

// Adicione esta declaração junto às demais

// Escreve a solução em um arquivo
void writeSolution(const Solution& solution, const std::string& outputPath);

// Se já existir a versão com três parâmetros, mantenha-a também:
void writeSolution(const Solution& solution, const Instance& instance, const std::string& outputPath);

// Adicione estas declarações ao arquivo de cabeçalho

// Função para ler instância do problema
Instance readInstance(const std::string& filename);

// Função para validar uma solução (já declarada acima)

#endif // SPO_SOLVER_H