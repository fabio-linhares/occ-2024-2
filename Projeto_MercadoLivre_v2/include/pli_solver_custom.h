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

private:
    Config config_;
    struct Estatisticas {
        int iteracoes = 0;
        double tempoTotal = 0.0;
        int nodesExplorados = 0;
        int variaveisFixadas = 0;
        int cortes = 0;
        double valorOtimo = 0.0;
        double gap = 0.0;
    } estatisticas_;
    
    // Estrutura para o nó do Branch-and-Bound
    struct Node {
        std::vector<int> pedidosFixosIn;
        std::vector<int> pedidosFixosOut;
        std::vector<int> pedidosDisponiveis;
        std::unordered_set<int> corredoresIncluidos;
        double limiteSuperior;
        int totalUnidades;
        int nivel;
        
        // Operador para ordenação na fila de prioridade
        bool operator<(const Node& outro) const {
            return limiteSuperior < outro.limiteSuperior;
        }
    };
    
    // Estrutura para matriz PLI (para uso interno)
    struct MatrizPLI {
        std::vector<std::vector<double>> A;  // Matriz de coeficientes
        std::vector<double> b;               // Lado direito
        std::vector<double> c;               // Coeficientes da função objetivo
    };
    
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
    std::chrono::time_point<std::chrono::high_resolution_clock> tempoInicio_;
};