#include "otimizador_multiobjetivo.h"
#include <algorithm>
#include <random>
#include <numeric>
#include <cmath>
#include <chrono>
#include <iostream>
#include <limits>
#include <unordered_set>

// Define the ObjetivoEnum if it's not already defined in the header
#ifndef ObjetivoEnum_DEFINED
#define ObjetivoEnum_DEFINED
enum class ObjetivoEnum {
    MAXIMIZAR_UNIDADES_POR_CORREDOR,
    MINIMIZAR_DISTANCIA_TOTAL,
    MAXIMIZAR_PRIORIDADE_PEDIDOS,
    BALANCEAR_CARGA_CORREDORES,
    MINIMIZAR_TEMPO_COLETA
};
#endif

OtimizadorMultiobjetivo::OtimizadorMultiobjetivo(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador
) : 
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador) {
    
    // Configuração padrão: apenas maximizar unidades por corredor
    objetivosSelecionados_ = {ObjetivoEnum::MAXIMIZAR_UNIDADES_POR_CORREDOR};
    pesosObjetivos_ = {1.0};
}

void OtimizadorMultiobjetivo::configurarObjetivos(
    const std::vector<ObjetivoEnum>& objetivos,
    const std::vector<double>& pesos
) {
    if (objetivos.size() != pesos.size()) {
        throw std::invalid_argument("Número de objetivos e pesos deve ser o mesmo");
    }
    
    objetivosSelecionados_ = objetivos;
    pesosObjetivos_ = pesos;
    
    // Normalizar pesos
    double somaPesos = std::accumulate(pesos.begin(), pesos.end(), 0.0);
    if (somaPesos > 0) {
        for (auto& peso : pesosObjetivos_) {
            peso /= somaPesos;
        }
    }
}

std::vector<SolucaoMultiobjetivo> OtimizadorMultiobjetivo::otimizarNSGAII(
    int tamanhoPopulacao,
    int numGeracoes,
    int LB, int UB
) {
    // Configurar gerador de números aleatórios
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Inicializar população aleatória
    std::vector<SolucaoMultiobjetivo> populacao;
    populacao.reserve(tamanhoPopulacao);
    
    // Criar soluções iniciais aleatórias
    for (int i = 0; i < tamanhoPopulacao; i++) {
        SolucaoMultiobjetivo solucao;
        
        // Gerar uma solução aleatória viável
        // Algoritmo simplificado: selecionar pedidos aleatoriamente até atingir LB
        std::vector<int> todosPedidos(backlog_.numPedidos);
        std::iota(todosPedidos.begin(), todosPedidos.end(), 0);
        std::shuffle(todosPedidos.begin(), todosPedidos.end(), gen);
        
        int totalUnidades = 0;
        std::unordered_set<int> corredoresIncluidos;
        
        for (int pedidoId : todosPedidos) {
            // Calcular unidades adicionais e corredores adicionais
            int unidadesAdicionais = 0;
            std::unordered_set<int> corredoresAdicionais;
            
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
                unidadesAdicionais += quantidade;
                
                // Adicionar corredores necessários para este item
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    if (corredoresIncluidos.find(corredorId) == corredoresIncluidos.end()) {
                        corredoresAdicionais.insert(corredorId);
                    }
                }
            }
            
            // Verificar se adicionar este pedido excederia o limite UB
            if (totalUnidades + unidadesAdicionais > UB) {
                continue;
            }
            
            // Adicionar este pedido à solução
            solucao.pedidosWave.push_back(pedidoId);
            totalUnidades += unidadesAdicionais;
            
            // Adicionar corredores à solução
            for (int corredorId : corredoresAdicionais) {
                corredoresIncluidos.insert(corredorId);
            }
            
            // Se atingiu o limite inferior, podemos parar
            if (totalUnidades >= LB) {
                break;
            }
        }
        
        // Converter conjunto de corredores para vetor
        solucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
        
        // Calcular valores dos objetivos
        solucao.valoresObjetivo.resize(objetivosSelecionados_.size(), 0.0);
        avaliarObjetivos(solucao);
        
        solucao.dominada = false;
        populacao.push_back(solucao);
    }
    
    // Loop principal do NSGA-II
    for (int geracao = 0; geracao < numGeracoes; geracao++) {
        // Criar população combinada (pais + filhos)
        std::vector<SolucaoMultiobjetivo> populacaoCombinada = populacao;
        
        // Gerar novos indivíduos (filhos) através de crossover e mutação
        for (int i = 0; i < tamanhoPopulacao; i++) {
            int idxPai1 = selecaoTorneio(populacao, gen);
            int idxPai2 = selecaoTorneio(populacao, gen);
            
            // Aplicar crossover
            SolucaoMultiobjetivo filho = crossover(populacao[idxPai1], populacao[idxPai2], gen);
            
            // Aplicar mutação
            mutacao(filho, gen, LB, UB);
            
            // Calcular valores dos objetivos
            avaliarObjetivos(filho);
            
            // Adicionar à população combinada
            populacaoCombinada.push_back(filho);
        }
        
        // Ordenar população combinada usando dominância de Pareto
        ordenarPorDominancia(populacaoCombinada);
        
        // Selecionar os melhores indivíduos para a próxima geração
        populacao.clear();
        populacao.reserve(tamanhoPopulacao);
        
        // Primeiro adicionar todas as soluções não dominadas
        for (const auto& solucao : populacaoCombinada) {
            if (populacao.size() >= tamanhoPopulacao) {
                break;
            }
            
            if (!solucao.dominada) {
                populacao.push_back(solucao);
            }
        }
        
        // Se não temos indivíduos suficientes, adicionar os próximos melhores
        if (populacao.size() < tamanhoPopulacao) {
            for (const auto& solucao : populacaoCombinada) {
                if (populacao.size() >= tamanhoPopulacao) {
                    break;
                }
                
                if (solucao.dominada && 
                    std::find(populacao.begin(), populacao.end(), solucao) == populacao.end()) {
                    populacao.push_back(solucao);
                }
            }
        }
    }
    
    // Retornar apenas as soluções não dominadas (fronteira de Pareto)
    std::vector<SolucaoMultiobjetivo> fronteira;
    for (const auto& solucao : populacao) {
        if (!solucao.dominada) {
            fronteira.push_back(solucao);
        }
    }
    
    return fronteira;
}

std::vector<SolucaoMultiobjetivo> OtimizadorMultiobjetivo::otimizarMOEAD(
    int tamanhoPopulacao,
    int numGeracoes,
    int LB, int UB
) {
    // Configurar gerador de números aleatórios
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Inicializar vetores de peso uniformemente distribuídos
    std::vector<std::vector<double>> vetoresPeso;
    vetoresPeso.reserve(tamanhoPopulacao);
    
    // Para simplicidade, apenas consideramos o caso bi-objetivo
    // Em uma implementação real, usaríamos uma técnica para gerar vetores em múltiplas dimensões
    for (int i = 0; i < tamanhoPopulacao; i++) {
        double w1 = static_cast<double>(i) / (tamanhoPopulacao - 1);
        double w2 = 1.0 - w1;
        vetoresPeso.push_back({w1, w2});
    }
    
    // Inicializar população
    std::vector<SolucaoMultiobjetivo> populacao;
    populacao.reserve(tamanhoPopulacao);
    
    // Criar vizinhança para cada subproblema
    int T = std::min(10, tamanhoPopulacao / 5);    
    std::vector<std::vector<int>> vizinhos(tamanhoPopulacao, std::vector<int>(T));
    
    // Determinar vizinhos baseados na distância entre vetores de peso
    for (int i = 0; i < tamanhoPopulacao; i++) {
        std::vector<std::pair<double, int>> distancias;
        for (int j = 0; j < tamanhoPopulacao; j++) {
            if (i != j) {
                double dist = 0.0;
                for (size_t k = 0; k < vetoresPeso[i].size(); k++) {
                    dist += pow(vetoresPeso[i][k] - vetoresPeso[j][k], 2);
                }
                dist = sqrt(dist);
                distancias.push_back({dist, j});
            }
        }
        
        // Ordenar por distância e pegar os T mais próximos
        std::sort(distancias.begin(), distancias.end());
        for (int j = 0; j < T; j++) {
            vizinhos[i][j] = distancias[j].second;
        }
    }
    
    // Inicializar população e valores ideais
    std::vector<double> z(objetivosSelecionados_.size(), std::numeric_limits<double>::max());
    
    for (int i = 0; i < tamanhoPopulacao; i++) {
        // Gerar solução aleatória como no NSGA-II
        SolucaoMultiobjetivo solucao;
        
        // Código para gerar solução aleatória (similar ao NSGA-II)
        std::vector<int> todosPedidos(backlog_.numPedidos);
        std::iota(todosPedidos.begin(), todosPedidos.end(), 0);
        std::shuffle(todosPedidos.begin(), todosPedidos.end(), gen);
        
        int totalUnidades = 0;
        std::unordered_set<int> corredoresIncluidos;
        
        for (int pedidoId : todosPedidos) {
            int unidadesAdicionais = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
                unidadesAdicionais += quantidade;
                
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    corredoresIncluidos.insert(corredorId);
                }
            }
            
            if (totalUnidades + unidadesAdicionais > UB) {
                continue;
            }
            
            solucao.pedidosWave.push_back(pedidoId);
            totalUnidades += unidadesAdicionais;
            
            if (totalUnidades >= LB) break;
        }
        
        solucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
        solucao.valoresObjetivo.resize(objetivosSelecionados_.size(), 0.0);
        
        // Calcular valores dos objetivos
        avaliarObjetivos(solucao);
        
        // Atualizar pontos ideais
        for (size_t j = 0; j < z.size(); j++) {
            z[j] = std::min(z[j], solucao.valoresObjetivo[j]);
        }
        
        populacao.push_back(solucao);
    }
    
    // Loop principal do MOEA/D
    for (int geracao = 0; geracao < numGeracoes; geracao++) {
        for (int i = 0; i < tamanhoPopulacao; i++) {
            // Selecionar dois índices da vizinhança
            std::uniform_int_distribution<> dist(0, T - 1);
            int k = vizinhos[i][dist(gen)];
            int l = vizinhos[i][dist(gen)];
            
            // Criar uma nova solução através de operadores genéticos
            SolucaoMultiobjetivo filho = crossover(populacao[k], populacao[l], gen);
            mutacao(filho, gen, LB, UB);
            avaliarObjetivos(filho);
            
            // Atualizar ponto ideal
            for (size_t j = 0; j < z.size(); j++) {
                z[j] = std::min(z[j], filho.valoresObjetivo[j]);
            }
            
            // Atualizar soluções vizinhas
            for (int j = 0; j < T; j++) {
                int vizinhoIdx = vizinhos[i][j];
                
                // Calcular valor da função objetivo ponderada usando Tchebycheff
                double g_x = 0.0;
                double g_y = 0.0;
                
                for (size_t k = 0; k < objetivosSelecionados_.size(); k++) {
                    double diff_x = std::abs(populacao[vizinhoIdx].valoresObjetivo[k] - z[k]);
                    double diff_y = std::abs(filho.valoresObjetivo[k] - z[k]);
                    
                    g_x = std::max(g_x, vetoresPeso[vizinhoIdx][k] * diff_x);
                    g_y = std::max(g_y, vetoresPeso[vizinhoIdx][k] * diff_y);
                }
                
                // Se a nova solução é melhor, substituir
                if (g_y <= g_x) {
                    populacao[vizinhoIdx] = filho;
                }
            }
        }
    }
    
    // Identificar soluções não-dominadas
    ordenarPorDominancia(populacao);
    
    // Retornar apenas as soluções não dominadas (fronteira de Pareto)
    std::vector<SolucaoMultiobjetivo> fronteira;
    for (const auto& solucao : populacao) {
        if (!solucao.dominada) {
            fronteira.push_back(solucao);
        }
    }
    
    return fronteira;
}

SolucaoMultiobjetivo OtimizadorMultiobjetivo::selecionarSolucaoPreferida(
    const std::vector<SolucaoMultiobjetivo>& solucoesFronteira
) {
    if (solucoesFronteira.empty()) {
        throw std::runtime_error("Não há soluções na fronteira de Pareto");
    }
    
    // Caso mais simples: apenas uma solução na fronteira
    if (solucoesFronteira.size() == 1) {
        return solucoesFronteira[0];
    }
    
    // Determinar pontos ideais e nadir para cada objetivo
    std::vector<double> idealPoint(objetivosSelecionados_.size(), std::numeric_limits<double>::lowest());
    std::vector<double> nadirPoint(objetivosSelecionados_.size(), std::numeric_limits<double>::max());
    
    // Isso assume que devemos maximizar todos os objetivos
    // Em uma implementação real, usaríamos a direção de cada objetivo
    for (const auto& solucao : solucoesFronteira) {
        for (size_t i = 0; i < objetivosSelecionados_.size(); i++) {
            idealPoint[i] = std::max(idealPoint[i], solucao.valoresObjetivo[i]);
            nadirPoint[i] = std::min(nadirPoint[i], solucao.valoresObjetivo[i]);
        }
    }
    
    // Normalizar para evitar bias devido a diferentes escalas
    std::vector<double> range(objetivosSelecionados_.size());
    for (size_t i = 0; i < objetivosSelecionados_.size(); i++) {
        range[i] = idealPoint[i] - nadirPoint[i];
        if (range[i] <= 0) range[i] = 1.0; // Evitar divisão por zero
    }
    
    // Escolher solução usando método ponderado de Tchebycheff
    double melhorValor = std::numeric_limits<double>::lowest();
    int melhorIndice = 0;
    
    for (size_t i = 0; i < solucoesFronteira.size(); i++) {
        double valor = 0.0;
        
        for (size_t j = 0; j < objetivosSelecionados_.size(); j++) {
            // Normalizar o valor do objetivo
            double valorNormalizado = (solucoesFronteira[i].valoresObjetivo[j] - nadirPoint[j]) / range[j];
            
            // Aplicar peso
            valor += pesosObjetivos_[j] * valorNormalizado;
        }
        
        if (valor > melhorValor) {
            melhorValor = valor;
            melhorIndice = i;
        }
    }
    
    return solucoesFronteira[melhorIndice];
}

void OtimizadorMultiobjetivo::avaliarObjetivos(SolucaoMultiobjetivo& solucao) {
    // Reinicializar valores dos objetivos
    solucao.valoresObjetivo.resize(objetivosSelecionados_.size(), 0.0);
    
    // Calcular total de unidades
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Calcular corredores necessários (já temos isso no vetor corredoresWave)
    int totalCorredores = solucao.corredoresWave.size();
    
    // Avaliar cada objetivo selecionado
    for (size_t i = 0; i < objetivosSelecionados_.size(); i++) {
        switch (objetivosSelecionados_[i]) {
            case ObjetivoEnum::MAXIMIZAR_UNIDADES_POR_CORREDOR:
                // Evitar divisão por zero
                if (totalCorredores > 0) {
                    solucao.valoresObjetivo[i] = static_cast<double>(totalUnidades) / totalCorredores;
                } else {
                    solucao.valoresObjetivo[i] = 0.0;
                }
                break;
                
            case ObjetivoEnum::MINIMIZAR_DISTANCIA_TOTAL:
                // Exemplo simples: distância proporcional ao número de corredores
                // Queremos maximizar, então usamos o negativo da distância
                solucao.valoresObjetivo[i] = -static_cast<double>(totalCorredores);
                break;
                
            case ObjetivoEnum::MAXIMIZAR_PRIORIDADE_PEDIDOS:
                // Exemplo simples: prioridade baseada no número de pedidos
                solucao.valoresObjetivo[i] = static_cast<double>(solucao.pedidosWave.size());
                break;
                
            case ObjetivoEnum::BALANCEAR_CARGA_CORREDORES:
                // Exemplo simples: quanto mais uniforme a distribuição, melhor
                // Como não temos essa informação aqui, usamos um valor constante
                solucao.valoresObjetivo[i] = 0.0;
                break;
                
            case ObjetivoEnum::MINIMIZAR_TEMPO_COLETA:
                // Exemplo simples: proporcional ao número de corredores, queremos maximizar
                solucao.valoresObjetivo[i] = -static_cast<double>(totalCorredores);
                break;
        }
    }
}

int OtimizadorMultiobjetivo::selecaoTorneio(
    const std::vector<SolucaoMultiobjetivo>& populacao,
    std::mt19937& gen
) {
    // Implementação do torneio binário
    std::uniform_int_distribution<> dist(0, populacao.size() - 1);
    
    int idx1 = dist(gen);
    int idx2 = dist(gen);
    
    // Selecionar o melhor (não dominado)
    // No caso de ambos dominados ou não dominados, escolha aleatória
    if (!populacao[idx1].dominada && populacao[idx2].dominada) {
        return idx1;
    } else if (populacao[idx1].dominada && !populacao[idx2].dominada) {
        return idx2;
    } else {
        // Escolha aleatória entre os dois
        return (gen() % 2 == 0) ? idx1 : idx2;
    }
}

SolucaoMultiobjetivo OtimizadorMultiobjetivo::crossover(
    const SolucaoMultiobjetivo& pai1,
    const SolucaoMultiobjetivo& pai2,
    std::mt19937& gen
) {
    // Implementação de crossover de um ponto
    SolucaoMultiobjetivo filho;
    
    // Decidir ponto de corte para pedidos
    std::uniform_int_distribution<> distPedidos(0, std::min(pai1.pedidosWave.size(), pai2.pedidosWave.size()));
    int pontoCortePedidos = distPedidos(gen);
    
    // Pegar pedidos do primeiro pai até o ponto de corte
    std::unordered_set<int> pedidosIncluidos;
    for (int i = 0; i < pontoCortePedidos && i < pai1.pedidosWave.size(); i++) {
        int pedidoId = pai1.pedidosWave[i];
        filho.pedidosWave.push_back(pedidoId);
        pedidosIncluidos.insert(pedidoId);
    }
    
    // Pegar pedidos do segundo pai que não estão no filho
    for (int pedidoId : pai2.pedidosWave) {
        if (pedidosIncluidos.find(pedidoId) == pedidosIncluidos.end()) {
            filho.pedidosWave.push_back(pedidoId);
        }
    }
    
    // Recalcular corredores necessários
    std::unordered_set<int> corredoresIncluidos;
    for (int pedidoId : filho.pedidosWave) {
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresIncluidos.insert(corredorId);
            }
        }
    }
    
    // Converter conjunto para vetor
    filho.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
    
    return filho;
}

void OtimizadorMultiobjetivo::mutacao(
    SolucaoMultiobjetivo& solucao,
    std::mt19937& gen,
    int LB, int UB
) {
    // Implementação da mutação
    // Taxa de mutação: probabilidade de trocar um pedido
    double taxaMutacao = 0.1;
    
    // Primeiro, calcular unidades atuais
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Lista de todos os pedidos possíveis
    std::vector<int> todosPedidos;
    for (int i = 0; i < backlog_.numPedidos; i++) {
        // Incluir apenas pedidos que não estão na solução atual
        if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), i) == solucao.pedidosWave.end()) {
            todosPedidos.push_back(i);
        }
    }
    
    // Aplicar mutação em cada posição com probabilidade taxaMutacao
    for (size_t i = 0; i < solucao.pedidosWave.size(); i++) {
        std::uniform_real_distribution<> dist(0.0, 1.0);
        if (dist(gen) < taxaMutacao && !todosPedidos.empty()) {
            int pedidoRemovido = solucao.pedidosWave[i];
            
            // Calcular unidades que serão removidas
            int unidadesRemovidas = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoRemovido]) {
                unidadesRemovidas += quantidade;
            }
            
            // Escolher aleatoriamente um pedido para substituir
            std::uniform_int_distribution<> distPedido(0, todosPedidos.size() - 1);
            int idxPedidoNovo = distPedido(gen);
            int pedidoNovo = todosPedidos[idxPedidoNovo];
            
            // Calcular unidades que serão adicionadas
            int unidadesAdicionadas = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoNovo]) {
                unidadesAdicionadas += quantidade;
            }
            
            // Verificar se a substituição mantém a viabilidade (LB e UB)
            int novoTotal = totalUnidades - unidadesRemovidas + unidadesAdicionadas;
            if (novoTotal >= LB && novoTotal <= UB) {
                // Substituir o pedido
                solucao.pedidosWave[i] = pedidoNovo;
                totalUnidades = novoTotal;
                
                // Remover pedidoNovo da lista de pedidos disponíveis
                todosPedidos.erase(todosPedidos.begin() + idxPedidoNovo);
            }
        }
    }
    
    // Recalcular corredores necessários
    std::unordered_set<int> corredoresIncluidos;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresIncluidos.insert(corredorId);
            }
        }
    }
    
    // Atualizar corredores da solução
    solucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
}

void OtimizadorMultiobjetivo::ordenarPorDominancia(
    std::vector<SolucaoMultiobjetivo>& populacao
) {
    // Implementação da ordenação por dominância de Pareto
    int n = populacao.size();
    
    // Inicialmente, todas as soluções são consideradas não dominadas
    for (auto& solucao : populacao) {
        solucao.dominada = false;
    }
    
    // Verificar dominância entre todos os pares
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (!populacao[i].dominada || !populacao[j].dominada) {
                bool iDominaJ = true;
                bool jDominaI = true;
                
                // Verificar dominância em todos os objetivos
                for (size_t k = 0; k < populacao[i].valoresObjetivo.size(); k++) {
                    double vi = populacao[i].valoresObjetivo[k];
                    double vj = populacao[j].valoresObjetivo[k];
                    
                    // Assumindo que queremos maximizar todos os objetivos
                    if (vi < vj) {
                        iDominaJ = false;
                    }
                    if (vi > vj) {
                        jDominaI = false;
                    }
                }
                
                // Marcar soluções dominadas
                if (iDominaJ && !jDominaI) {
                    populacao[j].dominada = true;
                } else if (!iDominaJ && jDominaI) {
                    populacao[i].dominada = true;
                }
            }
        }
    }
}