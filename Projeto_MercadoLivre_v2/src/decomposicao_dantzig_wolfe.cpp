#include "decomposicao_dantzig_wolfe.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <random>

DecomposicaoDantzigWolfe::DecomposicaoDantzigWolfe(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador,
    double limiteTempo,
    double tolerancia,
    int maxIteracoes
) :
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    limiteTempo_(limiteTempo),
    tolerancia_(tolerancia),
    maxIteracoes_(maxIteracoes),
    limiteInferior_(-std::numeric_limits<double>::infinity()),
    limiteSuperior_(std::numeric_limits<double>::infinity()),
    iteracoesRealizadas_(0),
    tempoTotal_(0.0),
    gap_(0.0),
    colunasGeradas_(0)
{
    // Inicializar a melhor solução com valores vazios
    melhorSolucao_.valorObjetivo = 0.0;
    
    // Inicializar o conjunto de colunas
    inicializarColunas();
}

Solucao DecomposicaoDantzigWolfe::resolver() {
    auto inicioExecucao = std::chrono::high_resolution_clock::now();
    
    int iteracao = 0;
    bool convergiu = false;
    
    std::cout << "Iniciando algoritmo de decomposição de Dantzig-Wolfe..." << std::endl;
    
    while (!convergiu && iteracao < maxIteracoes_) {
        iteracao++;
        std::cout << "Iteração " << iteracao << " de Dantzig-Wolfe:" << std::endl;
        
        // 1. Resolver o problema mestre restrito
        bool solucaoViavel = resolverProblemaMestreRestrito();
        
        if (!solucaoViavel) {
            std::cout << "Problema mestre restrito inviável. Gerando colunas iniciais adicionais..." << std::endl;
            
            // Gerar colunas iniciais adicionais e tentar novamente
            for (int i = 0; i < 5; i++) {
                if (gerarNovaColuna()) {
                    colunasGeradas_++;
                }
            }
            
            // Se ainda não tivermos colunas suficientes, podemos parar
            if (colunas_.empty()) {
                std::cout << "Não foi possível gerar colunas viáveis. Encerrando." << std::endl;
                break;
            }
            
            continue;
        }
        
        // 2. Resolver o subproblema para gerar uma nova coluna
        bool colunaGerada = gerarNovaColuna();
        
        if (colunaGerada) {
            colunasGeradas_++;
            std::cout << "  Nova coluna gerada. Total de colunas: " << colunas_.size() << std::endl;
        } else {
            std::cout << "  Não foi possível gerar uma nova coluna com custo reduzido negativo." << std::endl;
            
            // Se não conseguirmos gerar uma nova coluna, a solução atual é ótima
            // para a relaxação linear. Agora precisamos obter uma solução inteira.
            Solucao solucaoAtual = construirSolucaoFinal();
            
            // Atualizar limites
            limiteInferior_ = std::max(limiteInferior_, solucaoAtual.valorObjetivo);
            
            if (solucaoAtual.valorObjetivo > melhorSolucao_.valorObjetivo) {
                melhorSolucao_ = solucaoAtual;
            }
            
            // Indicar que convergimos
            convergiu = true;
        }
        
        // 3. Verificar limite de tempo
        auto tempoAtual = std::chrono::high_resolution_clock::now();
        double segundosDecorridos = std::chrono::duration<double>(tempoAtual - inicioExecucao).count();
        if (segundosDecorridos > limiteTempo_) {
            std::cout << "Limite de tempo atingido após " << segundosDecorridos << " segundos" << std::endl;
            break;
        }
        
        // 4. Verificar convergência
        convergiu = verificarConvergencia();
        
        // Calcular o gap atual (simplificado)
        if (limiteSuperior_ != std::numeric_limits<double>::infinity() &&
            limiteInferior_ != -std::numeric_limits<double>::infinity() &&
            limiteSuperior_ > 0) {
            gap_ = 100.0 * (limiteSuperior_ - limiteInferior_) / limiteSuperior_;
        } else {
            gap_ = 100.0;
        }
        
        std::cout << "  Limite inferior: " << limiteInferior_ << std::endl;
        std::cout << "  Limite superior: " << limiteSuperior_ << std::endl;
        std::cout << "  Gap: " << gap_ << "%" << std::endl;
    }
    
    // Construir a solução final
    if (melhorSolucao_.pedidosWave.empty() && !colunas_.empty()) {
        melhorSolucao_ = construirSolucaoFinal();
    }
    
    // Registrar estatísticas finais
    iteracoesRealizadas_ = iteracao;
    auto fimExecucao = std::chrono::high_resolution_clock::now();
    tempoTotal_ = std::chrono::duration<double>(fimExecucao - inicioExecucao).count();
    
    std::cout << "Algoritmo de Dantzig-Wolfe concluído em " << tempoTotal_ << " segundos" << std::endl;
    std::cout << "Iterações realizadas: " << iteracoesRealizadas_ << std::endl;
    std::cout << "Colunas geradas: " << colunasGeradas_ << std::endl;
    std::cout << "Gap final: " << gap_ << "%" << std::endl;
    std::cout << "Valor objetivo: " << melhorSolucao_.valorObjetivo << std::endl;
    
    return melhorSolucao_;
}

std::string DecomposicaoDantzigWolfe::obterEstatisticas() const {
    std::stringstream ss;
    ss << "Estatísticas da Decomposição de Dantzig-Wolfe:" << std::endl;
    ss << "  Iterações realizadas: " << iteracoesRealizadas_ << std::endl;
    ss << "  Tempo total: " << std::fixed << std::setprecision(2) << tempoTotal_ << " segundos" << std::endl;
    ss << "  Colunas geradas: " << colunasGeradas_ << std::endl;
    ss << "  Gap final: " << std::fixed << std::setprecision(2) << gap_ << "%" << std::endl;
    ss << "  Limite inferior: " << std::fixed << std::setprecision(4) << limiteInferior_ << std::endl;
    ss << "  Limite superior: " << std::fixed << std::setprecision(4) << limiteSuperior_ << std::endl;
    return ss.str();
}

void DecomposicaoDantzigWolfe::inicializarColunas() {
    // Gerar algumas colunas iniciais para o problema mestre restrito
    // Cada coluna representa um conjunto de pedidos que poderiam ser agrupados
    
    // Para simplificar, vamos gerar algumas colunas baseadas em heurísticas simples
    
    // Coluna 1: Pedidos com maior quantidade de itens
    std::vector<std::pair<int, int>> pedidosComTotalItens;
    for (int i = 0; i < backlog_.numPedidos; i++) {
        int totalItens = 0;
        for (const auto& [itemId, quantidade] : backlog_.pedido[i]) {
            totalItens += quantidade;
        }
        pedidosComTotalItens.push_back({i, totalItens});
    }
    
    // Ordenar por total de itens (decrescente)
    std::sort(pedidosComTotalItens.begin(), pedidosComTotalItens.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Criar uma coluna com os top pedidos
    Coluna coluna1;
    int numPedidos = std::min(backlog_.wave.UB, (int)pedidosComTotalItens.size());
    numPedidos = std::max(numPedidos, backlog_.wave.LB);
    
    for (int i = 0; i < numPedidos; i++) {
        coluna1.pedidosIncluidos.push_back(pedidosComTotalItens[i].first);
    }
    
    // Calcular o custo (neste caso, é o inverso do nosso objetivo, pois estamos minimizando)
    // Nosso objetivo é maximizar unidades/corredores, então o custo é corredores/unidades
    
    // Calcular unidades totais
    int totalUnidades = 0;
    std::unordered_set<int> corredoresNecessarios;
    
    for (int pedidoId : coluna1.pedidosIncluidos) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
            
            // Adicionar corredores necessários
            const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
            for (const auto& [corredorId, qtd] : corredoresComItem) {
                if (qtd > 0) {
                    corredoresNecessarios.insert(corredorId);
                }
            }
        }
    }
    
    if (totalUnidades > 0 && !corredoresNecessarios.empty()) {
        coluna1.custo = static_cast<double>(corredoresNecessarios.size()) / totalUnidades;
        coluna1.valorPrimal = 0.0; // Será definido ao resolver o problema mestre
        colunas_.push_back(coluna1);
    }
    
    // Coluna 2: Pedidos com menor número de corredores
    // (Para diversificar as colunas iniciais)
    
    // Calcular o número de corredores por pedido
    std::vector<std::pair<int, int>> pedidosComCorredores;
    for (int i = 0; i < backlog_.numPedidos; i++) {
        std::unordered_set<int> corredores;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[i]) {
            const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
            for (const auto& [corredorId, qtd] : corredoresComItem) {
                if (qtd > 0) {
                    corredores.insert(corredorId);
                }
            }
        }
        
        pedidosComCorredores.push_back({i, static_cast<int>(corredores.size())});
    }
    
    // Ordenar por número de corredores (crescente)
    std::sort(pedidosComCorredores.begin(), pedidosComCorredores.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Criar uma coluna com os pedidos que utilizam menos corredores
    Coluna coluna2;
    numPedidos = std::min(backlog_.wave.UB, (int)pedidosComCorredores.size());
    numPedidos = std::max(numPedidos, backlog_.wave.LB);
    
    for (int i = 0; i < numPedidos; i++) {
        coluna2.pedidosIncluidos.push_back(pedidosComCorredores[i].first);
    }
    
    // Calcular o custo
    totalUnidades = 0;
    corredoresNecessarios.clear();
    
    for (int pedidoId : coluna2.pedidosIncluidos) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
            
            // Adicionar corredores necessários
            const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
            for (const auto& [corredorId, qtd] : corredoresComItem) {
                if (qtd > 0) {
                    corredoresNecessarios.insert(corredorId);
                }
            }
        }
    }
    
    if (totalUnidades > 0 && !corredoresNecessarios.empty()) {
        coluna2.custo = static_cast<double>(corredoresNecessarios.size()) / totalUnidades;
        coluna2.valorPrimal = 0.0; // Será definido ao resolver o problema mestre
        colunas_.push_back(coluna2);
    }
    
    // Aqui poderíamos adicionar mais colunas iniciais diversificadas
}

bool DecomposicaoDantzigWolfe::resolverProblemaMestreRestrito() {
    // Em uma implementação real, resolveríamos o problema mestre restrito
    // usando um solver de PL
    
    // Para simplificar, vamos simular uma solução para o problema mestre
    
    // Simulação: Atribuir valores primais às colunas
    // Na prática, esses valores viriam do solver
    
    // Se não temos colunas, não podemos resolver
    if (colunas_.empty()) {
        return false;
    }
    
    // Simulação: atribuir todo o peso para a coluna de menor custo
    auto melhorColuna = std::min_element(colunas_.begin(), colunas_.end(),
                                        [](const Coluna& a, const Coluna& b) {
                                            return a.custo < b.custo;
                                        });
    
    // Zerar todos os valores primais
    for (auto& coluna : colunas_) {
        coluna.valorPrimal = 0.0;
    }
    
    // Atribuir 1.0 à melhor coluna
    melhorColuna->valorPrimal = 1.0;
    
    // Atualizar o limite superior (valor do problema relaxado)
    // O valor objetivo é 1/custo, pois o custo é o inverso do nosso objetivo
    if (melhorColuna->custo > 0) {
        limiteSuperior_ = 1.0 / melhorColuna->custo;
    }
    
    return true;
}

bool DecomposicaoDantzigWolfe::gerarNovaColuna() {
    // Em uma implementação real, resolveríamos o subproblema
    // para encontrar uma coluna com custo reduzido negativo
    
    // Para simplificar, vamos simular a geração de uma nova coluna
    // baseada em uma heurística
    
    // Simulação: variáveis duais do problema mestre
    // Na prática, esses valores viriam do solver
    std::vector<double> variaveisDuais(backlog_.numPedidos, 0.1);
    
    // Calcular custos reduzidos para cada pedido
    std::vector<std::pair<int, double>> custosReduzidos;
    for (int i = 0; i < backlog_.numPedidos; i++) {
        double custoReduzido = calcularCustoReducido(variaveisDuais, i);
        custosReduzidos.push_back({i, custoReduzido});
    }
    
    // Ordenar por custo reduzido (crescente)
    std::sort(custosReduzidos.begin(), custosReduzidos.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Criar uma nova coluna com os pedidos de menor custo reduzido
    Coluna novaColuna;
    int numPedidos = std::min(backlog_.wave.UB, (int)custosReduzidos.size());
    numPedidos = std::max(numPedidos, backlog_.wave.LB);
    
    // Verificar se já temos uma coluna similar
    bool colunaUnica = true;
    
    for (int i = 0; i < numPedidos; i++) {
        novaColuna.pedidosIncluidos.push_back(custosReduzidos[i].first);
    }
    
    // Verificar se esta coluna é única
    for (const auto& coluna : colunas_) {
        if (coluna.pedidosIncluidos.size() == novaColuna.pedidosIncluidos.size()) {
            std::vector<int> coluna1 = coluna.pedidosIncluidos;
            std::vector<int> coluna2 = novaColuna.pedidosIncluidos;
            
            std::sort(coluna1.begin(), coluna1.end());
            std::sort(coluna2.begin(), coluna2.end());
            
            if (coluna1 == coluna2) {
                colunaUnica = false;
                break;
            }
        }
    }
    
    if (!colunaUnica) {
        // Se a coluna não for única, tentar gerar uma coluna aleatória
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, backlog_.numPedidos - 1);
        
        novaColuna.pedidosIncluidos.clear();
        std::unordered_set<int> pedidosSelecionados;
        
        while (pedidosSelecionados.size() < static_cast<size_t>(numPedidos)) {
            pedidosSelecionados.insert(dis(gen));
        }
        
        novaColuna.pedidosIncluidos.assign(pedidosSelecionados.begin(), pedidosSelecionados.end());
    }
    
    // Calcular o custo
    int totalUnidades = 0;
    std::unordered_set<int> corredoresNecessarios;
    
    for (int pedidoId : novaColuna.pedidosIncluidos) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
            
            // Adicionar corredores necessários
            const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
            for (const auto& [corredorId, qtd] : corredoresComItem) {
                if (qtd > 0) {
                    corredoresNecessarios.insert(corredorId);
                }
            }
        }
    }
    
    if (totalUnidades > 0 && !corredoresNecessarios.empty()) {
        novaColuna.custo = static_cast<double>(corredoresNecessarios.size()) / totalUnidades;
        novaColuna.valorPrimal = 0.0; // Será definido ao resolver o problema mestre
        
        // Verificar se a coluna tem custo reduzido negativo
        // (ou se é melhor que as colunas existentes)
        
        // Em uma implementação simples, vamos apenas verificar se é melhor
        // que a média das colunas existentes
        double custoMedio = 0.0;
        for (const auto& coluna : colunas_) {
            custoMedio += coluna.custo;
        }
        
        if (colunas_.empty() || novaColuna.custo < custoMedio / colunas_.size()) {
            adicionarColuna(novaColuna);
            return true;
        }
    }
    
    return false;
}

Solucao DecomposicaoDantzigWolfe::construirSolucaoFinal() {
    // Construir uma solução inteira a partir da solução do problema mestre
    
    // Em uma implementação real, precisaríamos resolver um problema de PLI
    // ou aplicar uma heurística de arredondamento
    
    // Para simplificar, vamos usar a coluna com maior valor primal
    Solucao solucao;
    
    auto melhorColuna = std::max_element(colunas_.begin(), colunas_.end(),
                                        [](const Coluna& a, const Coluna& b) {
                                            return a.valorPrimal < b.valorPrimal;
                                        });
    
    if (melhorColuna != colunas_.end() && melhorColuna->valorPrimal > 0) {
        // Usar a melhor coluna como solução
        solucao.pedidosWave = melhorColuna->pedidosIncluidos;
        
        // Calcular os corredores necessários
        std::unordered_set<int> corredoresNecessarios;
        int totalUnidades = 0;
        
        for (int pedidoId : solucao.pedidosWave) {
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
                totalUnidades += quantidade;
                
                // Adicionar corredores necessários
                const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
                for (const auto& [corredorId, qtd] : corredoresComItem) {
                    if (qtd > 0) {
                        corredoresNecessarios.insert(corredorId);
                    }
                }
            }
        }
        
        solucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
        
        // Calcular valor objetivo
        if (!corredoresNecessarios.empty()) {
            solucao.valorObjetivo = static_cast<double>(totalUnidades) / corredoresNecessarios.size();
        }
    }
    
    return solucao;
}

bool DecomposicaoDantzigWolfe::verificarConvergencia() {
    // Verificar se o algoritmo convergiu
    
    // Na prática, verificaríamos se não há mais colunas com custo reduzido negativo
    // ou se o gap entre os limites é pequeno o suficiente
    
    if (limiteSuperior_ == std::numeric_limits<double>::infinity() ||
        limiteInferior_ == -std::numeric_limits<double>::infinity()) {
        return false;
    }
    
    double gap = (limiteSuperior_ - limiteInferior_) / limiteSuperior_;
    return gap < tolerancia_;
}

double DecomposicaoDantzigWolfe::calcularCustoReducido(
    const std::vector<double>& variaveisDuais, int pedidoId) {
    // Em uma implementação real, calcularíamos o custo reduzido
    // baseado nas variáveis duais do problema mestre
    
    // Para simplificar, vamos usar uma heurística
    
    // Calcular o custo do pedido (corredores necessários)
    std::unordered_set<int> corredoresNecessarios;
    int totalUnidades = 0;
    
    for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
        totalUnidades += quantidade;
        
        // Adicionar corredores necessários
        const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
        for (const auto& [corredorId, qtd] : corredoresComItem) {
            if (qtd > 0) {
                corredoresNecessarios.insert(corredorId);
            }
        }
    }
    
    double custo = 0.0;
    if (totalUnidades > 0 && !corredoresNecessarios.empty()) {
        custo = static_cast<double>(corredoresNecessarios.size()) / totalUnidades;
    } else {
        custo = std::numeric_limits<double>::max();
    }
    
    // Subtrair o valor dual (simulado)
    return custo - variaveisDuais[pedidoId];
}

void DecomposicaoDantzigWolfe::adicionarColuna(const Coluna& coluna) {
    colunas_.push_back(coluna);
}