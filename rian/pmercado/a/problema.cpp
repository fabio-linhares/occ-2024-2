#include "problema.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <unordered_map>
#include <map>
#include <set>

// Função para calcular prioridades de produtos e corredores
void calcularPrioridades(Problema& problema) {
    // 1. Priorização de produtos
    std::unordered_map<int, int> demanda_total;
    
    // Calcular demanda total por produto em todos os pedidos
    for (const auto& pedido : problema.pedidos) {
        for (const auto& [item_id, quantidade] : pedido.itens) {
            demanda_total[item_id] += quantidade;
        }
    }
    
    // Criar objetos de prioridade para cada produto
    for (int i = 0; i < problema.i; ++i) {
        PrioridadeProduto pp;
        pp.id = i;
        pp.demanda_total = demanda_total[i];
        pp.num_corredores_disponivel = problema.item_para_corredores[i].size();
        
        // Prioridade: demanda total / número de corredores que possuem o item
        pp.valor_prioridade = pp.num_corredores_disponivel > 0 ?
                             static_cast<double>(pp.demanda_total) / pp.num_corredores_disponivel : 0;
        
        problema.produtos_priorizados.push_back(pp);
    }
    
    // Ordenar produtos por prioridade (decrescente)
    std::sort(problema.produtos_priorizados.begin(), problema.produtos_priorizados.end(),
             [](const PrioridadeProduto& a, const PrioridadeProduto& b) {
                 return a.valor_prioridade > b.valor_prioridade;
             });
    
    // 2. Priorização de corredores
    std::unordered_map<int, std::vector<int>> produtos_por_corredor;
    std::map<int, std::set<int>> produtos_exclusivos;
    
    // Identificar produtos exclusivos por corredor
    for (int i = 0; i < problema.i; ++i) {
        if (problema.item_para_corredores[i].size() == 1) {
            int corredor_id = problema.item_para_corredores[i][0];
            produtos_exclusivos[corredor_id].insert(i);
        }
        
        for (int corredor_id : problema.item_para_corredores[i]) {
            produtos_por_corredor[corredor_id].push_back(i);
        }
    }
    
    // Criar objetos de prioridade para cada corredor
    for (int k = 0; k < problema.a; ++k) {
        PrioridadeCorredor pc;
        pc.id = k;
        pc.cobertura_total = produtos_por_corredor[k].size();
        
        // Adicionar produtos exclusivos
        if (produtos_exclusivos.count(k)) {
            pc.produtos_exclusivos.assign(
                produtos_exclusivos[k].begin(),
                produtos_exclusivos[k].end()
            );
        }
        
        // Prioridade: Somatório da demanda dos produtos no corredor + (2 * número de produtos exclusivos)
        double somatorio_demanda = 0;
        for (int item_id : produtos_por_corredor[k]) {
            somatorio_demanda += demanda_total[item_id];
        }
        
        pc.valor_prioridade = somatorio_demanda + (pc.produtos_exclusivos.size() * 2.0);
        problema.corredores_priorizados.push_back(pc);
    }
    
    // Ordenar corredores por prioridade (decrescente)
    std::sort(problema.corredores_priorizados.begin(), problema.corredores_priorizados.end(),
             [](const PrioridadeCorredor& a, const PrioridadeCorredor& b) {
                 return a.valor_prioridade > b.valor_prioridade;
             });
}

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
    
    // Leitura dos limites LB e UB (último registro do arquivo)
    std::string linha_limites;
    while (std::getline(arquivo, linha_limites)) {
        if (!linha_limites.empty()) {
            std::istringstream iss(linha_limites);
            iss >> problema.LB >> problema.UB;
            break;
        }
    }
    
    // Após inicializar todas as estruturas do problema, calcular prioridades
    calcularPrioridades(problema);
    
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