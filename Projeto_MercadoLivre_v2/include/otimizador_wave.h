#pragma once

#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include <vector>
#include <unordered_set>
#include <random>
#include <cmath>
#include <algorithm>

/**
 * @brief Classe para otimização de waves usando Simulated Annealing
 */
class OtimizadorWave {
private:
    struct SolucaoInterna {
        std::vector<int> pedidosWave;
        std::unordered_set<int> corredoresWave; // Agora usando unordered_set!
        int totalUnidades;
        double valorObjetivo;
        
        SolucaoInterna() : totalUnidades(0), valorObjetivo(0.0) {}
    };

    const Deposito& deposito;
    const Backlog& backlog;
    const LocalizadorItens& localizador;
    const VerificadorDisponibilidade& verificador;
    
    std::mt19937 rng;
    
    // Parâmetros do Simulated Annealing
    double temperaturaInicial = 100.0;
    double taxaResfriamento = 0.95;
    int maxIteracoesPorTemperatura = 100;
    int maxIteracoesSemMelhoria = 1000;
    double temperaturaMinima = 0.01;
    
    /**
     * @brief Calcula o valor objetivo de uma solução
     * @param solucao Solução a ser avaliada
     * @return Valor objetivo calculado
     */
    double calcularValorObjetivo(SolucaoInterna& solucao) {
        if (solucao.pedidosWave.empty() || solucao.corredoresWave.empty()) {
            return 0.0;
        }
        
        solucao.valorObjetivo = (double)solucao.totalUnidades / solucao.corredoresWave.size();
        return solucao.valorObjetivo;
    }
    
    /**
     * @brief Calcula o impacto marginal de adicionar um pedido
     * @param pedidoId ID do pedido a ser avaliado
     * @param solucao Solução atual
     * @return Impacto no valor objetivo
     */
    double calcularImpactoMarginal(int pedidoId, const SolucaoInterna& solucao) {
        // Calcular novos corredores necessários
        std::unordered_set<int> novosCorredores = solucao.corredoresWave;
        int unidadesAdicionais = 0;
        
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            unidadesAdicionais += quantidade;
            
            for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                novosCorredores.insert(corredorId);
            }
        }
        
        // Calcular valor objetivo atual e novo
        double valorAtual = solucao.corredoresWave.empty() ? 0 : 
                           (double)solucao.totalUnidades / solucao.corredoresWave.size();
        
        double valorNovo = (double)(solucao.totalUnidades + unidadesAdicionais) / novosCorredores.size();
        
        return valorNovo - valorAtual;
    }
    
    /**
     * @brief Gera uma solução inicial
     * @param limiteLB Limite inferior de unidades
     * @param limiteUB Limite superior de unidades
     * @return Solução inicial
     */
    SolucaoInterna gerarSolucaoInicial(int limiteLB, int limiteUB) {
        SolucaoInterna solucao;
        std::vector<int> todosPedidos;
        
        // Criar lista de todos os pedidos disponíveis
        for (int i = 0; i < backlog.numPedidos; i++) {
            if (verificador.verificarDisponibilidade(backlog.pedido[i])) {
                todosPedidos.push_back(i);
            }
        }
        
        // Embaralhar pedidos para criar uma solução aleatória
        std::shuffle(todosPedidos.begin(), todosPedidos.end(), rng);
        
        // Adicionar pedidos até atingir o limite inferior
        for (int pedidoId : todosPedidos) {
            if (!adicionarPedidoSeViavel(pedidoId, solucao, limiteUB)) {
                continue;
            }
            
            // Verificar se atingimos o limite inferior
            if (solucao.totalUnidades >= limiteLB) {
                break;
            }
        }
        
        // Se não conseguimos atingir o limite inferior, tentar novamente
        if (solucao.totalUnidades < limiteLB && !todosPedidos.empty()) {
            return gerarSolucaoInicial(limiteLB, limiteUB);
        }
        
        calcularValorObjetivo(solucao);
        return solucao;
    }
    
    /**
     * @brief Adiciona um pedido à solução se for viável
     * @param pedidoId ID do pedido a ser adicionado
     * @param solucao Solução atual
     * @param limiteUB Limite superior de unidades
     * @return True se o pedido foi adicionado com sucesso
     */
    bool adicionarPedidoSeViavel(int pedidoId, SolucaoInterna& solucao, int limiteUB) {
        // Verificar se o pedido já está na wave
        if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) != solucao.pedidosWave.end()) {
            return false;
        }
        
        // Calcular unidades adicionais
        int unidadesAdicionais = 0;
        for (const auto& [_, quantidade] : backlog.pedido[pedidoId]) {
            unidadesAdicionais += quantidade;
        }
        
        // Verificar se excede o limite superior
        if (solucao.totalUnidades + unidadesAdicionais > limiteUB) {
            return false;
        }
        
        // Adicionar o pedido à wave
        solucao.pedidosWave.push_back(pedidoId);
        solucao.totalUnidades += unidadesAdicionais;
        
        // Atualizar corredores necessários
        for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                solucao.corredoresWave.insert(corredorId);
            }
        }
        
        return true;
    }
    
    /**
     * @brief Gera uma solução vizinha através de perturbação
     * @param solucaoAtual Solução atual
     * @param limiteLB Limite inferior de unidades
     * @param limiteUB Limite superior de unidades
     * @return Nova solução gerada
     */
    SolucaoInterna gerarVizinha(const SolucaoInterna& solucaoAtual, int limiteLB, int limiteUB) {
        SolucaoInterna novaSolucao = solucaoAtual;
        
        // Escolher operação: 0 = remover, 1 = adicionar, 2 = trocar
        std::uniform_int_distribution<> distOp(0, 2);
        int operacao = distOp(rng);
        
        if (operacao == 0 && !novaSolucao.pedidosWave.empty()) {
            // Remover um pedido aleatório
            std::uniform_int_distribution<> distRem(0, novaSolucao.pedidosWave.size() - 1);
            int idx = distRem(rng);
            int pedidoRemovido = novaSolucao.pedidosWave[idx];
            
            // Verificar se podemos remover sem violar o limite inferior
            int unidadesRemovidas = 0;
            for (const auto& [_, quantidade] : backlog.pedido[pedidoRemovido]) {
                unidadesRemovidas += quantidade;
            }
            
            if (novaSolucao.totalUnidades - unidadesRemovidas >= limiteLB) {
                // Remover o pedido
                novaSolucao.pedidosWave.erase(novaSolucao.pedidosWave.begin() + idx);
                novaSolucao.totalUnidades -= unidadesRemovidas;
                
                // Recalcular corredores necessários
                novaSolucao.corredoresWave.clear();
                for (int pedidoId : novaSolucao.pedidosWave) {
                    for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
                        for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                            novaSolucao.corredoresWave.insert(corredorId);
                        }
                    }
                }
            }
        } else if (operacao == 1) {
            // Adicionar um novo pedido
            std::vector<int> pedidosCandidatos;
            for (int i = 0; i < backlog.numPedidos; i++) {
                if (std::find(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end(), i) == novaSolucao.pedidosWave.end() &&
                    verificador.verificarDisponibilidade(backlog.pedido[i])) {
                    pedidosCandidatos.push_back(i);
                }
            }
            
            if (!pedidosCandidatos.empty()) {
                std::uniform_int_distribution<> distAdd(0, pedidosCandidatos.size() - 1);
                int novoPedidoId = pedidosCandidatos[distAdd(rng)];
                
                adicionarPedidoSeViavel(novoPedidoId, novaSolucao, limiteUB);
            }
        } else if (operacao == 2 && !novaSolucao.pedidosWave.empty()) {
            // Trocar um pedido existente por um novo
            std::uniform_int_distribution<> distTroca(0, novaSolucao.pedidosWave.size() - 1);
            int idx = distTroca(rng);
            int pedidoRemovido = novaSolucao.pedidosWave[idx];
            
            // Remover o pedido selecionado
            int unidadesRemovidas = 0;
            for (const auto& [_, quantidade] : backlog.pedido[pedidoRemovido]) {
                unidadesRemovidas += quantidade;
            }
            
            novaSolucao.pedidosWave.erase(novaSolucao.pedidosWave.begin() + idx);
            novaSolucao.totalUnidades -= unidadesRemovidas;
            
            // Recalcular corredores
            novaSolucao.corredoresWave.clear();
            for (int pedidoId : novaSolucao.pedidosWave) {
                for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
                    for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                        novaSolucao.corredoresWave.insert(corredorId);
                    }
                }
            }
            
            // Adicionar um novo pedido
            std::vector<int> pedidosCandidatos;
            for (int i = 0; i < backlog.numPedidos; i++) {
                if (std::find(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end(), i) == novaSolucao.pedidosWave.end() &&
                    verificador.verificarDisponibilidade(backlog.pedido[i])) {
                    pedidosCandidatos.push_back(i);
                }
            }
            
            if (!pedidosCandidatos.empty()) {
                std::uniform_int_distribution<> distAdd(0, pedidosCandidatos.size() - 1);
                int novoPedidoId = pedidosCandidatos[distAdd(rng)];
                
                adicionarPedidoSeViavel(novoPedidoId, novaSolucao, limiteUB);
            }
        }
        
        calcularValorObjetivo(novaSolucao);
        return novaSolucao;
    }

public:
    OtimizadorWave(const Deposito& dep, const Backlog& back, 
                   const LocalizadorItens& loc, const VerificadorDisponibilidade& ver)
        : deposito(dep), backlog(back), localizador(loc), verificador(ver),
          rng(std::random_device{}()) {}
    
    /**
     * @brief Converte um SolucaoInterna para uma struct Solucao padrão
     * @param solucaoInterna Solução interna a ser convertida
     * @return Struct Solucao padrão
     */
    struct Solucao {
        std::vector<int> pedidosWave;
        std::vector<int> corredoresWave;
        double valorObjetivo;
    };

    /**
     * @brief Aplica o algoritmo Simulated Annealing para otimizar a wave
     * @param limiteLB Limite inferior de unidades
     * @param limiteUB Limite superior de unidades
     * @return Melhor solução encontrada
     */
    Solucao otimizarWave(int limiteLB, int limiteUB) {
        // Gerar solução inicial
        SolucaoInterna solucaoAtual = gerarSolucaoInicial(limiteLB, limiteUB);
        SolucaoInterna melhorSolucao = solucaoAtual;
        
        double temperatura = temperaturaInicial;
        int iteracoesSemMelhoria = 0;
        
        // Fase de pré-otimização: Ordenar pedidos por impacto marginal
        std::vector<std::pair<int, double>> pedidosComImpacto;
        for (int i = 0; i < backlog.numPedidos; i++) {
            if (verificador.verificarDisponibilidade(backlog.pedido[i])) {
                double impacto = calcularImpactoMarginal(i, solucaoAtual);
                pedidosComImpacto.push_back({i, impacto});
            }
        }
        
        // Ordenar por impacto decrescente
        std::sort(pedidosComImpacto.begin(), pedidosComImpacto.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Tentar adicionar pedidos com maior impacto positivo
        for (const auto& [pedidoId, impacto] : pedidosComImpacto) {
            if (impacto > 0) {
                adicionarPedidoSeViavel(pedidoId, solucaoAtual, limiteUB);
            }
        }
        
        // Recalcular valor objetivo
        calcularValorObjetivo(solucaoAtual);
        
        // Se a solução melhorou, atualizar a melhor solução
        if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
            melhorSolucao = solucaoAtual;
        }
        
        // Loop principal do Simulated Annealing
        while (temperatura > temperaturaMinima && iteracoesSemMelhoria < maxIteracoesSemMelhoria) {
            for (int i = 0; i < maxIteracoesPorTemperatura; i++) {
                // Gerar solução vizinha
                SolucaoInterna solucaoVizinha = gerarVizinha(solucaoAtual, limiteLB, limiteUB);
                
                // Calcular delta de energia
                double delta = solucaoVizinha.valorObjetivo - solucaoAtual.valorObjetivo;
                
                // Decidir se aceita a solução vizinha
                bool aceitarSolucao = false;
                if (delta > 0) {
                    // Solução melhor, aceitar sempre
                    aceitarSolucao = true;
                } else {
                    // Solução pior, aceitar com probabilidade baseada em temperatura
                    std::uniform_real_distribution<> dist(0.0, 1.0);
                    double probabilidade = exp(delta / temperatura);
                    aceitarSolucao = dist(rng) < probabilidade;
                }
                
                // Atualizar solução atual se aceita
                if (aceitarSolucao) {
                    solucaoAtual = solucaoVizinha;
                    
                    // Atualizar melhor solução se necessário
                    if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                        melhorSolucao = solucaoAtual;
                        iteracoesSemMelhoria = 0;
                    } else {
                        iteracoesSemMelhoria++;
                    }
                } else {
                    iteracoesSemMelhoria++;
                }
                
                // Intensificação e diversificação
                if (iteracoesSemMelhoria > maxIteracoesSemMelhoria / 2) {
                    // Diversificar (retornar à melhor solução e fazer uma grande perturbação)
                    solucaoAtual = melhorSolucao;
                    
                    // Perturbar fortemente
                    for (int j = 0; j < 3; j++) {
                        solucaoAtual = gerarVizinha(solucaoAtual, limiteLB, limiteUB);
                    }
                    
                    iteracoesSemMelhoria = 0;
                }
            }
            
            // Resfriar
            temperatura *= taxaResfriamento;
        }
        
        // Converter de SolucaoInterna para Solucao pública
        Solucao solucaoFinal;
        solucaoFinal.pedidosWave = melhorSolucao.pedidosWave;
        solucaoFinal.corredoresWave.assign(melhorSolucao.corredoresWave.begin(), melhorSolucao.corredoresWave.end());
        solucaoFinal.valorObjetivo = melhorSolucao.valorObjetivo;
        
        return solucaoFinal;
    }
};