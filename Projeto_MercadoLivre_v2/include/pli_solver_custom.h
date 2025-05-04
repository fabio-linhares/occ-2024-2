#pragma once

#include "solucionar_desafio.h"
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <numeric>
#include "pli_solver.h"

class PLISolverCustom : public PLISolver {
public:
    PLISolverCustom();
    void configurar(const PLISolver::Config& config) override;
    
    Solucao resolver(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    ) override;
    
    std::string obterEstatisticas() const override;
    
    // Métodos específicos para resolver com diferentes algoritmos
    Solucao resolverPontosInteriores(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );
    
    Solucao resolverSimplexBNB(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );
    
    Solucao resolverGeracaoColunas(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );
    
    Solucao resolverBranchAndCut(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );
    
    Solucao resolverHibrido(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );

private:
    // Configurações
    Config config_;
    
    // Estatísticas
    struct Estatisticas {
        double tempoTotal;
        double valorOtimo;
        double gap;
        int iteracoes;
        int nodesExplorados;
        int cortes;
        int variaveisFixadas;
    } estatisticas_;
    
    // Estrutura para nós da árvore de busca
    struct Node {
        std::vector<int> pedidosFixosIn;
        std::vector<int> pedidosFixosOut;
        std::vector<int> pedidosDisponiveis;
        double limiteSuperior;
        double limiteInferior;
        std::unordered_set<int> corredoresIncluidos;
        int totalUnidades;
        int nivel;
        double lambda;

        // Operador de comparação para uso em std::priority_queue
        // Para max-heap (maior valor no topo), usamos '>' em vez de '<'
        bool operator<(const Node& other) const {
            return limiteSuperior < other.limiteSuperior;
        }
    };
    
    // Estrutura para representar matriz sparse do PLI
    struct MatrizPLI {
        std::vector<std::vector<double>> A;
        std::vector<double> b;
        std::vector<double> c;
    };
    
    // Métodos auxiliares para branch-and-bound
    Solucao resolverBranchAndBoundPersonalizado(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr,
        bool usarCortes = true
    );
    
    double calcularLimiteSuperior(
        const Node& node,
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda
    );
    
    // Métodos para abordagens gulosas
    Solucao resolverGulosoComRelaxacao(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );
    
    Solucao resolverGulosoComMultipleStarts(
        const Deposito& deposito,
        const Backlog& backlog,
        double lambda,
        int LB, int UB,
        const Solucao* solucaoInicial = nullptr
    );
    
    int selecionarVariavelParaRamificacao(const std::vector<double>& solucao);
    std::vector<std::vector<double>> gerarCortes(const std::vector<double>& solucao, const MatrizPLI& matriz, const Deposito& deposito, const Backlog& backlog);
    MatrizPLI atualizarMatrizComVariavelFixa(const MatrizPLI& matriz, int varIdx, int valor);
    bool tempoExcedido() const;
    double calcularValorObjetivo(const std::vector<double>& solucao, const std::vector<double>& c);
    double dot(const std::vector<double>& a, const std::vector<double>& b);
    
    // Tempo de início para controle do limite de tempo
    std::chrono::time_point<std::chrono::high_resolution_clock> tempoInicio_;
};