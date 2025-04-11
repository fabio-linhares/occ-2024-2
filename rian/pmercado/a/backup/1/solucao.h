#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

// Definição da estrutura Solucao
struct Solucao {
    double custo_total;  // Agora representa o benefício (itens/corredores)
    std::vector<int> pedidos_atendidos;
    std::vector<int> corredores_utilizados;  // Novo campo para armazenar os corredores
};

// Definição da estrutura ResultadoInstancia (usada em main.cpp)
struct ResultadoInstancia {
    std::string nome_instancia;
    Solucao solucao;
    long long tempo_execucao_ms;
};

// Declaração da função gerarSaida
void gerarSaida(const std::string& caminho_saida, const Solucao& solucao);