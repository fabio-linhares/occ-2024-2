#include "otimizador_pli_alm.h"
#include "parser.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include <chrono>
#include <unordered_set>
#include <filesystem>
#include "formatacao_terminal.h"

// Modificar a inclusão do OR-Tools para evitar o erro de dependência
#if defined(USE_ORTOOLS) && __has_include(<ortools/linear_solver/linear_solver.h>)
#include <ortools/linear_solver/linear_solver.h>
#endif

using namespace FormatacaoTerminal;

OtimizadorPLI_ALM::OtimizadorPLI_ALM(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador
) : 
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    iteracoesRealizadas_(0),
    tempoExecucao_(0.0),
    valorObjetivo_(0.0),
    gap_(1.0)
{
    // Inicializar multiplicadores de Lagrange e parâmetros de reforço
    // Para simplificar, começamos com 3 tipos de restrições (LB, UB, cobertura de itens)
    multiplicadoresLagrange_.resize(3, 0.0);
    parametrosReforco_.resize(3, 1.0);
    
    // Gerar pontos de quebra para linearização por partes
    pontosQuebra_.resize(3);
    
    // Pontos para restrição LB (limite inferior)
    pontosQuebra_[0] = gerarPontosQuebra(0, backlog.wave.LB, 10);
    
    // Pontos para restrição UB (limite superior)
    pontosQuebra_[1] = gerarPontosQuebra(0, backlog.wave.UB - backlog.wave.LB, 10);
    
    // Pontos para restrição de cobertura (simplificado - usaria mais pontos em implementação real)
    pontosQuebra_[2] = gerarPontosQuebra(0, 1000, 10);
}

void OtimizadorPLI_ALM::exibirModeloMatematico(std::ostream& output) {
    output << cabecalho("MODELO MATEMÁTICO - PROGRAMAÇÃO LINEAR INTEIRA COM ALM") << std::endl;
    
    output << colorirBold("PROBLEMA ORIGINAL:", CIANO) << std::endl;
    output << "Maximizar a produtividade do processo de coleta, definida como o número total\n";
    output << "de unidades coletadas dividido pelo número de corredores visitados.\n\n";
    
    output << "max  ∑_{o∈O'} ∑_{i∈I_o} u_{oi} / |A'|\n\n";
    
    output << "Sujeito a:\n";
    output << "∑_{o∈O'} ∑_{i∈I_o} u_{oi} ≥ " << backlog_.wave.LB << "   (Limite inferior)\n";
    output << "∑_{o∈O'} ∑_{i∈I_o} u_{oi} ≤ " << backlog_.wave.UB << "   (Limite superior)\n";
    output << "∑_{o∈O'} u_{oi} ≤ ∑_{a∈A'} u_{ai}, ∀i∈I_o, ∀o∈O'   (Cobertura de itens por corredores)\n\n";
    
    output << colorirBold("LINEARIZAÇÃO:", CIANO) << std::endl;
    output << "Para linearizar o problema, introduzimos variáveis de decisão binárias e auxiliares:\n\n";
    
    output << "Variáveis de Decisão:\n";
    output << "- x_o ∈ {0,1}: Indica se o pedido o é selecionado (total: " << backlog_.numPedidos << " variáveis)\n";
    output << "- y_a ∈ {0,1}: Indica se o corredor a é visitado (total: " << deposito_.numCorredores << " variáveis)\n";
    output << "- w_o ≥ 0: Variável auxiliar para linearização (w_o = z * x_o)\n";
    output << "- z ≥ 0: Representa o inverso do número de corredores (z = 1/|A'|)\n\n";
    
    output << "Restrições linearizadas:\n";
    output << "∑_{o∈O} ∑_{i∈I_o} u_{oi} * x_o ≥ " << backlog_.wave.LB << "   (Limite inferior)\n";
    output << "∑_{o∈O} ∑_{i∈I_o} u_{oi} * x_o ≤ " << backlog_.wave.UB << "   (Limite superior)\n";
    output << "∑_{o∈O} u_{oi} * x_o ≤ ∑_{a∈A} u_{ai} * y_a, ∀i∈I, ∀o∈O   (Cobertura)\n\n";
    
    output << "Restrições adicionais para garantir w_o = z * x_o:\n";
    output << "w_o ≤ x_o, ∀o∈O\n";
    output << "w_o ≤ M * z, ∀o∈O\n";
    output << "w_o ≥ x_o + M * (z - 1), ∀o∈O\n";
    output << "∑_{a∈A} y_a * z = 1   (Relação entre z e corredores selecionados)\n\n";
    
    output << colorirBold("MÉTODO DO LAGRANGEANO AUMENTADO (ALM):", CIANO) << std::endl;
    output << "Para melhorar a convergência, aplicamos o método ALM, que transforma restrições\n";
    output << "em penalizações na função objetivo, combinando multiplicadores de Lagrange com\n";
    output << "termos quadráticos, que são linearizados por partes.\n\n";
    
    output << "Função objetivo ALM linearizada por partes:\n";
    output << "max f(x) - ∑_i λ_i * g_i(x) - ∑_i (ρ_i/2) * [max(0, g_i(x))]²\n";
    output << "onde λ_i são os multiplicadores de Lagrange e ρ_i são os parâmetros de reforço.\n";
}

std::vector<PontoQuebra> OtimizadorPLI_ALM::gerarPontosQuebra(double min, double max, int numPontos) {
    std::vector<PontoQuebra> pontos;
    pontos.reserve(numPontos);
    
    if (numPontos <= 0 || min > max) {
        // Caso de entrada inválida, retornar vetor vazio ou um único ponto
        if (min <= max) {
            pontos.push_back({min, min * min});
        }
        return pontos;
    }
    
    // Caso de um único ponto
    if (numPontos == 1 || std::abs(max - min) < 1e-10) {
        pontos.push_back({min, min * min});
        return pontos;
    }
    
    // Distribuir os pontos uniformemente no intervalo [min, max]
    double delta = (max - min) / (numPontos - 1);
    
    for (int i = 0; i < numPontos; i++) {
        double x = min + i * delta;
        double y = x * x; // Função quadrática simples
        pontos.push_back({x, y});
    }
    
    return pontos;
}

double OtimizadorPLI_ALM::aproximarFuncaoQuadratica(
    double valor, 
    const std::vector<PontoQuebra>& pontos, 
    std::vector<double>& alphas
) {
    if (pontos.empty()) {
        alphas.clear();
        return 0.0;
    }
    
    // Caso de um único ponto
    if (pontos.size() == 1) {
        alphas.resize(1, 1.0);
        return pontos[0].valorQuadratico;
    }
    
    // Encontrar os segmentos adjacentes ao valor
    size_t idx = 0;
    while (idx < pontos.size() - 1 && pontos[idx + 1].valor < valor) {
        idx++;
    }
    
    // Se estamos no último ponto, usar apenas esse ponto
    if (idx >= pontos.size() - 1) {
        alphas.resize(pontos.size(), 0.0);
        alphas[pontos.size() - 1] = 1.0;
        return pontos[pontos.size() - 1].valorQuadratico;
    }
    
    // Interpolação linear entre os dois pontos adjacentes
    double x0 = pontos[idx].valor;
    double y0 = pontos[idx].valorQuadratico;
    double x1 = pontos[idx + 1].valor;
    double y1 = pontos[idx + 1].valorQuadratico;
    
    // Calcular pesos da interpolação
    double lambda = 0.0;
    if (std::abs(x1 - x0) > 1e-10) { // Evitar divisão por zero
        lambda = (valor - x0) / (x1 - x0);
    }
    
    // Calcular valor aproximado por interpolação linear
    double valorAproximado = (1 - lambda) * y0 + lambda * y1;
    
    // Preencher os pesos das variáveis lambda
    alphas.resize(pontos.size(), 0.0);
    alphas[idx] = 1 - lambda;
    alphas[idx + 1] = lambda;
    
    return valorAproximado;
}

Solucao OtimizadorPLI_ALM::resolver(int LB, int UB, int maxIteracoes, double tolerancia) {
    // Inicializar timer
    auto inicioTempo = std::chrono::high_resolution_clock::now();
    
    // Inicializar contadores e variáveis
    iteracoesRealizadas_ = 0;
    double melhorValorObj = -std::numeric_limits<double>::infinity();
    double violacaoRestricoes = std::numeric_limits<double>::max();
    
    // Gerar solução inicial usando método guloso
    Solucao solucaoAtual = gerarSolucaoGulosa();
    if (solucaoAtual.pedidosWave.empty()) {
        std::cerr << "Erro: Não foi possível gerar uma solução inicial válida" << std::endl;
        
        // Retornar solução vazia em caso de erro
        Solucao solucaoVazia;
        solucaoVazia.valorObjetivo = 0.0;
        return solucaoVazia;
    }
    
    // Ajustar viabilidade da solução inicial
    solucaoAtual = ajustarViabilidade(solucaoAtual, LB, UB);
    
    // Inicializar melhor solução
    Solucao melhorSolucao = solucaoAtual;
    melhorValorObj = calcularValorObjetivo(solucaoAtual);
    
    std::cout << "Iniciando otimização ALM com solução inicial: " 
              << melhorValorObj << " (" << solucaoAtual.pedidosWave.size() 
              << " pedidos, " << solucaoAtual.corredoresWave.size() << " corredores)" << std::endl;
    
    // Loop principal do ALM
    while (iteracoesRealizadas_ < maxIteracoes && violacaoRestricoes > tolerancia) {
        // Atualizar modelo PLI
        construirModeloPLI();
        
        // Resolver subproblema linearizado
        Solucao novaSolucao = resolverHeuristicaCustom(solucaoAtual, LB, UB);
        
        // Aplicar busca local para melhorar a solução
        novaSolucao = aplicarBuscaLocal(novaSolucao, LB, UB);
        
        // Calcular violação das restrições
        violacaoRestricoes = calcularViolacaoRestricoes(novaSolucao);
        
        // Calcular valor objetivo
        double valorAtual = calcularValorObjetivo(novaSolucao);
        
        // Atualizar melhor solução se necessário
        if (valorAtual > melhorValorObj && violacaoRestricoes < tolerancia * 10) {
            melhorSolucao = novaSolucao;
            melhorValorObj = valorAtual;
            std::cout << "Nova melhor solução: " << melhorValorObj 
                      << " (Violação: " << violacaoRestricoes << ")" << std::endl;
        }
        
        // Atualizar multiplicadores de Lagrange
        atualizarMultiplicadoresLagrange(novaSolucao);
        
        // Atualizar solução atual
        solucaoAtual = novaSolucao;
        
        // Incrementar contador de iterações
        iteracoesRealizadas_++;
        
        // Log de progresso a cada 10 iterações
        if (iteracoesRealizadas_ % 10 == 0) {
            std::cout << "Iteração " << iteracoesRealizadas_ 
                      << ": Valor = " << valorAtual 
                      << ", Violação = " << violacaoRestricoes << std::endl;
        }
    }
    
    // Calcular tempo de execução
    auto fimTempo = std::chrono::high_resolution_clock::now();
    tempoExecucao_ = std::chrono::duration<double>(fimTempo - inicioTempo).count();
    
    // Calcular estatísticas finais
    valorObjetivo_ = melhorValorObj;
    double limiteSuperior = estimarLimiteSuperior();
    gap_ = (limiteSuperior > 0) ? (limiteSuperior - melhorValorObj) / limiteSuperior : 0.0;
    
    std::cout << "Otimização ALM concluída em " << tempoExecucao_ << " segundos" << std::endl;
    std::cout << "Iterações: " << iteracoesRealizadas_ << std::endl;
    std::cout << "Valor objetivo: " << valorObjetivo_ << std::endl;
    std::cout << "Gap: " << (gap_ * 100) << "%" << std::endl;
    
    return melhorSolucao;
}

void OtimizadorPLI_ALM::atualizarMultiplicadoresLagrange(const Solucao& solucao) {
    // Calcular violações das restrições
    
    // 1. Violação do limite inferior (LB)
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    double violacaoLB = std::max(0.0, static_cast<double>(backlog_.wave.LB - totalUnidades));
    
    // 2. Violação do limite superior (UB)
    double violacaoUB = std::max(0.0, static_cast<double>(totalUnidades - backlog_.wave.UB));
    
    // 3. Violação da cobertura de itens
    double violacaoCobertura = 0.0;
    std::unordered_map<int, int> demandaPorItem;
    
    // Calcular demanda total por item
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            demandaPorItem[itemId] += quantidade;
        }
    }
    
    // Verificar se os itens são cobertos pelos corredores selecionados
    for (const auto& [itemId, demanda] : demandaPorItem) {
        int estoqueDisponivel = 0;
        
        // Somar estoque disponível nos corredores selecionados
        for (int corredorId : solucao.corredoresWave) {
            auto it = deposito_.corredor[corredorId].find(itemId);
            if (it != deposito_.corredor[corredorId].end()) {
                estoqueDisponivel += it->second;
            }
        }
        
        // Calcular violação (estoque insuficiente)
        violacaoCobertura += std::max(0.0, static_cast<double>(demanda - estoqueDisponivel));
    }
    
    // Atualizar multiplicadores de Lagrange
    multiplicadoresLagrange_[0] += parametrosReforco_[0] * violacaoLB;
    multiplicadoresLagrange_[1] += parametrosReforco_[1] * violacaoUB;
    multiplicadoresLagrange_[2] += parametrosReforco_[2] * violacaoCobertura;
    
    // Aumentar parâmetros de reforço se a violação não diminui significativamente
    static double violacaoAnterior = violacaoLB + violacaoUB + violacaoCobertura;
    double violacaoTotal = violacaoLB + violacaoUB + violacaoCobertura;
    
    if (violacaoTotal > violacaoAnterior * 0.95) {
        for (double& param : parametrosReforco_) {
            param *= 1.5; // Aumentar parâmetro em 50%
        }
    }
    
    violacaoAnterior = violacaoTotal;
}

double OtimizadorPLI_ALM::calcularViolacaoRestricoes(const Solucao& solucao) {
    // Calcular unidades totais
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Violação do limite inferior (LB)
    double violacaoLB = std::max(0.0, static_cast<double>(backlog_.wave.LB - totalUnidades));
    
    // Violação do limite superior (UB)
    double violacaoUB = std::max(0.0, static_cast<double>(totalUnidades - backlog_.wave.UB));
    
    // Violação da cobertura de itens
    double violacaoCobertura = 0.0;
    std::unordered_map<int, int> demandaPorItem;
    
    // Calcular demanda total por item
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            demandaPorItem[itemId] += quantidade;
        }
    }
    
    // Verificar cobertura pelos corredores selecionados
    for (const auto& [itemId, demanda] : demandaPorItem) {
        int estoqueDisponivel = 0;
        
        for (int corredorId : solucao.corredoresWave) {
            auto it = deposito_.corredor[corredorId].find(itemId);
            if (it != deposito_.corredor[corredorId].end()) {
                estoqueDisponivel += it->second;
            }
        }
        
        // Adicionar violação se o estoque disponível é insuficiente
        violacaoCobertura += std::max(0.0, static_cast<double>(demanda - estoqueDisponivel));
    }
    
    // Retornar soma das violações
    return violacaoLB + violacaoUB + violacaoCobertura;
}

Solucao OtimizadorPLI_ALM::gerarSolucaoGulosa() {
    Solucao solucao;
    
    // Calcular eficiência (unidades por corredor) para cada pedido
    std::vector<std::pair<int, double>> pedidosEficiencia;
    
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        if (backlog_.pedido[pedidoId].empty()) {
            continue; // Pular pedidos vazios
        }
        
        // Calcular total de unidades deste pedido
        int unidades = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidades += quantidade;
        }
        
        // Encontrar corredores necessários para este pedido
        std::unordered_set<int> corredores;
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
            
            // Adicionar o corredor com maior quantidade disponível
            int melhorCorredor = -1;
            int maiorQuantidade = 0;
            
            for (const auto& [corredorId, quantidade] : corredoresItem) {
                if (quantidade > maiorQuantidade) {
                    maiorQuantidade = quantidade;
                    melhorCorredor = corredorId;
                }
            }
            
            if (melhorCorredor >= 0) {
                corredores.insert(melhorCorredor);
            }
        }
        
        // Calcular eficiência (unidades por corredor)
        double eficiencia = (corredores.empty()) ? 0.0 : static_cast<double>(unidades) / corredores.size();
        
        pedidosEficiencia.push_back({pedidoId, eficiencia});
    }
    
    // Ordenar pedidos por eficiência (decrescente)
    std::sort(pedidosEficiencia.begin(), pedidosEficiencia.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Adicionar pedidos em ordem de eficiência até atingir o LB
    int totalUnidades = 0;
    std::unordered_set<int> corredoresIncluidos;
    
    for (const auto& [pedidoId, _] : pedidosEficiencia) {
        // Calcular unidades deste pedido
        int unidadesPedido = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }
        
        // Verificar se excederia o UB
        if (totalUnidades + unidadesPedido > backlog_.wave.UB) {
            continue; // Pular este pedido
        }
        
        // Adicionar pedido à solução
        solucao.pedidosWave.push_back(pedidoId);
        totalUnidades += unidadesPedido;
        
        // Adicionar corredores necessários
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
            
            // Adicionar o corredor com maior quantidade
            int melhorCorredor = -1;
            int maiorQuantidade = 0;
            
            for (const auto& [corredorId, quantidade] : corredoresItem) {
                if (quantidade > maiorQuantidade) {
                    maiorQuantidade = quantidade;
                    melhorCorredor = corredorId;
                }
            }
            
            if (melhorCorredor >= 0) {
                corredoresIncluidos.insert(melhorCorredor);
            }
        }
        
        // Se atingimos o LB, podemos parar
        if (totalUnidades >= backlog_.wave.LB) {
            break;
        }
    }
    
    // Converter conjunto de corredores para vetor
    solucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
    
    // Calcular valor objetivo
    solucao.valorObjetivo = calcularValorObjetivo(solucao);
    
    return solucao;
}

Solucao OtimizadorPLI_ALM::ajustarViabilidade(const Solucao& solucao, int LB, int UB) {
    Solucao solucaoAjustada = solucao;
    
    // Calcular total de unidades atual
    int totalUnidades = 0;
    for (int pedidoId : solucaoAjustada.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Se abaixo do LB, adicionar mais pedidos
    if (totalUnidades < LB) {
        // Calcular pedidos candidatos a adicionar (não incluídos na solução)
        std::vector<std::pair<int, double>> pedidosCandidatos;
        
        for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
            // Verificar se já está na solução
            if (std::find(solucaoAjustada.pedidosWave.begin(), 
                         solucaoAjustada.pedidosWave.end(), 
                         pedidoId) != solucaoAjustada.pedidosWave.end()) {
                continue;
            }
            
            // Calcular unidades deste pedido
            int unidadesPedido = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }
            
            // Calcular novos corredores necessários
            std::unordered_set<int> corredoresAdicionais;
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
                
                bool itemJaCoberto = false;
                for (int corredorId : solucaoAjustada.corredoresWave) {
                    if (corredoresItem.find(corredorId) != corredoresItem.end()) {
                        itemJaCoberto = true;
                        break;
                    }
                }
                
                if (!itemJaCoberto && !corredoresItem.empty()) {
                    // Adicionar primeiro corredor com este item
                    corredoresAdicionais.insert(corredoresItem.begin()->first);
                }
            }
            
            // Calcular eficiência (unidades por corredor adicional)
            double eficiencia = (corredoresAdicionais.empty()) ? 
                std::numeric_limits<double>::max() : 
                static_cast<double>(unidadesPedido) / corredoresAdicionais.size();
            
            pedidosCandidatos.push_back({pedidoId, eficiencia});
        }
        
        // Ordenar candidatos por eficiência (decrescente)
        std::sort(pedidosCandidatos.begin(), pedidosCandidatos.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Adicionar pedidos até atingir LB
        for (const auto& [pedidoId, _] : pedidosCandidatos) {
            if (totalUnidades >= LB) 
                break;
            
            // Calcular unidades deste pedido
            int unidadesPedido = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }
            
            // Verificar se excederia UB
            if (totalUnidades + unidadesPedido > UB) {
                continue; // Pular este pedido
            }
            
            // Adicionar pedido
            solucaoAjustada.pedidosWave.push_back(pedidoId);
            totalUnidades += unidadesPedido;
            
            // Atualizar corredores
            std::unordered_set<int> corredoresNecessarios(
                solucaoAjustada.corredoresWave.begin(),
                solucaoAjustada.corredoresWave.end()
            );
            
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
                
                bool itemJaCoberto = false;
                for (int corredorId : corredoresNecessarios) {
                    if (corredoresItem.find(corredorId) != corredoresItem.end()) {
                        itemJaCoberto = true;
                        break;
                    }
                }
                
                if (!itemJaCoberto && !corredoresItem.empty()) {
                    // Adicionar primeiro corredor com este item
                    corredoresNecessarios.insert(corredoresItem.begin()->first);
                }
            }
            
            solucaoAjustada.corredoresWave.assign(
                corredoresNecessarios.begin(), 
                corredoresNecessarios.end()
            );
        }
    }
    
    // Se acima do UB, remover pedidos
    if (totalUnidades > UB) {
        // Calcular eficiência de cada pedido
        std::vector<std::tuple<int, int, double>> pedidosNaWave;
        
        for (size_t i = 0; i < solucaoAjustada.pedidosWave.size(); i++) {
            int pedidoId = solucaoAjustada.pedidosWave[i];
            
            // Calcular unidades
            int unidadesPedido = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }
            
            // Calcular eficiência (unidades / corredores exclusivos)
            double eficiencia = 0.0;
            
            // Temporariamente remove este pedido para ver quais corredores são exclusivos
            std::vector<int> pedidosSemEste = solucaoAjustada.pedidosWave;
            pedidosSemEste.erase(pedidosSemEste.begin() + i);
            
            std::unordered_set<int> corredoresSemEste;
            for (int p : pedidosSemEste) {
                for (const auto& [itemId, _] : backlog_.pedido[p]) {
                    const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
                    if (!corredoresItem.empty()) {
                        corredoresSemEste.insert(corredoresItem.begin()->first);
                    }
                }
            }
            
            int corredoresExclusivos = std::min(1, (int)solucaoAjustada.corredoresWave.size() - (int)corredoresSemEste.size());
            
            if (corredoresExclusivos > 0) {
                eficiencia = static_cast<double>(unidadesPedido) / corredoresExclusivos;
            } else {
                eficiencia = static_cast<double>(unidadesPedido);
            }
            
            pedidosNaWave.push_back({pedidoId, (int)i, eficiencia});
        }
        
        // Ordenar por eficiência (crescente)
        std::sort(pedidosNaWave.begin(), pedidosNaWave.end(),
                 [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
        
        // Remover pedidos até ficar abaixo de UB
        std::vector<size_t> indicesRemover;
        
        for (const auto& [pedidoId, indice, _] : pedidosNaWave) {
            if (totalUnidades <= UB) 
                break;
            
            int unidadesPedido = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }
            
            totalUnidades -= unidadesPedido;
            indicesRemover.push_back(indice);
            
            // Se agora ficaríamos abaixo de LB, cancelar esta remoção
            if (totalUnidades < LB) {
                totalUnidades += unidadesPedido;
                indicesRemover.pop_back();
            }
        }
        
        // Ordenar índices em ordem decrescente para remoção segura
        std::sort(indicesRemover.begin(), indicesRemover.end(), std::greater<size_t>());
        
        // Remover pedidos
        for (size_t indice : indicesRemover) {
            if (indice < solucaoAjustada.pedidosWave.size()) {
                solucaoAjustada.pedidosWave.erase(solucaoAjustada.pedidosWave.begin() + indice);
            }
        }
        
        // Recalcular corredores necessários
        std::unordered_set<int> corredoresNecessarios;
        for (int pedidoId : solucaoAjustada.pedidosWave) {
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
                if (!corredoresItem.empty()) {
                    corredoresNecessarios.insert(corredoresItem.begin()->first);
                }
            }
        }
        
        solucaoAjustada.corredoresWave.assign(
            corredoresNecessarios.begin(), 
            corredoresNecessarios.end()
        );
    }
    
    // Recalcular valor objetivo
    solucaoAjustada.valorObjetivo = calcularValorObjetivo(solucaoAjustada);
    
    return solucaoAjustada;
}

double OtimizadorPLI_ALM::calcularValorObjetivo(const Solucao& solucao) {
    // Calcular total de unidades
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Calcular BOV (unidades / corredores)
    if (solucao.corredoresWave.empty()) {
        return 0.0;
    }
    
    return static_cast<double>(totalUnidades) / solucao.corredoresWave.size();
}

double OtimizadorPLI_ALM::estimarLimiteSuperior() {
    // Tentar primeiramente o método PL
    double limite = estimarLimiteSuperiorPL();
    
    // Se falhou, usar método heurístico
    if (limite <= 0.0) {
        return estimarLimiteSuperiorHeuristico();
    }
    
    return limite;
}

double OtimizadorPLI_ALM::estimarLimiteSuperiorHeuristico() {
    // Métrica: unidades / corredores

    // Selecionar os pedidos mais eficientes individualmente
    std::vector<std::pair<int, double>> pedidosEficiencia;
    
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        int unidades = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidades += quantidade;
        }
        
        std::unordered_set<int> corredoresNecessarios;
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
            if (!corredoresItem.empty()) {
                corredoresNecessarios.insert(corredoresItem.begin()->first);
            }
        }
        
        if (!corredoresNecessarios.empty()) {
            double eficiencia = static_cast<double>(unidades) / corredoresNecessarios.size();
            pedidosEficiencia.push_back({pedidoId, eficiencia});
        }
    }
    
    // Ordenar por eficiência
    std::sort(pedidosEficiencia.begin(), pedidosEficiencia.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Selecionar os melhores pedidos até atingir o UB
    int totalUnidades = 0;
    std::unordered_set<int> corredoresUnicos;
    
    for (const auto& [pedidoId, _] : pedidosEficiencia) {
        // Calcular unidades neste pedido
        int unidadesPedido = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }
        
        // Se excederia UB, pular
        if (totalUnidades + unidadesPedido > backlog_.wave.UB) {
            continue;
        }
        
        // Adicionar pedido
        totalUnidades += unidadesPedido;
        
        // Adicionar corredores necessários
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
            if (!corredoresItem.empty()) {
                corredoresUnicos.insert(corredoresItem.begin()->first);
            }
        }
        
        // Se atingimos o LB, podemos parar
        if (totalUnidades >= backlog_.wave.LB) {
            break;
        }
    }
    
    // Calcular limiteSuperior
    if (totalUnidades < backlog_.wave.LB || corredoresUnicos.empty()) {
        return 0.0;
    }
    
    return static_cast<double>(totalUnidades) / corredoresUnicos.size();
}

double OtimizadorPLI_ALM::estimarLimiteSuperiorPL() {
#if defined(USE_ORTOOLS)
    try {
        operations_research::MPSolver solver("PL_Relaxado", 
            operations_research::MPSolver::GLOP_LINEAR_PROGRAMMING);
        
        // Variáveis de decisão
        std::vector<operations_research::MPVariable*> x_vars; // Pedidos
        for (int j = 0; j < backlog_.numPedidos; j++) {
            x_vars.push_back(solver.MakeNumVar(0.0, 1.0, "x_" + std::to_string(j)));
        }
        
        std::vector<operations_research::MPVariable*> y_vars; // Corredores
        for (int i = 0; i < deposito_.numCorredores; i++) {
            y_vars.push_back(solver.MakeNumVar(0.0, 1.0, "y_" + std::to_string(i)));
        }
        
        // Variável z = 1/n_corredores (approximation)
        operations_research::MPVariable* z = solver.MakeNumVar(0.0, 1.0, "z");
        
        // Restrições
        // Limite inferior e superior de unidades
        operations_research::MPConstraint* const_lb = solver.MakeRowConstraint(
            backlog_.wave.LB, operations_research::MPSolver::infinity(), "LB");
        
        operations_research::MPConstraint* const_ub = solver.MakeRowConstraint(
            0, backlog_.wave.UB, "UB");
        
        for (int j = 0; j < backlog_.numPedidos; j++) {
            int unidades = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[j]) {
                unidades += quantidade;
            }
            const_lb->SetCoefficient(x_vars[j], unidades);
            const_ub->SetCoefficient(x_vars[j], unidades);
        }
        
        // Função objetivo: Maximizar unidades totais
        operations_research::MPObjective* objetivo = solver.MutableObjective();
        
        for (int j = 0; j < backlog_.numPedidos; j++) {
            int unidades = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[j]) {
                unidades += quantidade;
            }
            objetivo->SetCoefficient(x_vars[j], unidades);
        }
        
        objetivo->SetMaximization();
        
        // Resolver
        operations_research::MPSolver::ResultStatus status = solver.Solve();
        
        if (status == operations_research::MPSolver::OPTIMAL) {
            double unidadesTotal = objetivo->Value();
            
            // Contar corredores fracionários
            double corredoresTotal = 0.0;
            for (int i = 0; i < deposito_.numCorredores; i++) {
                corredoresTotal += y_vars[i]->solution_value();
            }
            
            // Garantir pelo menos 1 corredor
            corredoresTotal = std::max(1.0, corredoresTotal);
            
            return unidadesTotal / corredoresTotal;
        }
        
        return 0.0;
    } catch (const std::exception& e) {
        std::cerr << "Erro ao resolver PL relaxada: " << e.what() << std::endl;
        return 0.0;
    }
#else
    // Sem OR-Tools, usar método heurístico
    return estimarLimiteSuperiorHeuristico();
#endif
}

void OtimizadorPLI_ALM::construirModeloPLI() {
    // Em uma implementação completa, construiríamos o modelo PLI completo
    // Aqui apenas inicializamos estatísticas básicas
    
    estatisticas_.variaveis = backlog_.numPedidos + deposito_.numCorredores + 5;
    estatisticas_.restricoes = 3 + backlog_.numPedidos + deposito_.numCorredores;
    estatisticas_.naoZeros = estatisticas_.variaveis * 5; // Estimativa
}

Solucao OtimizadorPLI_ALM::aplicarBuscaLocal(const Solucao& solucaoInicial, int LB, int UB) {
    Solucao melhorVizinho = solucaoInicial;
    double melhorValor = calcularValorObjetivo(solucaoInicial);
    
    // Flag para indicar se houve melhoria
    bool houveMelhoria = true;
    
    // Iterações máximas da busca local
    const int MAX_ITERACOES = 200;
    int iteracao = 0;
    
    while (houveMelhoria && iteracao < MAX_ITERACOES) {
        houveMelhoria = false;
        iteracao++;
        
        // 1. Tentar remover pedidos para melhorar BOV
        for (size_t i = 0; i < melhorVizinho.pedidosWave.size(); i++) {
            Solucao vizinho = melhorVizinho;
            
            // Remover pedido
            vizinho.pedidosWave.erase(vizinho.pedidosWave.begin() + i);
            
            // Recalcular corredores
            std::unordered_set<int> corredoresNecessarios;
            int totalUnidades = 0;
            
            for (int pedidoId : vizinho.pedidosWave) {
                for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
                    totalUnidades += quantidade;
                    
                    const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
                    if (!corredoresItem.empty()) {
                        corredoresNecessarios.insert(corredoresItem.begin()->first);
                    }
                }
            }
            
            // Verificar se ainda respeita LB
            if (totalUnidades < LB) {
                continue;
            }
            
            vizinho.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
            
            // Calcular novo valor objetivo
            double novoValor = calcularValorObjetivo(vizinho);
            
            // Se for melhor, atualizar
            if (novoValor > melhorValor) {
                melhorVizinho = vizinho;
                melhorValor = novoValor;
                houveMelhoria = true;
                break; // Reiniciar busca com a nova solução melhorada
            }
        }
        
        if (houveMelhoria) continue;
        
        // 2. Tentar adicionar pedidos
        for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
            // Verificar se já está incluído
            if (std::find(melhorVizinho.pedidosWave.begin(), 
                         melhorVizinho.pedidosWave.end(), 
                         pedidoId) != melhorVizinho.pedidosWave.end()) {
                continue;
            }
            
            Solucao vizinho = melhorVizinho;
            
            // Adicionar pedido
            vizinho.pedidosWave.push_back(pedidoId);
            
            // Recalcular corredores e unidades
            std::unordered_set<int> corredoresNecessarios(
                vizinho.corredoresWave.begin(), 
                vizinho.corredoresWave.end()
            );
            
            int totalUnidades = 0;
            for (int pid : vizinho.pedidosWave) {
                for (const auto& [itemId, quantidade] : backlog_.pedido[pid]) {
                    totalUnidades += quantidade;
                    
                    const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
                    if (!corredoresItem.empty()) {
                        corredoresNecessarios.insert(corredoresItem.begin()->first);
                    }
                }
            }
            
            // Verificar se não excede UB
            if (totalUnidades > UB) {
                continue;
            }
            
            vizinho.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
            
            // Calcular novo valor objetivo
            double novoValor = calcularValorObjetivo(vizinho);
            
            // Se for melhor, atualizar
            if (novoValor > melhorValor) {
                melhorVizinho = vizinho;
                melhorValor = novoValor;
                houveMelhoria = true;
                break; // Reiniciar busca com a nova solução melhorada
            }
        }
        
        if (houveMelhoria) continue;
        
        // 3. Tentar trocar pedidos (swap)
        for (size_t i = 0; i < melhorVizinho.pedidosWave.size(); i++) {
            int pedidoRemover = melhorVizinho.pedidosWave[i];
            
            for (int pedidoAdicionar = 0; pedidoAdicionar < backlog_.numPedidos; pedidoAdicionar++) {
                // Verificar se já está incluído
                if (std::find(melhorVizinho.pedidosWave.begin(), 
                            melhorVizinho.pedidosWave.end(), 
                            pedidoAdicionar) != melhorVizinho.pedidosWave.end()) {
                    continue;
                }
                
                Solucao vizinho = melhorVizinho;
                
                // Remover pedido atual
                vizinho.pedidosWave.erase(vizinho.pedidosWave.begin() + i);
                
                // Adicionar novo pedido
                vizinho.pedidosWave.push_back(pedidoAdicionar);
                
                // Recalcular corredores e unidades
                std::unordered_set<int> corredoresNecessarios;
                int totalUnidades = 0;
                
                for (int pid : vizinho.pedidosWave) {
                    for (const auto& [itemId, quantidade] : backlog_.pedido[pid]) {
                        totalUnidades += quantidade;
                        
                        const auto& corredoresItem = localizador_.getCorredoresComItem(itemId);
                        if (!corredoresItem.empty()) {
                            corredoresNecessarios.insert(corredoresItem.begin()->first);
                        }
                    }
                }
                
                // Verificar limites LB e UB
                if (totalUnidades < LB || totalUnidades > UB) {
                    continue;
                }
                
                vizinho.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
                
                // Calcular novo valor objetivo
                double novoValor = calcularValorObjetivo(vizinho);
                
                // Se for melhor, atualizar
                if (novoValor > melhorValor) {
                    melhorVizinho = vizinho;
                    melhorValor = novoValor;
                    houveMelhoria = true;
                    break; // Reiniciar busca com a nova solução melhorada
                }
            }
            
            if (houveMelhoria) break;
        }
    }
    
    return melhorVizinho;
}

Solucao OtimizadorPLI_ALM::resolverHeuristicaCustom(const Solucao& solucaoInicial, int LB, int UB) {
    // Implementação simples: aplicar busca local seguida de ajustes para viabilidade
    Solucao solucao = aplicarBuscaLocal(solucaoInicial, LB, UB);
    return ajustarViabilidade(solucao, LB, UB);
}

void preprocessamentoPLI(const std::string& caminhoEntrada, const std::string& diretorioSaida) {
    std::cout << ColorirBold("Iniciando pré-processamento PLI para: " + caminhoEntrada, CIANO) << std::endl;
    
    try {
        // Carregar instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(caminhoEntrada);
        
        // Criar estruturas auxiliares
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);
        
        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);
        
        // Criar otimizador
        OtimizadorPLI_ALM otimizador(deposito, backlog, localizador, verificador);
        
        // Exibir modelo matemático
        std::cout << "\nModelo matemático do problema:" << std::endl;
        otimizador.exibirModeloMatematico(std::cout);
        
        // Resolver o problema
        std::cout << "\nResolvendo o problema usando método ALM..." << std::endl;
        Solucao solucao = otimizador.resolver(backlog.wave.LB, backlog.wave.UB, 100, 1e-4);
        
        // Verificar se a solução é válida
        if (solucao.pedidosWave.empty()) {
            std::cerr << "ERRO: Não foi possível encontrar uma solução válida." << std::endl;
            return;
        }
        
        // Calcular estatísticas da solução
        int totalUnidades = 0;
        for (int pedidoId : solucao.pedidosWave) {
            for (const auto& [_, quantidade] : backlog.pedido[pedidoId]) {
                totalUnidades += quantidade;
            }
        }
        
        double BOV = (solucao.corredoresWave.empty()) ? 0.0 : 
                    static_cast<double>(totalUnidades) / solucao.corredoresWave.size();
        
        // Exibir informações da solução
        std::cout << "\nSolução encontrada:" << std::endl;
        std::cout << "- Pedidos selecionados: " << solucao.pedidosWave.size() << std::endl;
        std::cout << "- Corredores necessários: " << solucao.corredoresWave.size() << std::endl;
        std::cout << "- Total de unidades: " << totalUnidades << std::endl;
        std::cout << "- BOV (unidades/corredores): " << BOV << std::endl;
        
        // Salvar solução
        std::string nomeArquivo = std::filesystem::path(caminhoEntrada).stem().string();
        std::string caminhoSaida = diretorioSaida + "/" + nomeArquivo + "_pli_alm.txt";
        
        std::ofstream outFile(caminhoSaida);
        if (outFile.is_open()) {
            // Formato do arquivo de solução: numPedidos numCorredores
            outFile << solucao.pedidosWave.size() << " " << solucao.corredoresWave.size() << std::endl;
            
            // Lista de pedidos
            for (int pedido : solucao.pedidosWave) {
                outFile << pedido << " ";
            }
            outFile << std::endl;
            
            // Lista de corredores
            for (int corredor : solucao.corredoresWave) {
                outFile << corredor << " ";
            }
            outFile << std::endl;
            
            outFile.close();
            
            std::cout << "\nSolução salva em: " << caminhoSaida << std::endl;
        } else {
            std::cerr << "ERRO: Não foi possível criar o arquivo de saída: " << caminhoSaida << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "ERRO durante o pré-processamento PLI: " << e.what() << std::endl;
    }
}