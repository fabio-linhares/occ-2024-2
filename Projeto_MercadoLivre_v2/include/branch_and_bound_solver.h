#pragma once

#include <vector>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <utility>
#include <memory>
#include <chrono>
#include <random>
#include <functional>
#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"

/**
 * @brief Implementa um algoritmo branch-and-bound para o subproblema linearizado do Dinkelbach
 * 
 * Este solver é especialmente eficiente para instâncias pequenas a médias, pois pode 
 * explorar de forma sistemática o espaço de soluções através de técnicas de poda e ramificação.
 */
class BranchAndBoundSolver {
public:
    /**
     * @brief Enumeração para diferentes estratégias de seleção de variáveis
     */
    enum class EstrategiaSelecionarVariavel {
        PRIMEIRA,              // Seleciona a primeira variável disponível (estratégia básica)
        MAIOR_IMPACTO,         // Seleciona a variável com maior impacto potencial na função objetivo
        MOST_INFEASIBLE,       // Seleciona a variável com valor mais distante de um inteiro (para relaxações)
        PSEUDO_CUSTO           // Usa histórico de impacto das variáveis para guiar seleção
    };
    
    /**
     * @brief Estrutura para armazenar uma solução
     */
    struct Solucao {
        std::vector<int> pedidosWave;           // IDs dos pedidos na wave
        std::vector<int> corredoresWave;        // IDs dos corredores na wave
        double valorObjetivo;                   // Valor da função objetivo F(x) - lambda*G(x)
        int totalUnidades;                     // Total de unidades (F(x))
        int totalCorredores;                   // Total de corredores (G(x))
        
        // Constructor
        Solucao(std::vector<int> pedidos = {}, 
                std::vector<int> corredores = {}, 
                double valor = 0.0, 
                int unidades = 0, 
                int numCorredores = 0) : 
            pedidosWave(pedidos),
            corredoresWave(corredores),
            valorObjetivo(valor),
            totalUnidades(unidades),
            totalCorredores(numCorredores) {}
    };

    /**
     * @brief Estrutura para representar um nó na árvore de branch-and-bound
     */
    struct Node {
        std::vector<int> pedidosFixosIn;       // Pedidos decididos a incluir
        std::vector<int> pedidosFixosOut;      // Pedidos decididos a excluir
        std::vector<int> pedidosDisponiveis;   // Pedidos ainda não decididos
        double limiteSuperior;                 // Melhor valor possível a partir deste nó
        double limiteInferior;                 // Pior valor garantido a partir deste nó
        Solucao melhorSolucaoLocal;            // Melhor solução encontrada neste nó
        double lambda;                         // Valor atual do lambda (Dinkelbach)
        int nivel;                             // Nível na árvore (para debug)
        std::unordered_set<int> corredoresIncluidos; // Corredores já incluídos na solução parcial
        int totalUnidades;                     // Total de unidades na solução parcial
        
        // Comparador para fila de prioridade (por limite superior decrescente)
        bool operator<(const Node& other) const {
            return limiteSuperior < other.limiteSuperior;
        }
    };

    /**
     * @brief Estrutura para manter estatísticas de execução
     */
    struct Estatisticas {
        int nodesExplorados;        // Número de nós explorados
        int nodesPodados;           // Número de nós podados
        int nodesPodadosLS;         // Nós podados por limite superior
        int nodesPodadosInfactivel; // Nós podados por infactibilidade
        int cortesDominancia;       // Número de cortes de dominância aplicados
        int cortesCobertura;        // Número de cortes de cobertura aplicados
        int tempoExecucaoMs;        // Tempo de execução em milissegundos
        
        Estatisticas() : nodesExplorados(0), nodesPodados(0), nodesPodadosLS(0), 
                        nodesPodadosInfactivel(0), cortesDominancia(0), 
                        cortesCobertura(0), tempoExecucaoMs(0) {}
    };

    /**
     * @brief Construtor
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog de pedidos
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     * @param limiteTempo Limite de tempo em segundos para execução
     * @param estrategia Estratégia de seleção de variáveis
     */
    BranchAndBoundSolver(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador,
        double limiteTempo = 5.0,
        EstrategiaSelecionarVariavel estrategia = EstrategiaSelecionarVariavel::MAIOR_IMPACTO
    );

    /**
     * @brief Resolve o subproblema linearizado do Dinkelbach
     * @param lambda Valor atual de lambda
     * @param LB Limite inferior para número de unidades
     * @param UB Limite superior para número de unidades
     * @return Melhor solução encontrada
     */
    Solucao resolver(double lambda, int LB, int UB);
    
    /**
     * @brief Obtém as estatísticas da última execução
     * @return Estrutura com estatísticas
     */
    const Estatisticas& getEstatisticas() const { return estatisticas_; }
    
    /**
     * @brief Define a estratégia de seleção de variáveis
     * @param estrategia Estratégia a ser utilizada
     */
    void setEstrategia(EstrategiaSelecionarVariavel estrategia) { estrategia_ = estrategia; }
    
    /**
     * @brief Define se deve usar cortes de cobertura
     * @param usar True para usar cortes de cobertura, false caso contrário
     */
    void setUsarCortesCobertura(bool usar) { usarCortesCobertura_ = usar; }
    
    /**
     * @brief Define se deve usar cortes de dominância
     * @param usar True para usar cortes de dominância, false caso contrário
     */
    void setUsarCortesDominancia(bool usar) { usarCortesDominancia_ = usar; }
    
    /**
     * @brief Define o coeficiente de peso para o cálculo de limites heurísticos
     * @param coef Valor entre 0 e 1, onde 0 é mais otimista e 1 mais pessimista
     */
    void setCoeficienteLimite(double coef) { coeficienteLimite_ = std::max(0.0, std::min(1.0, coef)); }

private:
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    double limiteTempo_;
    std::chrono::time_point<std::chrono::high_resolution_clock> tempoInicio_;
    Solucao melhorSolucao_;
    Estatisticas estatisticas_;
    EstrategiaSelecionarVariavel estrategia_;
    bool usarCortesCobertura_;
    bool usarCortesDominancia_;
    double coeficienteLimite_;
    std::mt19937 rng_;
    
    // Pseudo-custos para guiar seleção de variáveis
    std::vector<std::pair<double, double>> pseudoCustos_; // (custo para decidir 1, custo para decidir 0)
    
    // Cache para contribuições de pedidos
    std::unordered_map<int, std::pair<double, int>> cacheContribuicoes_;

    /**
     * @brief Verifica se o tempo limite foi excedido
     * @return true se o tempo limite foi excedido, false caso contrário
     */
    bool tempoExcedido() const;

    /**
     * @brief Calcula o limite superior para um nó usando técnicas avançadas
     * @param node Nó a ter seu limite superior calculado
     * @return Valor do limite superior
     */
    double calcularLimiteSuperior(Node& node);
    
    /**
     * @brief Calcula o limite inferior para um nó
     * @param node Nó a ter seu limite inferior calculado
     * @return Valor do limite inferior
     */
    double calcularLimiteInferior(Node& node);

    /**
     * @brief Constrói uma solução a partir dos pedidos selecionados
     * @param pedidosSelecionados Conjunto de IDs dos pedidos selecionados
     * @param lambda Valor atual de lambda para o cálculo da função objetivo
     * @return Solução construída
     */
    Solucao construirSolucao(const std::vector<int>& pedidosSelecionados, double lambda);

    /**
     * @brief Verifica se uma solução é viável considerando LB e UB
     * @param solucao Solução a ser verificada
     * @param LB Limite inferior para número de unidades
     * @param UB Limite superior para número de unidades
     * @return true se a solução é viável, false caso contrário
     */
    bool solucaoViavel(const Solucao& solucao, int LB, int UB) const;

    /**
     * @brief Seleciona o próximo pedido para ramificação baseado na estratégia escolhida
     * @param node Nó atual contendo pedidos disponíveis
     * @return Índice do pedido escolhido para ramificação
     */
    int selecionarPedidoParaRamificacao(const Node& node);
    
    /**
     * @brief Seleciona o próximo pedido usando a estratégia de maior impacto potencial
     * @param node Nó atual contendo pedidos disponíveis
     * @return Índice do pedido escolhido para ramificação
     */
    int selecionarPedidoPorMaiorImpacto(const Node& node);
    
    /**
     * @brief Seleciona o próximo pedido usando a estratégia de pseudo-custo
     * @param node Nó atual contendo pedidos disponíveis
     * @return Índice do pedido escolhido para ramificação
     */
    int selecionarPedidoPorPseudoCusto(const Node& node);

    /**
     * @brief Calcula a contribuição de um pedido para a função objetivo
     * @param pedidoId ID do pedido
     * @param lambda Valor atual de lambda
     * @param corredoresJaIncluidos Conjunto de corredores já incluídos na solução parcial
     * @return Par com (valorContribuicao, novosCorredores)
     */
    std::pair<double, int> calcularContribuicaoPedido(
        int pedidoId, 
        double lambda, 
        const std::unordered_set<int>& corredoresJaIncluidos
    );

    /**
     * @brief Realiza o processo de ramificação de um nó
     * @param node Nó a ser ramificado
     * @param pedidoId ID do pedido escolhido para ramificação
     * @return Par de nós filhos (incluir, excluir)
     */
    std::pair<Node, Node> ramificar(const Node& node, int pedidoId);

    /**
     * @brief Atualiza a melhor solução global
     * @param solucao Nova solução candidata
     * @return true se a solução foi atualizada, false caso contrário
     */
    bool atualizarMelhorSolucao(const Solucao& solucao);
    
    /**
     * @brief Aplica cortes de cobertura para podar o espaço de busca
     * @param node Nó a ser analisado para cortes
     * @return true se o nó deve ser podado, false caso contrário
     */
    bool aplicarCortesCobertura(const Node& node, int LB, int UB);
    
    /**
     * @brief Aplica cortes de dominância para podar o espaço de busca
     * @param node Nó a ser analisado para cortes
     * @return true se o nó deve ser podado, false caso contrário
     */
    bool aplicarCortesDominancia(const Node& node);
    
    /**
     * @brief Identifica pedidos incompatíveis para cortes de cobertura
     * @return Conjunto de pares de pedidos incompatíveis
     */
    std::vector<std::pair<int, int>> identificarPedidosIncompativeis();
    
    /**
     * @brief Atualiza os pseudo-custos com base no impacto da decisão
     * @param pedidoId ID do pedido
     * @param decisao True para incluir, false para excluir
     * @param impacto Impacto no valor objetivo
     */
    void atualizarPseudoCusto(int pedidoId, bool decisao, double impacto);
};