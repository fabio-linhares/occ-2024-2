#include "algoritmos.h"
#include <iostream> // Para debugging
#include <algorithm>
#include <random>
#include <set>
#include <cmath>
#include <limits.h> // Para INT_MAX

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
        
        // 1. Movimento: Tentar trocar pedidos usando lista de prioridades
        for (size_t i = 0; i < solucao.pedidos_atendidos.size(); ++i) {
            int pedido_atual = solucao.pedidos_atendidos[i];
            
            // Criar lista de candidatos priorizados para substituição
            std::vector<int> candidatos_priorizados;
            
            // Priorizar pedidos com produtos de alta prioridade
            for (int pedido_id = 0; pedido_id < problema.o; ++pedido_id) {
                if (pedidos_na_solucao.count(pedido_id)) continue;
                
                // Calcular valor de prioridade para este pedido
                double valor_prioridade = 0;
                for (const auto& [item_id, quantidade] : problema.pedidos[pedido_id].itens) {
                    // Encontrar o índice do item na lista priorizada
                    auto it = std::find_if(problema.produtos_priorizados.begin(), 
                                          problema.produtos_priorizados.end(),
                                          [item_id](const PrioridadeProduto& pp) { 
                                              return pp.id == item_id; 
                                          });
                    
                    if (it != problema.produtos_priorizados.end()) {
                        valor_prioridade += it->valor_prioridade * quantidade;
                    }
                }
                
                candidatos_priorizados.push_back(pedido_id);
            }
            
            // Ordenar candidatos por prioridade
            std::sort(candidatos_priorizados.begin(), candidatos_priorizados.end(),
                     [&problema](int a, int b) {
                         double valor_a = 0, valor_b = 0;
                         
                         for (const auto& [item_id, qtd] : problema.pedidos[a].itens) {
                             auto it = std::find_if(problema.produtos_priorizados.begin(), 
                                                  problema.produtos_priorizados.end(),
                                                  [item_id](const PrioridadeProduto& pp) { 
                                                      return pp.id == item_id; 
                                                  });
                             if (it != problema.produtos_priorizados.end()) {
                                 valor_a += it->valor_prioridade * qtd;
                             }
                         }
                         
                         for (const auto& [item_id, qtd] : problema.pedidos[b].itens) {
                             auto it = std::find_if(problema.produtos_priorizados.begin(), 
                                                  problema.produtos_priorizados.end(),
                                                  [item_id](const PrioridadeProduto& pp) { 
                                                      return pp.id == item_id; 
                                                  });
                             if (it != problema.produtos_priorizados.end()) {
                                 valor_b += it->valor_prioridade * qtd;
                             }
                         }
                         
                         return valor_a > valor_b;
                     });
            
            // Testar troca com candidatos priorizados (limite para os top 20)
            int max_candidatos = std::min(20, static_cast<int>(candidatos_priorizados.size()));
            for (int j = 0; j < max_candidatos; ++j) {
                int pedido_candidato = candidatos_priorizados[j];
                
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

// Implementar a nova função após busca_local_otimizada
void busca_local_intensiva(const Problema& problema, Solucao& solucao) {
    // Implementação conforme sugerido
}

// Implementar funções auxiliares para os diferentes tipos de movimentos
bool aplicarMovimentoSubstituicao(const Problema& problema, Solucao& solucao, const std::vector<std::set<int>>& corredores_por_pedido);
bool aplicarMovimentoAdicao(const Problema& problema, Solucao& solucao, const std::vector<std::set<int>>& corredores_por_pedido);
bool aplicarMovimentoRemocao(const Problema& problema, Solucao& solucao, const std::vector<std::set<int>>& corredores_por_pedido);
bool aplicarMovimentoTrocaGrupos(const Problema& problema, Solucao& solucao, const std::vector<std::set<int>>& corredores_por_pedido);
void removerCorredoresRedundantes(const Problema& problema, Solucao& solucao);

Solucao resolverProblemaAdaptativo(const Problema& problema) {
    MetricasInstancia metricas = calcularMetricas(problema);
    Parametros params;
    if (!calibrarAlgoritmo(params)) {
        // Usar parâmetros padrão
        params.intensidade_perturbacao = 0.3;
        params.max_iteracoes_grasp = 50;
    }
    
    // Estratégia adaptativa baseada em múltiplos critérios
    if (problema.o <= 8) {
        return buscaExaustiva(problema);
    } else if (problema.o <= 25 && metricas.densidade_matriz_cobertura > 0.2) {
        // Usar Dinkelbach para instâncias de tamanho médio com densidade suficiente
        return aplicarDinkelbach(problema, params, metricas);
    } else if (metricas.densidade_matriz_cobertura < 0.1) {
        // Para instâncias esparsas, melhor usar algoritmo guloso melhorado
        return algoritmoGulosoMelhorado(problema, metricas);
    } else {
        // Para os demais casos, usar ILS
        return aplicarILS(problema, params, metricas);
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
                
                // Adicionar bônus baseado na prioridade dos produtos deste pedido
                double bonus_prioridade = 0;
                for (const auto& [item_id, qtd] : problema.pedidos[pedido_id].itens) {
                    auto it = std::find_if(problema.produtos_priorizados.begin(), 
                                          problema.produtos_priorizados.end(),
                                          [item_id](const PrioridadeProduto& pp) { 
                                              return pp.id == item_id; 
                                          });
                    if (it != problema.produtos_priorizados.end()) {
                        bonus_prioridade += it->valor_prioridade * 0.1; // Fator de ajuste
                    }
                }
                
                // Ajustar o benefício com o bônus
                double beneficio_ajustado = beneficio + bonus_prioridade;
                
                rankings.push_back({beneficio_ajustado, pedido_id, novos_corredores});
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

Solucao aplicarILS(const Problema& problema, const Parametros& params, const MetricasInstancia& metricas) {
    // Parâmetros do ILS - podem ser ajustados ou incluídos na estrutura Parametros
    const int max_iteracoes = params.max_iteracoes_grasp; // Reutilizamos o mesmo parâmetro
    const double intensidade_perturbacao = params.intensidade_perturbacao;
    const int sem_melhoria_max = 20; // Critério de parada adicional
    
    // Valores de referência para o problema
    const int L_min = std::max(1, static_cast<int>(problema.pedidos.size() * 0.1)); // Mínimo de itens (ajustável)
    const int L_max = problema.UB; // Máximo de itens
    
    // Solução inicial usando algoritmo guloso determinístico
    Solucao solucao_atual = construirSolucaoInicial(problema, L_min, L_max);
    
    // Aplicar busca local na solução inicial
    busca_local_otimizada(problema, solucao_atual);
    
    // Guardar melhor solução
    Solucao melhor_solucao = solucao_atual;
    
    // Contador de iterações sem melhoria
    int iteracoes_sem_melhoria = 0;
    
    // Loop principal do ILS
    for (int iter = 0; iter < max_iteracoes && iteracoes_sem_melhoria < sem_melhoria_max; ++iter) {
        // Perturbar a solução atual
        Solucao solucao_perturbada = perturbarSolucao(solucao_atual, problema, 
                                                     intensidade_perturbacao, L_min, L_max);
        
        // Aplicar busca local na solução perturbada
        busca_local_otimizada(problema, solucao_perturbada);
        
        // Critério de aceitação
        if (solucao_perturbada.custo_total > solucao_atual.custo_total) {
            // Aceitar nova solução
            solucao_atual = solucao_perturbada;
            iteracoes_sem_melhoria = 0;
            
            // Atualizar melhor solução se necessário
            if (solucao_atual.custo_total > melhor_solucao.custo_total) {
                melhor_solucao = solucao_atual;
            }
        } else {
            // Critério de aceitação com probabilidade (simulated annealing-like)
            double delta = solucao_perturbada.custo_total - solucao_atual.custo_total;
            double temperatura = 0.1 * std::exp(-0.05 * iter); // Decai com o tempo
            double prob_aceitacao = std::exp(delta / temperatura);
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);
            
            if (dis(gen) < prob_aceitacao) {
                solucao_atual = solucao_perturbada;
            }
            
            iteracoes_sem_melhoria++;
        }
    }
    
    return melhor_solucao;
}

// Adicionar após a implementação de aplicarILS

Solucao construirSolucaoInicial(const Problema& problema, int L_min, int L_max) {
    Solucao solucao;
    solucao.custo_total = 0.0;
    solucao.pedidos_atendidos.clear();
    
    // Lista de todos os pedidos candidatos
    std::vector<int> pedidos_candidatos;
    for (int i = 0; i < problema.pedidos.size(); ++i) {
        pedidos_candidatos.push_back(i);
    }
    
    // Ordenar candidatos por densidade (itens/corredores necessários)
    std::sort(pedidos_candidatos.begin(), pedidos_candidatos.end(), 
             [&problema](int a, int b) {
                 std::vector<int> pedidos_a = {a};
                 std::vector<int> pedidos_b = {b};
                 
                 auto [corredores_a, itens_a] = calcularCorredoresEItens(pedidos_a, problema);
                 auto [corredores_b, itens_b] = calcularCorredoresEItens(pedidos_b, problema);
                 
                 double densidade_a = static_cast<double>(itens_a) / corredores_a.size();
                 double densidade_b = static_cast<double>(itens_b) / corredores_b.size();
                 
                 return densidade_a > densidade_b;
             });
    
    // Construir solução gulosa
    std::set<int> corredores_selecionados;
    int total_itens = 0;
    
    for (int pedido_id : pedidos_candidatos) {
        // Verificar se adicionar este pedido excede o limite máximo
        std::vector<int> pedidos_temp = solucao.pedidos_atendidos;
        pedidos_temp.push_back(pedido_id);
        
        auto [corredores_temp, itens_temp] = calcularCorredoresEItens(pedidos_temp, problema);
        
        if (itens_temp <= L_max) {
            solucao.pedidos_atendidos.push_back(pedido_id);
            corredores_selecionados = corredores_temp;
            total_itens = itens_temp;
        }
    }
    
    // Finalizar a solução
    solucao.corredores_utilizados = std::vector<int>(corredores_selecionados.begin(), 
                                                    corredores_selecionados.end());
    
    // Calcular o custo (benefício) da solução
    if (!solucao.corredores_utilizados.empty()) {
        solucao.custo_total = static_cast<double>(total_itens) / solucao.corredores_utilizados.size();
    } else {
        solucao.custo_total = 0.0;
    }
    
    return solucao;
}

Solucao perturbarSolucao(const Solucao& solucao_atual, const Problema& problema, 
                        double intensidade, int L_min, int L_max) {
    // Copiar a solução atual
    Solucao solucao_perturbada = solucao_atual;
    
    // Se a solução atual está vazia, retornar uma solução inicial
    if (solucao_perturbada.pedidos_atendidos.empty()) {
        return construirSolucaoInicial(problema, L_min, L_max);
    }
    
    // Quantidade de elementos a remover baseada na intensidade da perturbação
    int num_remover = std::max(1, static_cast<int>(solucao_atual.pedidos_atendidos.size() * intensidade));
    
    // Remover pedidos aleatoriamente
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < num_remover && !solucao_perturbada.pedidos_atendidos.empty(); ++i) {
        std::uniform_int_distribution<> dis(0, solucao_perturbada.pedidos_atendidos.size() - 1);
        int idx_remover = dis(gen);
        solucao_perturbada.pedidos_atendidos.erase(
            solucao_perturbada.pedidos_atendidos.begin() + idx_remover);
    }
    
    // Adicionar novos pedidos aleatoriamente
    std::vector<int> candidatos;
    for (int i = 0; i < problema.pedidos.size(); ++i) {
        if (std::find(solucao_perturbada.pedidos_atendidos.begin(), 
                     solucao_perturbada.pedidos_atendidos.end(), i) == 
                     solucao_perturbada.pedidos_atendidos.end()) {
            candidatos.push_back(i);
        }
    }
    
    // Embaralhar candidatos
    std::shuffle(candidatos.begin(), candidatos.end(), gen);
    
    // Adicionar novos pedidos enquanto respeitar o limite máximo
    for (int pedido_id : candidatos) {
        std::vector<int> pedidos_temp = solucao_perturbada.pedidos_atendidos;
        pedidos_temp.push_back(pedido_id);
        
        auto [corredores_temp, itens_temp] = calcularCorredoresEItens(pedidos_temp, problema);
        
        if (itens_temp <= L_max) {
            solucao_perturbada.pedidos_atendidos.push_back(pedido_id);
        }
    }
    
    // Recalcular corredores e custo
    auto [corredores, total_itens] = calcularCorredoresEItens(
        solucao_perturbada.pedidos_atendidos, problema);
    
    solucao_perturbada.corredores_utilizados = 
        std::vector<int>(corredores.begin(), corredores.end());
    
    if (!solucao_perturbada.corredores_utilizados.empty()) {
        solucao_perturbada.custo_total = 
            static_cast<double>(total_itens) / solucao_perturbada.corredores_utilizados.size();
    } else {
        solucao_perturbada.custo_total = 0.0;
    }
    
    return solucao_perturbada;
}

// Implementação do Método de Dinkelbach para o problema

Solucao aplicarDinkelbach(const Problema& problema, const Parametros& params, const MetricasInstancia& metricas) {
    // Inicialização
    double lambda = 0.0; // Valor inicial da razão
    const int L_min = std::max(1, static_cast<int>(problema.pedidos.size() * 0.1)); // Mínimo de itens
    const int L_max = problema.UB; // Máximo de itens
    
    // Construir solução inicial
    Solucao solucao_atual = construirSolucaoInicial(problema, L_min, L_max);
    
    // Se não conseguiu construir uma solução viável
    if (solucao_atual.pedidos_atendidos.empty() || solucao_atual.corredores_utilizados.empty()) {
        return solucao_atual; // Retorna solução vazia
    }
    
    // Calcular razão inicial
    auto [corredores_inicial, itens_inicial] = calcularCorredoresEItens(solucao_atual.pedidos_atendidos, problema);
    double lambda_atual = static_cast<double>(itens_inicial) / corredores_inicial.size();
    
    // Parâmetros de convergência
    const double precisao = 1e-6;
    const int max_iteracoes = 100;
    
    for (int iter = 0; iter < max_iteracoes; ++iter) {
        // Resolver o problema paramétrico para o lambda atual
        Solucao nova_solucao = resolverProblemaParametrico(problema, lambda_atual, L_min, L_max);
        
        // Se não encontrou solução viável
        if (nova_solucao.pedidos_atendidos.empty() || nova_solucao.corredores_utilizados.empty()) {
            break;
        }
        
        // Calcular nova razão
        auto [corredores, itens] = calcularCorredoresEItens(nova_solucao.pedidos_atendidos, problema);
        double lambda_novo = static_cast<double>(itens) / corredores.size();
        
        // Verificar convergência
        if (std::abs(lambda_novo - lambda_atual) < precisao) {
            // Aplicar busca local para refinar a solução
            busca_local_otimizada(problema, nova_solucao);
            return nova_solucao;
        }
        
        // Atualizar lambda para a próxima iteração
        lambda_atual = lambda_novo;
        solucao_atual = nova_solucao;
    }
    
    // Aplicar busca local final
    busca_local_otimizada(problema, solucao_atual);
    
    // Retornar a melhor solução encontrada
    return solucao_atual;
}

// Função para resolver o subproblema paramétrico
Solucao resolverProblemaParametrico(const Problema& problema, double lambda, int L_min, int L_max) {
    // Este é o subproblema: max { F(x) - lambda * G(x) }
    // onde F(x) = total de itens, G(x) = total de corredores

    if (problema.o <= 20) {
        // Usar abordagem exata para problemas pequenos
        return resolverParametricoExato(problema, lambda, L_min, L_max);
    } else {
        // Usar abordagem heurística para problemas maiores
        return resolverParametricoHeuristico(problema, lambda, L_min, L_max);
    }
}

// Implementação exata do subproblema para instâncias pequenas
Solucao resolverParametricoExato(const Problema& problema, double lambda, int L_min, int L_max) {
    int n = problema.pedidos.size();
    std::vector<int> melhor_combinacao;
    double melhor_valor = -std::numeric_limits<double>::max();
    
    // Tentar todas as combinações possíveis de pedidos
    for (int k = 1; k <= std::min(30, n); ++k) {  // Limitar para evitar explosão combinatória
        std::vector<int> indices(k);
        // Inicializar com os primeiros k índices
        for (int i = 0; i < k; ++i) {
            indices[i] = i;
        }
        
        do {
            // Verificar valor da função objetivo parametrizada
            auto [corredores, itens] = calcularCorredoresEItens(indices, problema);
            
            // Verificar restrições de limites
            if (itens < L_min || itens > L_max) continue;
            
            // Se não há corredores, pular (solução inválida)
            if (corredores.empty()) continue;
            
            // Calcular valor: F(x) - lambda * G(x)
            double valor = itens - lambda * corredores.size();
            
            if (valor > melhor_valor) {
                melhor_valor = valor;
                melhor_combinacao = indices;
            }
            
        } while (next_combination(indices, n));
    }
    
    // Construir solução a partir da melhor combinação
    Solucao solucao;
    solucao.pedidos_atendidos = melhor_combinacao;
    
    // Se não encontramos uma combinação viável
    if (melhor_combinacao.empty()) {
        solucao.custo_total = 0.0;
        return solucao;
    }
    
    auto [corredores, itens] = calcularCorredoresEItens(melhor_combinacao, problema);
    solucao.corredores_utilizados = std::vector<int>(corredores.begin(), corredores.end());
    solucao.custo_total = static_cast<double>(itens) / corredores.size();
    
    return solucao;
}

// Implementação heurística do subproblema para instâncias maiores
Solucao resolverParametricoHeuristico(const Problema& problema, double lambda, int L_min, int L_max) {
    // Iniciar com conjunto vazio
    std::vector<int> pedidos_selecionados;
    int total_itens = 0;
    
    // Calcular benefício marginal para cada pedido (considerando λ)
    std::vector<std::tuple<double, int, std::set<int>>> beneficios; // (benefício ajustado, pedido_id, corredores)
    
    for (int i = 0; i < problema.pedidos.size(); ++i) {
        std::vector<int> temp_pedidos = {i};
        auto [temp_corredores, temp_itens] = calcularCorredoresEItens(temp_pedidos, problema);
        
        // Se não cobrir itens ou não tiver corredores, pular
        if (temp_itens == 0 || temp_corredores.empty()) continue;
        
        // Benefício parametrizado: itens - lambda * corredores
        double beneficio = temp_itens - lambda * temp_corredores.size();
        
        // Adicionar bônus para produtos priorizados
        double bonus_prioridade = 0;
        for (const auto& [item_id, qtd] : problema.pedidos[i].itens) {
            auto it = std::find_if(problema.produtos_priorizados.begin(), 
                                  problema.produtos_priorizados.end(),
                                  [item_id](const PrioridadeProduto& pp) { 
                                      return pp.id == item_id; 
                                  });
            if (it != problema.produtos_priorizados.end()) {
                bonus_prioridade += it->valor_prioridade * 0.05; // Fator de ajuste menor
            }
        }
        
        beneficio += bonus_prioridade;
        beneficios.push_back({beneficio, i, temp_corredores});
    }
    
    // Ordenar por benefício parametrizado (decrescente)
    std::sort(beneficios.begin(), beneficios.end(), 
             [](const auto& a, const auto& b) { return std::get<0>(a) > std::get<0>(b); });
    
    // Conjunto de corredores selecionados
    std::set<int> corredores_selecionados;
    
    // Construir solução gulosa
    for (const auto& [beneficio, pedido_id, corredores_pedido] : beneficios) {
        // Verificar se adicionar este pedido excede o limite máximo
        std::vector<int> temp_pedidos = pedidos_selecionados;
        temp_pedidos.push_back(pedido_id);
        
        auto [temp_corredores, temp_itens] = calcularCorredoresEItens(temp_pedidos, problema);
        
        if (temp_itens <= L_max) {
            // Adicionar este pedido
            pedidos_selecionados = temp_pedidos;
            corredores_selecionados = temp_corredores;
            total_itens = temp_itens;
            
            // Se já atingimos o mínimo, verificar se o benefício parametrizado é positivo
            if (total_itens >= L_min && beneficio <= 0) {
                break; // Parar de adicionar quando não há mais benefício marginal positivo
            }
        }
    }
    
    // Verificar a restrição de itens mínimos
    if (total_itens < L_min) {
        // Tentativa adicional usando critério de densidade
        std::vector<std::pair<double, int>> densidade_pedidos;
        
        for (int i = 0; i < problema.pedidos.size(); ++i) {
            if (std::find(pedidos_selecionados.begin(), pedidos_selecionados.end(), i) != pedidos_selecionados.end()) {
                continue; // Já selecionado
            }
            
            std::vector<int> temp_pedidos = {i};
            auto [temp_corredores, temp_itens] = calcularCorredoresEItens(temp_pedidos, problema);
            
            if (temp_itens > 0 && !temp_corredores.empty()) {
                double densidade = static_cast<double>(temp_itens) / temp_corredores.size();
                densidade_pedidos.push_back({densidade, i});
            }
        }
        
        // Ordenar por densidade (decrescente)
        std::sort(densidade_pedidos.begin(), densidade_pedidos.end(), std::greater<>());
        
        // Adicionar pedidos por densidade até atingir o mínimo
        for (const auto& [densidade, pedido_id] : densidade_pedidos) {
            if (total_itens >= L_min) break;
            
            if (std::find(pedidos_selecionados.begin(), pedidos_selecionados.end(), pedido_id) != pedidos_selecionados.end()) {
                continue; // Já selecionado
            }
            
            std::vector<int> temp_pedidos = pedidos_selecionados;
            temp_pedidos.push_back(pedido_id);
            
            auto [temp_corredores, temp_itens] = calcularCorredoresEItens(temp_pedidos, problema);
            
            if (temp_itens <= L_max) {
                // Adicionar este pedido
                pedidos_selecionados = temp_pedidos;
                corredores_selecionados = temp_corredores;
                total_itens = temp_itens;
            }
        }
    }
    
    // Construir solução final
    Solucao solucao;
    solucao.pedidos_atendidos = pedidos_selecionados;
    solucao.corredores_utilizados = std::vector<int>(corredores_selecionados.begin(), corredores_selecionados.end());
    
    if (!corredores_selecionados.empty()) {
        solucao.custo_total = static_cast<double>(total_itens) / corredores_selecionados.size();
    } else {
        solucao.custo_total = 0.0;
    }
    
    return solucao;
}

// Substitua a função solucionadorRobusto existente

Solucao solucionadorRobusto(const Problema& problema) {
    MetricasInstancia metricas = calcularMetricas(problema);
    Parametros params;
    if (!calibrarAlgoritmo(params)) {
        // Usar parâmetros padrão
        params.intensidade_perturbacao = 0.3;
        params.max_iteracoes_grasp = 50;
    }
    
    // Abordagem adaptativa baseada no tamanho do problema
    if (problema.o <= 8) {
        // Para problemas pequenos, usar busca exaustiva
        return buscaExaustiva(problema);
    } else if (problema.o <= 25) {
        // Para problemas médios, usar Dinkelbach
        return aplicarDinkelbach(problema, params, metricas);
    } else {
        // Para problemas grandes, usar ILS (mais eficiente computacionalmente)
        return aplicarILS(problema, params, metricas);
    }
}

// Implementar a nova função acelerada após a função Dinkelbach existente
Solucao aplicarDinkelbachAcelerado(const Problema& problema, const Parametros& params, const MetricasInstancia& metricas) {
    // Obter múltiplas soluções iniciais de alta qualidade
    std::vector<Solucao> solucoes_iniciais;
    
    // Aplicar construção gulosa com perturbação aleatória
    for (int i = 0; i < 5; ++i) {
        solucoes_iniciais.push_back(construirSolucaoInicial(problema, 0, INT_MAX));
    }
    
    // Adicionar solução do ILS
    solucoes_iniciais.push_back(aplicarILS(problema, params, metricas));
    
    // Selecionar a melhor solução inicial
    Solucao melhor_solucao = *std::max_element(solucoes_iniciais.begin(), solucoes_iniciais.end(), 
                                              [](const Solucao& a, const Solucao& b) {
                                                  return a.custo_total < b.custo_total;
                                              });
    
    // Aplicar o método de Dinkelbach com critério de parada adaptativo
    const double EPSILON = 1e-6;
    const int MAX_ITER = 20;
    
    double lambda_atual = melhor_solucao.custo_total;
    Solucao solucao_atual = melhor_solucao;
    
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        // Resolver o subproblema paramétrico mais eficientemente
        Solucao nova_solucao = resolverProblemaParametricoOtimizado(problema, lambda_atual, 0, INT_MAX);
        
        // Calcular o valor da função auxiliar F(λ)
        double F_lambda = nova_solucao.custo_total - lambda_atual;
        
        // Verificar convergência com tolerância adaptativa
        if (std::abs(F_lambda) < EPSILON * (1.0 + std::abs(lambda_atual))) {
            return nova_solucao;
        }
        
        // Atualizar lambda usando o método de Newton-Raphson
        if (!nova_solucao.corredores_utilizados.empty()) {
            // Calculate total items in the solution
            int total_itens = 0;
            for (int pedido_id : nova_solucao.pedidos_atendidos) {
                for (const auto& item_info : problema.pedidos[pedido_id].itens) {
                    total_itens += item_info.second; // Quantity of item
                }
            }
            double lambda_novo = static_cast<double>(total_itens) / 
                              nova_solucao.corredores_utilizados.size();
            
            // Verificar melhoria
            if (lambda_novo > lambda_atual) {
                solucao_atual = nova_solucao;
                lambda_atual = lambda_novo;
            } else {
                // Se não melhorou, aplique uma perturbação maior e continue
                lambda_atual = lambda_atual * 0.98 + lambda_novo * 0.02;
            }
        }
    }
    
    // Aplicar busca local intensiva na melhor solução
    busca_local_intensiva(problema, solucao_atual);
    
    return solucao_atual;
}

// Implementar a nova função após resolverProblemaParametrico
Solucao resolverProblemaParametricoOtimizado(const Problema& problema, double lambda, int L_min, int L_max) {
    // Estratégia de seleção baseada em prioridades considerando lambda
    
    // Calcular valor de cada pedido baseado no lambda atual
    std::vector<std::pair<int, double>> valores_pedidos;
    for (int o = 0; o < problema.o; ++o) {
        // Total de itens no pedido
        int total_itens = problema.pedidos[o].total_itens;
        
        // Estimar número de corredores adicionais necessários
        std::set<int> corredores_pedido;
        for (const auto& item : problema.pedidos[o].itens) {
            int item_id = item.first;
            for (int corredor : problema.item_para_corredores.at(item_id)) {
                corredores_pedido.insert(corredor);
            }
        }
        
        // Valor do pedido é itens - lambda * corredores
        double valor = total_itens - lambda * corredores_pedido.size();
        valores_pedidos.push_back({o, valor});
    }
    
    // Ordenar pedidos por valor (decrescente)
    std::sort(valores_pedidos.begin(), valores_pedidos.end(),
             [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                 return a.second > b.second;
             });
    
    // Construir solução gulosa com viés para pedidos de alto valor
    Solucao solucao;
    std::set<int> corredores_selecionados;
    int total_itens = 0;
    
    // Selecionar primeiro os pedidos de maior valor
    for (const auto& [pedido_id, valor] : valores_pedidos) {
        if (valor <= 0) continue; // Skip pedidos com valor negativo
        
        // Verificar corredores adicionais necessários
        std::set<int> novos_corredores;
        std::set<int> corredores_pedido;
        
        // Coletar corredores necessários para este pedido
        for (const auto& item : problema.pedidos[pedido_id].itens) {
            int item_id = item.first;
            for (int corredor : problema.item_para_corredores.at(item_id)) {
                if (corredores_selecionados.find(corredor) == corredores_selecionados.end()) {
                    novos_corredores.insert(corredor);
                }
                corredores_pedido.insert(corredor);
            }
        }
        
        int itens_pedido = problema.pedidos[pedido_id].total_itens;
        
        // Verificar se ainda vale a pena adicionar este pedido
        double ganho_liquido = itens_pedido - lambda * novos_corredores.size();
        if (ganho_liquido > 0) {
            // Adicionar pedido à solução
            solucao.pedidos_atendidos.push_back(pedido_id);
            total_itens += itens_pedido;
            
            // Atualizar corredores
            for (int corredor : novos_corredores) {
                corredores_selecionados.insert(corredor);
            }
        }
    }
    
    // Finalizar solução
    solucao.corredores_utilizados.assign(corredores_selecionados.begin(), corredores_selecionados.end());
    if (!solucao.corredores_utilizados.empty()) {
        solucao.custo_total = static_cast<double>(total_itens) / solucao.corredores_utilizados.size();
    } else {
        solucao.custo_total = 0.0;
    }
    
    return solucao;
}

// Adicionar função de suporte para calcular corredores adicionais
std::set<int> calcularCorredoresAdicionais(const Problema& problema, int pedido_id, const std::set<int>& corredores_existentes);

// Adicionar função para ajustar solução para respeitar limites
void ajustarSolucaoParaLimites(Solucao& solucao, const Problema& problema, int total_itens, int L_min, int L_max);

// Adicionar função para calcular valor da função auxiliar F(λ)
double calcularValorFuncaoAuxiliar(const Solucao& solucao, double lambda);

#include "problema.h"
#include "solucao.h"

// Estrutura para parâmetros de calibração do algoritmo
struct Parametros {
    int iteracoes_max;
    double temperatura_inicial;
    double fator_resfriamento;
    // Adicione outros parâmetros conforme necessário
};

// Função para calibrar os parâmetros do algoritmo
bool calibrarAlgoritmo(Parametros& parametros);

#include "algoritmos.h"

// Implementação da função de calibração do algoritmo
bool calibrarAlgoritmo(Parametros& parametros) {
    // Definir valores padrão para os parâmetros
    parametros.iteracoes_max = 1000;
    parametros.temperatura_inicial = 100.0;
    parametros.fator_resfriamento = 0.95;
    // Adicione outras configurações conforme necessário
    
    return true; // Retorna true se a calibração foi bem-sucedida
}