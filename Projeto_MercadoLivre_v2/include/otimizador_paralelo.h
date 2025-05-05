#pragma once

#include "armazem.h"
#include "solucionar_desafio.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "analisador_relevancia.h"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <random>

/**
 * @brief Implementa versão paralela do algoritmo de otimização
 * 
 * Esta classe gerencia múltiplas threads de busca que exploram o espaço
 * de soluções simultaneamente e compartilham informações sobre as melhores
 * soluções encontradas.
 */
class OtimizadorParalelo {
public:
    /**
     * @brief Construtor
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     * @param analisador Estrutura para análise de relevância
     * @param numThreads Número de threads a utilizar (0 = automático)
     */
    OtimizadorParalelo(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador,
        const AnalisadorRelevancia& analisador,
        unsigned int numThreads = 0
    );
    
    /**
     * @brief Otimiza a solução usando múltiplas threads
     * @param solucaoInicial Solução inicial
     * @return Melhor solução encontrada
     */
    Solucao otimizar(const Solucao& solucaoInicial);
    
    /**
     * @brief Configura o tempo máximo de execução
     * @param segundos Tempo máximo em segundos
     */
    void setTempoMaximo(double segundos);
    
    /**
     * @brief Configura a frequência de comunicação entre threads
     * @param iteracoes Número de iterações entre comunicações
     */
    void setFrequenciaComunicacao(int iteracoes);

private:
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    const AnalisadorRelevancia& analisador_;
    unsigned int numThreads_;
    double tempoMaximo_ = 60.0;
    int iteracoesComunicacao_ = 100;
    
    // Estruturas para sincronização
    std::mutex melhorSolucaoMutex_;
    std::atomic<bool> terminar_;
    
    /**
     * @brief Função executada por cada thread
     * @param threadId Identificador da thread
     * @param solucaoInicial Solução inicial
     * @param melhorSolucaoGlobal Referência para a melhor solução global
     * @param resultados Promessa para retornar o resultado
     */
    void threadOtimizacao(
        int threadId,
        Solucao solucaoInicial,
        Solucao& melhorSolucaoGlobal,
        std::promise<Solucao> resultados
    );
    
    /**
     * @brief Perturbação de solução thread-safe
     * @param solucao Solução a perturbar
     * @param gerador Gerador de números aleatórios
     * @return Solução perturbada
     */
    Solucao perturbarSolucaoLocal(const Solucao& solucao, std::mt19937& gerador);
    
    /**
     * @brief Gera solução aleatória para reinicialização
     * @param gerador Gerador de números aleatórios
     * @return Nova solução aleatória diversificada
     */
    Solucao gerarSolucaoAleatoriaLocal(std::mt19937& gerador);
};