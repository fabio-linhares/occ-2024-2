#include "decomposicao_benders.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unordered_set>

DecomposicaoBenders::DecomposicaoBenders(
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
    limiteSuperior_(std::numeric_limits<double>::infinity()),
    limiteInferior_(-std::numeric_limits<double>::infinity()),
    iteracoesRealizadas_(0),
    tempoTotal_(0.0),
    gap_(0.0)
{
    // Inicializar a melhor solução com valores vazios
    melhorSolucao_.valorObjetivo = 0.0;
    
    // Construir os modelos para o problema mestre e subproblema
    construirProblemaMestre();
    construirSubproblema();
}

Solucao DecomposicaoBenders::resolver() {
    auto inicioExecucao = std::chrono::high_resolution_clock::now();
    
    int iteracao = 0;
    bool convergiu = false;
    
    std::cout << "Iniciando algoritmo de decomposição de Benders..." << std::endl;
    
    while (!convergiu && iteracao < maxIteracoes_) {
        iteracao++;
        std::cout << "Iteração " << iteracao << " de Benders:" << std::endl;
        
        // 1. Resolver o problema mestre (selecionar pedidos)
        Solucao solucaoAtual = resolverProblemaMestre();
        
        // 2. Resolver o subproblema (calcular corredores necessários)
        auto [valorSubproblema, novoCorteBenders] = resolverSubproblema(solucaoAtual.pedidosWave);
        
        // 3. Atualizar limites
        double valorObjetivoAtual = solucaoAtual.valorObjetivo;
        limiteInferior_ = std::max(limiteInferior_, valorObjetivoAtual);
        
        if (valorSubproblema < std::numeric_limits<double>::infinity()) {
            limiteSuperior_ = std::min(limiteSuperior_, valorSubproblema);
        }
        
        // 4. Verificar se encontramos uma solução melhor
        if (valorSubproblema > melhorSolucao_.valorObjetivo) {
            melhorSolucao_ = solucaoAtual;
            melhorSolucao_.valorObjetivo = valorSubproblema;
        }
        
        // 5. Adicionar o corte ao problema mestre
        adicionarCorte(novoCorteBenders);
        
        // 6. Verificar convergência
        convergiu = verificarConvergencia();
        
        // 7. Verificar limite de tempo
        auto tempoAtual = std::chrono::high_resolution_clock::now();
        double segundosDecorridos = std::chrono::duration<double>(tempoAtual - inicioExecucao).count();
        if (segundosDecorridos > limiteTempo_) {
            std::cout << "Limite de tempo atingido após " << segundosDecorridos << " segundos" << std::endl;
            break;
        }
        
        // Calcular o gap atual
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
    
    // Registrar estatísticas finais
    iteracoesRealizadas_ = iteracao;
    auto fimExecucao = std::chrono::high_resolution_clock::now();
    tempoTotal_ = std::chrono::duration<double>(fimExecucao - inicioExecucao).count();
    
    std::cout << "Algoritmo de Benders concluído em " << tempoTotal_ << " segundos" << std::endl;
    std::cout << "Iterações realizadas: " << iteracoesRealizadas_ << std::endl;
    std::cout << "Gap final: " << gap_ << "%" << std::endl;
    std::cout << "Valor objetivo: " << melhorSolucao_.valorObjetivo << std::endl;
    
    return melhorSolucao_;
}

std::string DecomposicaoBenders::obterEstatisticas() const {
    std::stringstream ss;
    ss << "Estatísticas da Decomposição de Benders:" << std::endl;
    ss << "  Iterações realizadas: " << iteracoesRealizadas_ << std::endl;
    ss << "  Tempo total: " << std::fixed << std::setprecision(2) << tempoTotal_ << " segundos" << std::endl;
    ss << "  Gap final: " << std::fixed << std::setprecision(2) << gap_ << "%" << std::endl;
    ss << "  Limite inferior: " << std::fixed << std::setprecision(4) << limiteInferior_ << std::endl;
    ss << "  Limite superior: " << std::fixed << std::setprecision(4) << limiteSuperior_ << std::endl;
    ss << "  Cortes gerados: " << cortes_.size() << std::endl;
    return ss.str();
}

Solucao DecomposicaoBenders::resolverProblemaMestre() {
    // Aqui implementaríamos a resolução do problema mestre
    // usando técnicas de PLI para selecionar os pedidos
    
    // Para uma implementação inicial, podemos usar uma heurística
    // ou um solver PLI simplificado
    
    // Exemplo simplificado (heurístico):
    Solucao solucao;
    
    // Selecionar pedidos baseados em alguma heurística
    // Como estamos no início, vamos selecionar pedidos com maior quantidade de itens
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
    
    // Selecionar até o limite UB
    int numPedidosSelecionados = std::min(backlog_.wave.UB, (int)pedidosComTotalItens.size());
    numPedidosSelecionados = std::max(numPedidosSelecionados, backlog_.wave.LB);
    
    for (int i = 0; i < numPedidosSelecionados; i++) {
        solucao.pedidosWave.push_back(pedidosComTotalItens[i].first);
    }
    
    // Neste ponto, precisamos calcular os corredores necessários
    // Isso seria feito no subproblema, mas para termos um valor aqui:
    std::unordered_set<int> corredoresNecessarios;
    
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
            // Adicionar o primeiro corredor que contém o item
            if (!corredoresComItem.empty()) {
                corredoresNecessarios.insert(corredoresComItem.begin()->first);
            }
        }
    }
    
    solucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
    
    // Calcular valor objetivo
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    solucao.valorObjetivo = totalUnidades / static_cast<double>(solucao.corredoresWave.size());
    
    return solucao;
}

std::pair<double, DecomposicaoBenders::Corte> DecomposicaoBenders::resolverSubproblema(
    const std::vector<int>& pedidosSelecionados) {
    // Aqui implementaríamos a resolução do subproblema dual
    // para gerar um corte de Benders
    
    // Para uma implementação inicial, vamos usar uma abordagem simplificada
    // que calcula a solução ótima para a alocação de corredores
    // dado o conjunto de pedidos selecionados
    
    // 1. Determinar os corredores necessários
    std::unordered_set<int> corredoresNecessarios;
    std::unordered_set<int> itensNecessarios;
    
    for (int pedidoId : pedidosSelecionados) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            itensNecessarios.insert(itemId);
        }
    }
    
    // 2. Encontrar os corredores que contêm esses itens
    for (int itemId : itensNecessarios) {
        const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
        for (const auto& [corredorId, quantidade] : corredoresComItem) {
            if (quantidade > 0) {
                corredoresNecessarios.insert(corredorId);
            }
        }
    }
    
    // 3. Calcular o valor objetivo
    int totalUnidades = 0;
    for (int pedidoId : pedidosSelecionados) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    double valorObjetivo = 0.0;
    if (!corredoresNecessarios.empty()) {
        valorObjetivo = totalUnidades / static_cast<double>(corredoresNecessarios.size());
    }
    
    // 4. Gerar o corte de Benders
    Corte corte;
    corte.termo_independente = valorObjetivo;
    corte.coeficientes.resize(backlog_.numPedidos, 0.0);
    
    // Calculando os coeficientes (simplificado)
    // Em uma implementação completa, esses coeficientes viriam 
    // do problema dual do subproblema
    for (int pedidoId : pedidosSelecionados) {
        corte.coeficientes[pedidoId] = valorObjetivo / pedidosSelecionados.size();
    }
    
    return {valorObjetivo, corte};
}

void DecomposicaoBenders::adicionarCorte(const Corte& corte) {
    cortes_.push_back(corte);
    // Em uma implementação real, aqui atualizaríamos o modelo do problema mestre
    // para incluir o novo corte
}

bool DecomposicaoBenders::verificarConvergencia() {
    // Se os limites estão próximos o suficiente, consideramos que convergiu
    if (limiteSuperior_ == std::numeric_limits<double>::infinity() ||
        limiteInferior_ == -std::numeric_limits<double>::infinity()) {
        return false;
    }
    
    double gap = (limiteSuperior_ - limiteInferior_) / limiteSuperior_;
    return gap < tolerancia_;
}

void DecomposicaoBenders::construirProblemaMestre() {
    // Neste método, construiríamos o modelo para o problema mestre
    // usando um solver de PLI
    
    // Como estamos em uma implementação inicial, vamos apenas
    // inicializar quaisquer estruturas necessárias
}

void DecomposicaoBenders::construirSubproblema() {
    // Neste método, construiríamos o modelo para o subproblema
    // usando um solver de PL
    
    // Como estamos em uma implementação inicial, vamos apenas
    // inicializar quaisquer estruturas necessárias
}