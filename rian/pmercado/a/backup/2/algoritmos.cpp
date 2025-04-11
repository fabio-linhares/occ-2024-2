#include "algoritmos.h"
#include <iostream> // Para debugging
#include <algorithm>
#include <random>
#include <set>
#include <cmath>

// Gera a próxima combinação lexicográfica de k elementos escolhidos de {0, 1, ..., n-1}
static bool next_combination(std::vector<int>& indices, int n) {
    int k = indices.size();
    
    // Encontrar o índice mais à direita que pode ser incrementado
    for (int i = k - 1; i >= 0; --i) {
        if (indices[i] < n - k + i) {
            // Incrementar este índice
            ++indices[i];
            
            // Reajustar todos os índices à direita para valores consecutivos
            for (int j = i + 1; j < k; ++j) {
                indices[j] = indices[i] + (j - i);
            }
            
            return true; // Nova combinação encontrada
        }
    }
    
    return false; // Não há mais combinações
}

// Calcula o benefício (itens/corredores) para um conjunto de pedidos
double calcularBeneficio(const std::vector<int>& pedidos_selecionados, const std::set<int>& corredores_necessarios, const Problema& problema) {
    int total_itens = 0;
    for (int pedido_id : pedidos_selecionados) {
        for (const auto& item_info : problema.pedidos[pedido_id].itens) {
            total_itens += item_info.second; // Quantidade do item
        }
    }
    
    return (corredores_necessarios.empty()) ? 0 : 
           static_cast<double>(total_itens) / corredores_necessarios.size();
}

// Verifica se o total de itens está dentro dos limites permitidos
bool respeitaLimites(int total_itens, int L_min, int L_max) {
    return (total_itens >= L_min && total_itens <= L_max);
}

// Calcula os corredores necessários e o total de itens para um conjunto de pedidos
std::pair<std::set<int>, int> calcularCorredoresEItens(const std::vector<int>& pedidos, const Problema& problema) {
    std::set<int> corredores_necessarios;
    int total_itens = 0;
    
    for (int pedido_id : pedidos) {
        for (const auto& item_info : problema.pedidos[pedido_id].itens) {
            int item_id = item_info.first;
            int quantidade = item_info.second;
            
            total_itens += quantidade;
            
            // Se o item existe em algum corredor
            if (problema.item_para_corredores.find(item_id) != problema.item_para_corredores.end() &&
                !problema.item_para_corredores.at(item_id).empty()) {
                // Adicionar o primeiro corredor que tem o item
                corredores_necessarios.insert(problema.item_para_corredores.at(item_id)[0]);
            }
        }
    }
    
    return {corredores_necessarios, total_itens};
}

Solucao calcularWave(const Problema& problema, const std::vector<int>& indices_pedidos, bool aplicar_busca_local) {
    Solucao solucao;
    
    // Verifique se há pedidos para processar
    if (indices_pedidos.empty()) {
        solucao.custo_total = 0.0;
        return solucao;
    }
    
    // Calcule corredores necessários e total de itens
    std::set<int> corredores_necessarios;
    int total_itens = 0;
    
    // Mapeamento eficiente: item_id -> quantidade necessária total
    std::unordered_map<int, int> demanda_por_item;
    
    // 1. Calcular demanda total por item em todos os pedidos
    for (int pedido_id : indices_pedidos) {
        for (const auto& [item_id, quantidade] : problema.pedidos[pedido_id].itens) {
            demanda_por_item[item_id] += quantidade;
            total_itens += quantidade;
        }
    }
    
    // 2. Determinar conjunto mínimo de corredores necessários
    std::unordered_map<int, std::vector<std::pair<int, int>>> corredor_para_itens; // corredor -> [(item, capacidade)]
    
    // Pré-processamento para identificar quais itens cada corredor possui
    for (const auto& [item_id, _] : demanda_por_item) {
        if (problema.item_para_corredores.find(item_id) != problema.item_para_corredores.end()) {
            for (int corredor_id : problema.item_para_corredores.at(item_id)) {
                if (problema.item_quantidade_corredores.count(item_id) && 
                    problema.item_quantidade_corredores.at(item_id).count(corredor_id)) {
                    int capacidade = problema.item_quantidade_corredores.at(item_id).at(corredor_id);
                    corredor_para_itens[corredor_id].push_back({item_id, capacidade});
                }
            }
        }
    }
    
    // Algoritmo guloso para selecionar corredores (priorizando os que cobrem mais itens)
    while (!demanda_por_item.empty()) {
        int melhor_corredor = -1;
        int max_itens_cobertos = 0;
        
        for (const auto& [corredor_id, itens] : corredor_para_itens) {
            int itens_cobertos = 0;
            for (const auto& [item_id, _] : itens) {
                if (demanda_por_item.count(item_id)) {
                    itens_cobertos++;
                }
            }
            
            if (itens_cobertos > max_itens_cobertos) {
                max_itens_cobertos = itens_cobertos;
                melhor_corredor = corredor_id;
            }
        }
        
        // Se não encontrou corredor que cubra itens pendentes, é porque alguns itens não têm cobertura
        if (melhor_corredor == -1) break;
        
        // Adicionar o melhor corredor à solução
        corredores_necessarios.insert(melhor_corredor);
        
        // Atualizar demanda restante
        for (const auto& [item_id, capacidade] : corredor_para_itens[melhor_corredor]) {
            if (demanda_por_item.count(item_id)) {
                // Removemos o item da demanda se for atendido completamente
                demanda_por_item.erase(item_id);
            }
        }
    }
    
    // Verificar se todos os itens foram cobertos
    if (!demanda_por_item.empty()) {
        // Não foi possível atender todos os itens
        solucao.custo_total = 0.0;
        return solucao;
    }
    
    // 3. Montar a solução
    solucao.pedidos_atendidos = indices_pedidos;
    solucao.corredores_utilizados.assign(corredores_necessarios.begin(), corredores_necessarios.end());
    solucao.custo_total = static_cast<double>(total_itens) / corredores_necessarios.size();
    
    // 4. Aplicar busca local se solicitado
    if (aplicar_busca_local) {
        busca_local_otimizada(problema, solucao);
    }
    
    return solucao;
}

// Nova função para busca local otimizada
void busca_local_otimizada(const Problema& problema, Solucao& solucao) {
    bool melhorou = true;
    const int L_min = 10;  // Ajustar conforme necessidade
    const int L_max = 1000; // Ajustar conforme necessidade
    
    // Cache de corredores por item para acesso rápido
    std::unordered_map<int, std::set<int>> cache_corredores_por_item;
    
    // Preencher cache
    for (int i = 0; i < problema.i; ++i) {
        if (problema.item_para_corredores.find(i) != problema.item_para_corredores.end()) {
            for (int corredor : problema.item_para_corredores.at(i)) {
                cache_corredores_por_item[i].insert(corredor);
            }
        }
    }
    
    // Conjunto de pedidos já na solução para busca rápida
    std::set<int> pedidos_na_solucao(solucao.pedidos_atendidos.begin(), solucao.pedidos_atendidos.end());
    
    while (melhorou) {
        melhorou = false;
        
        // 1. Movimento: Tentar trocar pedidos
        for (size_t i = 0; i < solucao.pedidos_atendidos.size(); ++i) {
            int pedido_atual = solucao.pedidos_atendidos[i];
            
            // Para cada pedido não incluído, tentar trocar
            for (int pedido_candidato = 0; pedido_candidato < problema.o; ++pedido_candidato) {
                if (pedidos_na_solucao.count(pedido_candidato)) continue;
                
                // Criar solução temporária com a troca
                std::vector<int> novos_pedidos = solucao.pedidos_atendidos;
                novos_pedidos[i] = pedido_candidato;
                
                // Calcular nova solução
                auto nova_solucao = calcularWave(problema, novos_pedidos, false);
                
                // Verificar se é melhor e respeita limites
                int total_itens_nova = 0;
                for (int p : nova_solucao.pedidos_atendidos) {
                    for (const auto& [_, qtd] : problema.pedidos[p].itens) {
                        total_itens_nova += qtd;
                    }
                }
                
                if (nova_solucao.custo_total > solucao.custo_total && 
                    total_itens_nova >= L_min && total_itens_nova <= L_max) {
                    solucao = nova_solucao;
                    pedidos_na_solucao.erase(pedido_atual);
                    pedidos_na_solucao.insert(pedido_candidato);
                    melhorou = true;
                    break;
                }
            }
            
            if (melhorou) break;
        }
        
        if (melhorou) continue;
        
        // 2. Movimento: Tentar adicionar pedidos que aumentam eficiência
        for (int pedido_candidato = 0; pedido_candidato < problema.o; ++pedido_candidato) {
            if (pedidos_na_solucao.count(pedido_candidato)) continue;
            
            // Criar solução temporária com adição
            std::vector<int> novos_pedidos = solucao.pedidos_atendidos;
            novos_pedidos.push_back(pedido_candidato);
            
            // Calcular nova solução
            auto nova_solucao = calcularWave(problema, novos_pedidos, false);
            
            // Verificar se é melhor e respeita limites
            int total_itens_nova = 0;
            for (int p : nova_solucao.pedidos_atendidos) {
                for (const auto& [_, qtd] : problema.pedidos[p].itens) {
                    total_itens_nova += qtd;
                }
            }
            
            if (nova_solucao.custo_total > solucao.custo_total && 
                total_itens_nova >= L_min && total_itens_nova <= L_max) {
                solucao = nova_solucao;
                pedidos_na_solucao.insert(pedido_candidato);
                melhorou = true;
                break;
            }
        }
        
        if (melhorou) continue;
        
        // 3. Movimento: Tentar remover pedidos que diminuem eficiência
        if (solucao.pedidos_atendidos.size() > 1) {  // Garantir que há pelo menos 1 pedido após remoção
            for (size_t i = 0; i < solucao.pedidos_atendidos.size(); ++i) {
                int pedido_atual = solucao.pedidos_atendidos[i];
                
                // Criar solução temporária sem este pedido
                std::vector<int> novos_pedidos;
                for (size_t j = 0; j < solucao.pedidos_atendidos.size(); ++j) {
                    if (j != i) novos_pedidos.push_back(solucao.pedidos_atendidos[j]);
                }
                
                // Calcular nova solução
                auto nova_solucao = calcularWave(problema, novos_pedidos, false);
                
                // Verificar se é melhor e respeita limites
                int total_itens_nova = 0;
                for (int p : nova_solucao.pedidos_atendidos) {
                    for (const auto& [_, qtd] : problema.pedidos[p].itens) {
                        total_itens_nova += qtd;
                    }
                }
                
                if (nova_solucao.custo_total > solucao.custo_total && 
                    total_itens_nova >= L_min && total_itens_nova <= L_max) {
                    solucao = nova_solucao;
                    pedidos_na_solucao.erase(pedido_atual);
                    melhorou = true;
                    break;
                }
            }
        }
    }
}

Solucao resolverProblemaAdaptativo(const Problema& problema) {
    // Calcular métricas da instância para decisão adaptativa
    MetricasInstancia metricas = calcularMetricas(problema);
    
    // Decidir qual algoritmo usar com base nas métricas
    if (problema.o <= 4) { // Para instâncias muito pequenas
        return buscaExaustiva(problema);
    } else if (metricas.densidade_matriz_cobertura < 0.3) {
        // Para instâncias esparsas
        Parametros params;
        calibrarAlgoritmo(params);
        return aplicarGRASPComRankings(problema, params, metricas);
    } else {
        // Para instâncias mais densas
        return algoritmoGulosoMelhorado(problema, metricas);
    }
}

bool calibrarAlgoritmo(Parametros& parametros) {
    // Definindo valores padrão para os parâmetros
    parametros.intensidade_perturbacao = 0.3;
    parametros.max_iteracoes_perturbacao = 100;
    parametros.intensidade_grasp = 0.2;
    parametros.max_iteracoes_grasp = 50;
    parametros.usar_perturbacao_agressiva = false;
    parametros.usar_modelo_exato_4pedidos = true;
    
    std::cout << "Algoritmo calibrado com valores padrão." << std::endl;
    return true;
}

Solucao algoritmoGulosoMelhorado(const Problema& problema, const MetricasInstancia& metricas) {
    // Definir limites de capacidade (deve ser parametrizado adequadamente)
    const int L_min = 10;  // Exemplo - ajustar conforme necessidade
    const int L_max = 1000; // Exemplo - ajustar conforme necessidade
    
    Solucao solucao;
    solucao.custo_total = 0.0;
    
    // Conjunto para rastrear os corredores escolhidos
    std::set<int> corredores_escolhidos;
    int total_itens_selecionados = 0;
    
    // Calcular o benefício inicial para cada pedido (itens/corredores)
    std::vector<std::pair<double, int>> beneficios_pedidos; // (benefício, pedido_id)
    
    for (int p = 0; p < problema.o; ++p) {
        std::set<int> corredores_necessarios;
        int itens_pedido = 0;
        
        for (const auto& item_info : problema.pedidos[p].itens) {
            int item_id = item_info.first;
            int quantidade = item_info.second;
            
            itens_pedido += quantidade;
            
            // Se o item existe em algum corredor
            if (problema.item_para_corredores.find(item_id) != problema.item_para_corredores.end() &&
                !problema.item_para_corredores.at(item_id).empty()) {
                // Adicionar o primeiro corredor que tem o item
                corredores_necessarios.insert(problema.item_para_corredores.at(item_id)[0]);
            }
        }
        
        // Calcular benefício
        double beneficio = corredores_necessarios.empty() ? 0 :
                         static_cast<double>(itens_pedido) / corredores_necessarios.size();
        
        beneficios_pedidos.push_back({beneficio, p});
    }
    
    // Ordenar os pedidos pelo maior benefício
    std::sort(beneficios_pedidos.begin(), beneficios_pedidos.end(), std::greater<>());
    
    // Selecionar pedidos enquanto respeitar o limite máximo
    for (const auto& [beneficio, pedido_id] : beneficios_pedidos) {
        // Verificar quantidade de itens deste pedido
        int itens_pedido = 0;
        for (const auto& item_info : problema.pedidos[pedido_id].itens) {
            itens_pedido += item_info.second;
        }
        
        // Se adicionar exceder o limite, pular
        if (total_itens_selecionados + itens_pedido > L_max) {
            continue;
        }
        
        // Adicionar pedido
        solucao.pedidos_atendidos.push_back(pedido_id);
        total_itens_selecionados += itens_pedido;
        
        // Adicionar corredores necessários
        for (const auto& item_info : problema.pedidos[pedido_id].itens) {
            int item_id = item_info.first;
            
            // Se o item existe em algum corredor
            if (problema.item_para_corredores.find(item_id) != problema.item_para_corredores.end() &&
                !problema.item_para_corredores.at(item_id).empty()) {
                // Adicionar o primeiro corredor que tem o item
                corredores_escolhidos.insert(problema.item_para_corredores.at(item_id)[0]);
            }
        }
    }
    
    // Verificar limite mínimo
    if (total_itens_selecionados < L_min) {
        // Não conseguimos atender ao limite mínimo, retornar solução vazia
        solucao.pedidos_atendidos.clear();
        solucao.corredores_utilizados.clear();
        solucao.custo_total = 0.0;
        return solucao;
    }
    
    // Preencher corredores utilizados
    solucao.corredores_utilizados.assign(corredores_escolhidos.begin(), corredores_escolhidos.end());
    
    // Calcular benefício final
    solucao.custo_total = static_cast<double>(total_itens_selecionados) / corredores_escolhidos.size();
    
    return solucao;
}

void melhorar_solucao_local_com_metricas(const Problema& problema, Solucao& solucao, const MetricasInstancia& metricas) {
    // Estrutura para rastrear os corredores necessários para cada pedido
    std::vector<std::set<int>> corredores_por_pedido(problema.o);
    
    // Para cada pedido atendido, identificar os corredores necessários
    for (int pedido_id : solucao.pedidos_atendidos) {
        for (const auto& item_info : problema.pedidos[pedido_id].itens) {
            int item_id = item_info.first;
            
            // Adicionar corredores que têm o item
            for (int corredor_id : problema.item_para_corredores.at(item_id)) {
                corredores_por_pedido[pedido_id].insert(corredor_id);
            }
        }
    }
    
    // Tentar remover corredores redundantes
    bool melhorado = true;
    while (melhorado) {
        melhorado = false;
        
        // Para cada corredor, verificar se ele pode ser removido
        std::set<int> todos_corredores;
        for (const auto& corredores : corredores_por_pedido) {
            for (int c : corredores) {
                todos_corredores.insert(c);
            }
        }
        
        for (int corredor : todos_corredores) {
            // Verificar se o corredor é necessário para algum pedido
            bool necessario = false;
            
            for (int pedido_id : solucao.pedidos_atendidos) {
                if (corredores_por_pedido[pedido_id].count(corredor) > 0) {
                    // Verificar se é o único corredor para algum item deste pedido
                    for (const auto& item_info : problema.pedidos[pedido_id].itens) {
                        int item_id = item_info.first;
                        
                        // Se o item está neste corredor
                        if (problema.matriz_cobertura[item_id][corredor]) {
                            // Verificar se há outros corredores com este item
                            bool tem_alternativa = false;
                            for (int outro_corredor : problema.item_para_corredores.at(item_id)) {
                                if (outro_corredor != corredor && corredores_por_pedido[pedido_id].count(outro_corredor) > 0) {
                                    tem_alternativa = true;
                                    break;
                                }
                            }
                            
                            if (!tem_alternativa) {
                                necessario = true;
                                break;
                            }
                        }
                    }
                    
                    if (necessario) break;
                }
            }
            
            // Se o corredor não é necessário, removê-lo
            if (!necessario) {
                for (auto& corredores : corredores_por_pedido) {
                    corredores.erase(corredor);
                }
                melhorado = true;
                // Recalcular o custo
                int novo_custo = 0;
                std::set<int> corredores_unicos;
                for (const auto& corredores : corredores_por_pedido) {
                    for (int c : corredores) {
                        corredores_unicos.insert(c);
                    }
                }
                solucao.custo_total = corredores_unicos.size();
                break;
            }
        }
    }
}

Solucao aplicarGRASPComRankings(const Problema& problema, const Parametros& params, const MetricasInstancia& metricas) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Definir limites de capacidade (deve ser parametrizado adequadamente)
    const int L_min = 10;  // Exemplo - ajustar conforme necessidade
    const int L_max = 1000; // Exemplo - ajustar conforme necessidade
    
    Solucao melhor_solucao;
    melhor_solucao.custo_total = 0.0; // Agora é um benefício a ser maximizado
    
    for (int iter = 0; iter < params.max_iteracoes_grasp; ++iter) {
        // Fase de construção
        Solucao solucao_atual;
        
        // Lista de pedidos candidatos (todos inicialmente)
        std::vector<int> candidatos;
        for (int i = 0; i < problema.o; ++i) {
            candidatos.push_back(i);
        }
        
        // Conjunto de pedidos selecionados
        std::vector<int> pedidos_selecionados;
        std::set<int> corredores_selecionados;
        int total_itens_selecionados = 0;
        
        // Enquanto houver candidatos e não exceder o limite máximo
        while (!candidatos.empty() && total_itens_selecionados < L_max) {
            // Calcular o benefício marginal de adicionar cada pedido
            std::vector<std::tuple<double, int, std::set<int>>> rankings; // (benefício, pedido_id, novos_corredores)
            
            for (int pedido_id : candidatos) {
                // Calcular conjunto temporário com pedido atual adicionado
                std::vector<int> pedidos_temp = pedidos_selecionados;
                pedidos_temp.push_back(pedido_id);
                
                // Calcular corredores necessários e total de itens com este pedido adicional
                auto [corredores_temp, itens_temp] = calcularCorredoresEItens(pedidos_temp, problema);
                
                // Se adicionar este pedido excede o limite máximo, pular
                if (itens_temp > L_max) continue;
                
                // Calcular benefício
                double beneficio = static_cast<double>(itens_temp) / corredores_temp.size();
                
                // Guardar os novos corredores necessários
                std::set<int> novos_corredores;
                for (int corredor : corredores_temp) {
                    if (corredores_selecionados.find(corredor) == corredores_selecionados.end()) {
                        novos_corredores.insert(corredor);
                    }
                }
                
                rankings.push_back({beneficio, pedido_id, novos_corredores});
            }
            
            // Se não houver mais candidatos viáveis, sair do loop
            if (rankings.empty()) break;
            
            // Ordenar candidatos pelo benefício (maior para menor)
            std::sort(rankings.begin(), rankings.end(), 
                     [](const auto& a, const auto& b) { return std::get<0>(a) > std::get<0>(b); });
            
            // Lista restrita de candidatos (RCL)
            int rcl_size = std::max(1, static_cast<int>(rankings.size() * params.intensidade_grasp));
            
            // Escolher aleatoriamente um candidato da RCL
            std::uniform_int_distribution<> dis(0, rcl_size - 1);
            int escolhido_idx = dis(gen);
            int pedido_escolhido = std::get<1>(rankings[escolhido_idx]);
            std::set<int> novos_corredores = std::get<2>(rankings[escolhido_idx]);
            
            // Adicionar o pedido à solução
            pedidos_selecionados.push_back(pedido_escolhido);
            
            // Atualizar corredores selecionados
            corredores_selecionados.insert(novos_corredores.begin(), novos_corredores.end());
            
            // Atualizar total de itens
            for (const auto& item_info : problema.pedidos[pedido_escolhido].itens) {
                total_itens_selecionados += item_info.second;
            }
            
            // Remover o pedido escolhido da lista de candidatos
            candidatos.erase(std::remove(candidatos.begin(), candidatos.end(), pedido_escolhido), candidatos.end());
        }
        
        // Verificar se atendemos o limite mínimo de itens
        if (total_itens_selecionados >= L_min) {
            // Calcular benefício final
            double beneficio = static_cast<double>(total_itens_selecionados) / corredores_selecionados.size();
            
            // Preencher a solução
            solucao_atual.pedidos_atendidos = pedidos_selecionados;
            solucao_atual.corredores_utilizados.assign(corredores_selecionados.begin(), corredores_selecionados.end());
            solucao_atual.custo_total = beneficio;
            
            // Atualizar a melhor solução encontrada (agora maximizando)
            if (solucao_atual.custo_total > melhor_solucao.custo_total) {
                melhor_solucao = solucao_atual;
            }
        }
    }
    
    return melhor_solucao;
}

// Substituir a implementação existente de buscaExaustiva

Solucao buscaExaustiva(const Problema& problema) {
    // Só é viável para instâncias muito pequenas
    if (problema.o > 10) {
        std::cout << "Aviso: Busca exaustiva não recomendada para instâncias com mais de 10 pedidos." << std::endl;
    }
    
    // Definir limites de capacidade (deve ser parametrizado adequadamente)
    const int L_min = 10;  // Exemplo - ajustar conforme necessidade
    const int L_max = 1000; // Exemplo - ajustar conforme necessidade
    
    Solucao melhor_solucao;
    melhor_solucao.custo_total = 0.0; // Agora é um benefício a ser maximizado
    
    // Gerar todas as combinações possíveis de pedidos
    for (int k = 1; k <= problema.o; ++k) {
        // Iniciar com os k primeiros pedidos
        std::vector<int> indices(k);
        for (int i = 0; i < k; ++i) {
            indices[i] = i;
        }
        
        do {
            // Avaliar a combinação atual
            auto [corredores_necessarios, total_itens] = calcularCorredoresEItens(indices, problema);
            
            // Verificar restrições de capacidade
            if (respeitaLimites(total_itens, L_min, L_max)) {
                // Calcular benefício
                double beneficio = static_cast<double>(total_itens) / corredores_necessarios.size();
                
                // Se o benefício for melhor que o melhor encontrado até agora
                if (beneficio > melhor_solucao.custo_total) {
                    melhor_solucao.pedidos_atendidos = indices;
                    melhor_solucao.corredores_utilizados.assign(corredores_necessarios.begin(), corredores_necessarios.end());
                    melhor_solucao.custo_total = beneficio;
                }
            }
            
        } while (next_combination(indices, problema.o));
    }
    
    return melhor_solucao;
}

Solucao solucionadorRobusto(const Problema& problema) {
    // Esta função tenta diferentes abordagens e retorna a melhor
    
    MetricasInstancia metricas = calcularMetricas(problema);
    
    // Abordagem 1: Algoritmo guloso melhorado
    Solucao solucao1 = algoritmoGulosoMelhorado(problema, metricas);
    
    // Abordagem 2: GRASP com rankings
    Parametros params;
    calibrarAlgoritmo(params);
    Solucao solucao2 = aplicarGRASPComRankings(problema, params, metricas);
    
    // Se a instância for pequena o suficiente, tentar busca exaustiva
    Solucao solucao3;
    if (problema.o <= 8) {
        solucao3 = buscaExaustiva(problema);
    } else {
        solucao3.custo_total = std::numeric_limits<double>::max();
    }
    
    // Retornar a melhor solução entre as três abordagens
    if (solucao1.custo_total <= solucao2.custo_total && solucao1.custo_total <= solucao3.custo_total) {
        return solucao1;
    } else if (solucao2.custo_total <= solucao1.custo_total && solucao2.custo_total <= solucao3.custo_total) {
        return solucao2;
    } else {
        return solucao3;
    }
}