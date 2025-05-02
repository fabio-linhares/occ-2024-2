#pragma once

#include <vector>
#include <functional>
#include <random>
#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"

/**
 * @brief Estrutura para solução multi-objetivo
 */
struct SolucaoMultiobjetivo {
    std::vector<int> pedidosWave;
    std::vector<int> corredoresWave;
    std::vector<double> valoresObjetivo;
    bool dominada;
    
    // Adicionar operador de igualdade para comparações
    bool operator==(const SolucaoMultiobjetivo& other) const {
        return pedidosWave == other.pedidosWave;
    }
};

/**
 * @brief Classe para otimização multi-objetivo
 */
class OtimizadorMultiobjetivo {
public:
    /**
     * @brief Enumeração dos objetivos disponíveis
     */
    enum ObjetivoEnum {
        MAXIMIZAR_UNIDADES_POR_CORREDOR,
        MINIMIZAR_DISTANCIA_TOTAL,
        MAXIMIZAR_PRIORIDADE_PEDIDOS,
        BALANCEAR_CARGA_CORREDORES,
        MINIMIZAR_TEMPO_COLETA
    };

private:
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    std::vector<ObjetivoEnum> objetivosSelecionados_;
    std::vector<double> pesosObjetivos_;
    
    /**
     * @brief Avalia todos os objetivos para uma solução
     * @param solucao Solução a ser avaliada
     */
    void avaliarObjetivos(SolucaoMultiobjetivo& solucao);
    
    /**
     * @brief Implementa o método de seleção por torneio
     * @param populacao População atual
     * @param gen Gerador de números aleatórios
     * @return Índice do indivíduo selecionado
     */
    int selecaoTorneio(
        const std::vector<SolucaoMultiobjetivo>& populacao,
        std::mt19937& gen
    );
    
    /**
     * @brief Implementa o operador de crossover
     * @param pai1 Primeiro pai
     * @param pai2 Segundo pai
     * @param gen Gerador de números aleatórios
     * @return Nova solução gerada
     */
    SolucaoMultiobjetivo crossover(
        const SolucaoMultiobjetivo& pai1,
        const SolucaoMultiobjetivo& pai2,
        std::mt19937& gen
    );
    
    /**
     * @brief Implementa o operador de mutação
     * @param solucao Solução a ser mutada
     * @param gen Gerador de números aleatórios
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     */
    void mutacao(
        SolucaoMultiobjetivo& solucao,
        std::mt19937& gen,
        int LB, int UB
    );
    
    /**
     * @brief Ordena uma população usando conceito de dominância de Pareto
     * @param populacao População a ser ordenada
     */
    void ordenarPorDominancia(
        std::vector<SolucaoMultiobjetivo>& populacao
    );
    
public:
    /**
     * @brief Construtor
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     */
    OtimizadorMultiobjetivo(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador
    );
    
    /**
     * @brief Configura os objetivos a serem considerados
     * @param objetivos Lista de objetivos
     * @param pesos Pesos correspondentes a cada objetivo
     */
    void configurarObjetivos(
        const std::vector<ObjetivoEnum>& objetivos,
        const std::vector<double>& pesos
    );
    
    /**
     * @brief Executa otimização com algoritmo NSGA-II
     * @param tamanhoPopulacao Tamanho da população
     * @param numGeracoes Número de gerações
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return Conjunto de soluções não-dominadas (fronteira de Pareto)
     */
    std::vector<SolucaoMultiobjetivo> otimizarNSGAII(
        int tamanhoPopulacao,
        int numGeracoes,
        int LB, int UB
    );
    
    /**
     * @brief Executa otimização com algoritmo MOEA/D
     * @param tamanhoPopulacao Tamanho da população
     * @param numGeracoes Número de gerações
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return Conjunto de soluções não-dominadas (fronteira de Pareto)
     */
    std::vector<SolucaoMultiobjetivo> otimizarMOEAD(
        int tamanhoPopulacao,
        int numGeracoes,
        int LB, int UB
    );
    
    /**
     * @brief Seleciona uma solução preferida da fronteira de Pareto
     * @param solucoesFronteira Soluções na fronteira de Pareto
     * @return Solução preferida segundo critérios adicionais
     */
    SolucaoMultiobjetivo selecionarSolucaoPreferida(
        const std::vector<SolucaoMultiobjetivo>& solucoesFronteira
    );
};