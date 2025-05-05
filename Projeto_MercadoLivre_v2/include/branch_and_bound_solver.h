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
 * @brief Estados possíveis para as variáveis de decisão
 */
enum class Estado {
    FIXO_IN,  // Pedido fixado para entrar na solução
    FIXO_OUT, // Pedido fixado para ficar fora da solução
    LIVRE     // Pedido ainda não decidido
};

/**
 * @brief Implementa um algoritmo branch-and-bound para o subproblema linearizado do Dinkelbach
 * 
 * Este solver é especialmente eficiente para instâncias pequenas a médias, pois pode 
 * explorar de forma sistemática o espaço de soluções através de técnicas de poda e ramificação.
 */
class BranchAndBoundSolver {
public:
    // Tipos e estruturas
    enum class Estado { LIVRE, FIXO_IN, FIXO_OUT };
    enum class EstrategiaSelecionarVariavel { PRIMEIRA, MAIOR_IMPACTO, PSEUDO_CUSTO };

    /**
     * @brief Estrutura para uma solução
     */
    struct Solucao {
        std::vector<int> pedidosWave;        
        std::vector<int> corredoresWave;        
        double valorObjetivo;        
        int totalUnidades;        
        int totalCorredores;        
        double lambda;
        
        // Constructor
        Solucao(std::vector<int> pedidos = {}, 
                std::vector<int> corredores = {}, 
                double valor = 0.0, 
                int unidades = 0, 
                int numCorredores = 0, 
                double lambdaValue = 0.0) : 
            pedidosWave(pedidos),
            corredoresWave(corredores),
            valorObjetivo(valor),
            totalUnidades(unidades),
            totalCorredores(numCorredores),
            lambda(lambdaValue) {}
    };

    /**
     * @brief Estrutura para representar um nó na árvore de branch-and-bound
     */
    struct Node {
        std::vector<Estado> pedidosFixados; // Estado de cada pedido
        std::vector<int> pedidosFixosOut;  // Pedidos fixados como excluídos
        std::vector<int> pedidosDisponiveis; // Pedidos ainda disponíveis para ramificação
        std::unordered_set<int> corredoresIncluidos; // Corredores já incluídos pelos pedidos fixos
        double limiteSuperior;    // Limite superior para este nó
        double limiteInferior;    // Limite inferior para este nó
        int nivel;                // Nível do nó na árvore
        double lambda;            // Valor de lambda para o subproblema
        int totalUnidades;        // Total de unidades incluídas até o momento
        
        // Operador para ordenação na fila de prioridade (maior limiteSuperior primeiro)
        bool operator<(const Node& outro) const {
            return limiteSuperior < outro.limiteSuperior;
        }
    };
    
    /**
     * @brief Estrutura para estatísticas do solver
     */
    struct Estatisticas {
        int nodesExplorados = 0;
        int nodesPodados = 0;
        int nodesPodadosLS = 0;
        int nodesPodadosInfactivel = 0;
        int cortesCobertura = 0;
        int cortesDominancia = 0;
        double tempoExecucaoMs = 0.0;
    };
    
    /**
     * @brief Construtor
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
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
     * @brief Resolve o problema de branch-and-bound
     * @param lambda Valor de lambda para o problema linearizado
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return Solução otimizada
     */
    Solucao resolver(double lambda, int LB, int UB);
    
    /**
     * @brief Obtém as estatísticas da última execução
     * @return Estrutura com estatísticas
     */
    const Estatisticas& getEstatisticas() const { return estatisticas_; }
    
    /**
     * @brief Define o coeficiente de limite (ajusta otimismo no cálculo de limites)
     * @param coeficiente Coeficiente entre 0 e 1 (0 = pessimista, 1 = otimista)
     */
    void setCoeficienteLimite(double coeficiente) { coeficienteLimite_ = coeficiente; }
    
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
     * @brief Atualiza os pseudo-custos para um pedido
     * @param pedidoId ID do pedido
     * @param decisao true para inclusão, false para exclusão
     * @param impacto Impacto observado na função objetivo
     */
    void atualizarPseudoCusto(int pedidoId, bool decisao, double impacto);

private:
    // Atributos
    Deposito deposito_;
    Backlog backlog_;
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
    std::vector<std::pair<double, double>> pseudoCustos_;    
    // Cache para evitar recálculos
    std::unordered_map<int, std::pair<double, int>> cacheContribuicoes_;
    
    // Adicionar estes atributos
    int maxNodos_; // Número máximo de nós a explorar
    double melhorValor_; // Melhor valor objetivo encontrado
    
    /**
     * @brief Verifica se o tempo limite foi excedido
     * @return true se o tempo foi excedido, false caso contrário
     */
    bool tempoExcedido() const;
    
    /**
     * @brief Gera uma solução inicial viável (gulosa)
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return true se gerou uma solução viável, false caso contrário
     */
    bool gerarSolucaoInicialViavel(int LB, int UB);
    
    /**
     * @brief Calcula o limite superior para um nó
     * @param node Nó a ser avaliado
     * @return Valor do limite superior
     */
    double calcularLimiteSuperior(Node& node);
    
    /**
     * @brief Calcula o limite inferior para um nó
     * @param node Nó a ser avaliado
     * @return Valor do limite inferior
     */
    double calcularLimiteInferior(Node& node);
    
    /**
     * @brief Constrói uma solução a partir de um conjunto de pedidos
     * @param pedidosSelecionados Lista de IDs de pedidos selecionados
     * @param lambda Valor de lambda para o cálculo do valor objetivo
     * @return Solução construída
     */
    Solucao construirSolucao(const std::vector<int>& pedidosSelecionados, double lambda);
    
    /**
     * @brief Verifica se uma solução é viável
     * @param solucao Solução a ser verificada
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return true se a solução é viável, false caso contrário
     */
    bool solucaoViavel(const Solucao& solucao, int LB, int UB) const;
    
    /**
     * @brief Seleciona o próximo pedido para ramificação
     * @param node Nó atual
     * @return Índice no vetor pedidosDisponiveis do pedido selecionado
     */
    int selecionarPedidoParaRamificacao(const Node& node);
    
    /**
     * @brief Seleciona pedido por maior impacto potencial
     * @param node Nó atual
     * @return Índice no vetor pedidosDisponiveis do pedido selecionado
     */
    int selecionarPedidoPorMaiorImpacto(const Node& node);
    
    /**
     * @brief Seleciona pedido por pseudo-custo acumulado
     * @param node Nó atual
     * @return Índice no vetor pedidosDisponiveis do pedido selecionado
     */
    int selecionarPedidoPorPseudoCusto(const Node& node);
    
    /**
     * @brief Seleção exaustiva de pedido (para instâncias pequenas)
     * @param node Nó atual
     * @return Índice no vetor pedidosDisponiveis do pedido selecionado
     */
    int selecionarPedidoExaustivo(const Node& node);
    
    /**
     * @brief Calcula a contribuição marginal de um pedido
     * @param pedidoId ID do pedido
     * @param lambda Valor de lambda
     * @param corredoresJaIncluidos Conjunto de corredores já incluídos
     * @return Par com contribuição e número de novos corredores
     */
    std::pair<double, int> calcularContribuicaoPedido(
        int pedidoId, 
        double lambda, 
        const std::unordered_set<int>& corredoresJaIncluidos
    );
    
    /**
     * @brief Ramifica um nó em dois (incluir/excluir pedido)
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
     * @param node Nó atual
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return true se o nó deve ser podado, false caso contrário
     */
    bool aplicarCortesCobertura(const Node& node, int LB, int UB);
    
    /**
     * @brief Aplica cortes de dominância para podar o espaço de busca
     * @param node Nó atual
     * @return true se o nó deve ser podado, false caso contrário
     */
    bool aplicarCortesDominancia(const Node& node);
    
    /**
     * @brief Identifica pares de pedidos incompatíveis (não podem estar juntos)
     * @return Lista de pares de pedidos incompatíveis
     */
    std::vector<std::pair<int, int>> identificarPedidosIncompativeis();

    /**
     * @brief Verifica a compatibilidade entre dois pedidos
     * @param pedidoId1 ID do primeiro pedido
     * @param pedidoId2 ID do segundo pedido
     * @return true se os pedidos são compatíveis, false caso contrário
     */
    bool verificarCompabilidadePedidos(int pedidoId1, int pedidoId2);

    /**
     * @brief Constrói uma solução gulosa
     * @param lambda Valor de lambda para o problema linearizado
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return Solução gulosa construída
     */
    Solucao construirSolucaoGulosa(double lambda, int LB, int UB);
};