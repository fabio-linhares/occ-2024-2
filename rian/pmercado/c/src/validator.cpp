#include "../include/validator.h"
#include "../include/constraints.h"
#include "../include/objective_function.h"
#include <fstream>
#include <iostream>

bool validarSolucao(const Solution& solucao, const Instance& instancia) {
    return verificarTodasRestricoes(solucao.pedidos_selecionados, 
                                   solucao.corredores_visitados, 
                                   instancia);
}

Solution lerSolucao(const std::string& filepath, const Instance& instancia) {
    Solution solucao;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filepath << std::endl;
        return solucao;
    }
    
    int num_pedidos;
    file >> num_pedidos;
    
    for (int i = 0; i < num_pedidos; i++) {
        int pedido_id;
        file >> pedido_id;
        solucao.pedidos_selecionados.push_back(pedido_id);
    }
    
    int num_corredores;
    file >> num_corredores;
    
    for (int i = 0; i < num_corredores; i++) {
        int corredor_id;
        file >> corredor_id;
        solucao.corredores_visitados.push_back(corredor_id);
    }
    
    // Calcular o valor objetivo
    solucao.valor_objetivo = calcularRazao(solucao.pedidos_selecionados, 
                                          solucao.corredores_visitados, 
                                          instancia);
    
    return solucao;
}

void escreverSolucao(const std::string& filepath, const Solution& solucao) {
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo para escrita: " << filepath << std::endl;
        return;
    }
    
    file << solucao.pedidos_selecionados.size() << std::endl;
    
    for (size_t i = 0; i < solucao.pedidos_selecionados.size(); i++) {
        file << solucao.pedidos_selecionados[i] << std::endl;
    }
    
    file << solucao.corredores_visitados.size() << std::endl;
    
    for (size_t i = 0; i < solucao.corredores_visitados.size(); i++) {
        file << solucao.corredores_visitados[i] << std::endl;
    }
}
