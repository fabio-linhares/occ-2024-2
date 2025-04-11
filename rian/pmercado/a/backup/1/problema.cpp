#include "problema.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cassert>

Problema parseEntrada(const std::string& caminho_entrada) {
    Problema problema;
    std::ifstream arquivo(caminho_entrada);
    
    if (!arquivo.is_open()) {
        throw std::runtime_error("Erro ao abrir o arquivo: " + caminho_entrada);
    }
    
    // Leitura do cabeçalho: número de pedidos (o), itens (i) e corredores (a)
    arquivo >> problema.o >> problema.i >> problema.a;
    
    // Inicialização das estruturas
    problema.pedidos.resize(problema.o);
    problema.corredores.resize(problema.a);
    problema.matriz_cobertura.resize(problema.i, std::vector<bool>(problema.a, false));
    
    // Leitura dos pedidos
    for (int o = 0; o < problema.o; ++o) {
        problema.pedidos[o].index = o;
        int num_itens_pedido;
        arquivo >> num_itens_pedido;
        problema.pedidos[o].total_itens = 0;
        
        for (int j = 0; j < num_itens_pedido; ++j) {
            int item_id, quantidade;
            arquivo >> item_id >> quantidade;
            problema.pedidos[o].itens.push_back({item_id, quantidade});
            problema.pedidos[o].total_itens += quantidade;
        }
    }
    
    // Leitura dos corredores
    for (int a = 0; a < problema.a; ++a) {
        problema.corredores[a].index = a;
        int num_itens_corredor;
        arquivo >> num_itens_corredor;
        
        for (int j = 0; j < num_itens_corredor; ++j) {
            int item_id, quantidade;
            arquivo >> item_id >> quantidade;
            problema.corredores[a].estoque.push_back({item_id, quantidade});
            
            // Atualizando o mapeamento de itens para corredores
            problema.item_para_corredores[item_id].push_back(a);
            problema.item_quantidade_corredores[item_id][a] = quantidade;
            
            // Atualizando a matriz de cobertura
            problema.matriz_cobertura[item_id][a] = true;
        }
    }
    
    // Processando informações adicionais
    problema.pedido_itens_unicos.resize(problema.o);
    for (int o = 0; o < problema.o; ++o) {
        std::set<int> itens_unicos;
        for (const auto& [item_id, _] : problema.pedidos[o].itens) {
            itens_unicos.insert(item_id);
        }
        problema.pedido_itens_unicos[o] = std::vector<int>(itens_unicos.begin(), itens_unicos.end());
    }
    
    // Definindo LB e UB (exemplo simples)
    problema.LB = 1;
    problema.UB = problema.a;
    
    arquivo.close();
    return problema;
}

// Função para verificar a integridade do problema
bool verificarIntegridadeProblema(const Problema& problema) {
    // Verificar se todos os itens dos pedidos existem em pelo menos um corredor
    for (const auto& pedido : problema.pedidos) {
        for (const auto& [item_id, _] : pedido.itens) {
            if (problema.item_para_corredores.find(item_id) == problema.item_para_corredores.end()) {
                std::cerr << "Erro: Item " << item_id << " do pedido " << pedido.index
                          << " não está em nenhum corredor." << std::endl;
                return false;
            }
        }
    }

    // Verificar se a matriz de cobertura está consistente com o mapeamento de itens para corredores
    for (int i = 0; i < problema.i; ++i) {
        for (int j = 0; j < problema.a; ++j) {
            auto it = problema.item_para_corredores.find(i);
            bool deveria_cobrir = (it != problema.item_para_corredores.end()) &&
                                  (std::find(it->second.begin(), it->second.end(), j) != it->second.end());
            if (problema.matriz_cobertura[i][j] != deveria_cobrir) {
                std::cerr << "Erro: Inconsistência na matriz de cobertura para item "
                          << i << " e corredor " << j << std::endl;
                return false;
            }
        }
    }

    // Verificar se a quantidade de itens em cada corredor é não-negativa
    for (const auto& corredor : problema.corredores) {
        for (const auto& [item_id, quantidade] : corredor.estoque) {
            if (quantidade < 0) {
                std::cerr << "Erro: Quantidade negativa do item " << item_id << " no corredor " << corredor.index << std::endl;
                return false;
            }
        }
    }

    // Verificar se a quantidade total de itens em um pedido é não-negativa
    for (const auto& pedido : problema.pedidos) {
        if (pedido.total_itens < 0) {
            std::cerr << "Erro: Quantidade total negativa no pedido " << pedido.index << std::endl;
            return false;
        }
    }

    return true;
}