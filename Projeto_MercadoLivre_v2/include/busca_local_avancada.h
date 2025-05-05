#pragma once

#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <chrono>
#include <memory>
#include <functional>
#include <sstream>
#include <iomanip>

/**
 * @brief Implementa técnicas avançadas de busca local para otimização de waves
 * 
 * Esta classe oferece diversos algoritmos de busca local, incluindo:
 * - Busca Tabu com memória de curto e longo prazo
 * - Variable Neighborhood Search (VNS)
 * - Iterated Local Search (ILS)
 * E estruturas de vizinhança complexas como swaps eficientes e chain-exchanges.
 */
class BuscaLocalAvancada {
public:
    /**
     * @brief Estrutura para representar uma solução
     */
    struct Solucao {
        std::vector<int> pedidosWave;        // IDs dos pedidos na wave
        std::vector<int> corredoresWave;     // IDs dos corredores na wave
        double valorObjetivo;                // Valor da função objetivo
        int totalUnidades;                  // Total de unidades
        
        // Construtor
        Solucao() : valorObjetivo(0.0), totalUnidades(0) {}
    };
    
    /**
     * @brief Enumeração para definir o tipo de algoritmo de busca local
     */
    enum class TipoBuscaLocal {
        BUSCA_TABU,        // Busca Tabu
        VNS,               // Variable Neighborhood Search
        ILS                // Iterated Local Search
    };
    
    /**
     * @brief Enumeração para definir o tipo de movimento
     */
    enum class TipoMovimento {
        ADICIONAR,
        REMOVER,
        SWAP,
        CHAIN_EXCHANGE,
        PATH_RELINKING
    };

    /**
     * @brief Estrutura para representar um movimento na busca local
     */
    struct Movimento {
        TipoMovimento tipo;           // Corrigido para usar o enum
        
        std::vector<int> pedidosRemover;
        std::vector<int> pedidosAdicionar;
        double deltaValorObjetivo;
    };
    
    /**
     * @brief Estrutura para configurar a busca Tabu
     */
    struct ConfigTabu {
        int tamanhoListaTabu = 20;        // Tamanho da lista tabu
        int maxIteracoesSemMelhoria = 100;// Máximo de iterações sem melhoria
        int duracaoTabuBase = 10;         // Duração base para movimentos tabu
        bool usarMemoriaLongoPrazo = true;// Usar memória de longo prazo
        int ciclosIntensificacao = 5;     // Ciclos para iniciar intensificação
        int ciclosDiversificacao = 10;    // Ciclos para iniciar diversificação
        int maxIteracoes = 1000;          // Adicionado campo que faltava
    };
    
    /**
     * @brief Estrutura para configurar o VNS
     */
    struct ConfigVNS {
        int kMax = 4;                     // Número máximo de estruturas de vizinhança
        int maxIteracoesSemMelhoria = 100;// Máximo de iterações sem melhoria
        int iteracoesPorVizinhanca = 20;  // Iterações por estrutura de vizinhança
        int maxIteracoes = 500;           // Adicionado campo que faltava
        double intensidadeShakeBase = 0.3;// Adicionado campo que faltava
        int numVizinhancas = 3;           // Adicionado campo que faltava
    };

    /**
     * @brief Estrutura para configurar o ILS
     */
    struct ConfigILS {
        int perturbacoesSemMelhoria = 500;        // Em vez de maxIteracoesSemMelhoria
        int maxIteracoesBuscaLocal = 2000;
        double intensidadePerturbacaoBase = 0.3;  // Em vez de nivelPerturbacao
        double intensidadePerturbacaoMax = 0.7;
        int maxIteracoes = 10000;
        bool usarPerturbacaoAdaptativa = true;
        bool usarReinicializacaoEstrategica = true;
        int intervaloDiversificacao = 10000;
        double fatorAumentoIntensidade = 0.01;    // Fator para aumentar a intensidade da perturbação
        int maxIterSemMelhoriaReset = 1000;       // Máximo de iterações sem melhoria antes do reset
    };
    
    /**
     * @brief Estrutura para estatísticas de execução
     */
    struct Estatisticas {
        int iteracoesRealizadas = 0;
        int movimentosGerados = 0;
        int movimentosAplicados = 0;
        double valorObjetivoInicial = 0.0;
        double melhorValorObjetivo = 0.0;
        double tempoTotalMs = 0.0;
        std::string algoritmoUsado;
        
        double tempoExecucaoMs = 0.0;
        double melhoria = 0.0;
        int iteracoesTotais = 0;
        int melhorias = 0;
        int movimentosAceitos = 0;
        int movimentosRejeitados = 0;
        int movimentosTabu = 0;
        int aspiracoesSucedidas = 0;
        int iteracoesIntensificacao = 0;
        int iteracoesDiversificacao = 0;
        int mudancasVizinhanca = 0;
        int shakesSucedidos = 0;
        int perturbacoes = 0;
        int buscasLocais = 0;
    };

    /**
     * @brief Construtor
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     * @param limiteTempo Limite de tempo em segundos (default: 10s)
     */
    BuscaLocalAvancada(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador,
        double limiteTempo = 10.0
    );
    
    /**
     * @brief Otimiza uma solução usando o algoritmo selecionado
     * @param solucaoInicial Solução inicial para otimização
     * @param LB Limite inferior para o número de unidades
     * @param UB Limite superior para o número de unidades
     * @param tipoBusca Tipo de algoritmo de busca local a ser usado
     * @return Solução otimizada
     */
    Solucao otimizar(
        const Solucao& solucaoInicial, 
        int LB, int UB, 
        TipoBuscaLocal tipoBusca = TipoBuscaLocal::BUSCA_TABU
    );
    
    /**
     * @brief Configura parâmetros da Busca Tabu
     * @param config Configuração para Busca Tabu
     */
    void configurarTabu(const ConfigTabu& config);
    
    /**
     * @brief Configura parâmetros do VNS
     * @param config Configuração para VNS
     */
    void configurarVNS(const ConfigVNS& config);
    
    /**
     * @brief Configura parâmetros do ILS
     * @param config Configuração para ILS
     */
    void configurarILS(const ConfigILS& config);
    
    /**
     * @brief Obtém estatísticas da última execução
     * @return String formatada com estatísticas
     */
    std::string obterEstatisticas() const;

    /**
     * @brief Inicializa as estatísticas de execução
     * @param solucaoInicial Solução inicial para otimização
     */
    void iniciarEstatisticas(const Solucao& solucaoInicial);

    /**
     * @brief Aplica uma perturbação à solução
     * @param solucao Solução inicial
     * @param intensidade Intensidade da perturbação
     * @param LB Limite inferior para o número de unidades
     * @param UB Limite superior para o número de unidades
     * @return Solução perturbada
     */
    Solucao aplicarPerturbacao(const Solucao& solucao, double intensidade, int LB, int UB) {
        return perturbarSolucao(solucao, intensidade, LB, UB);
    }
    
    /**
     * @brief Registra uma perturbação nas estatísticas
     */
    void registrarPerturbacao() {
        estatisticas_.perturbacoes++;
    }

private:
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    double limiteTempo_;
    std::mt19937 rng_;
    ConfigTabu configTabu_;
    ConfigVNS configVNS_;
    ConfigILS configILS_;
    Estatisticas estatisticas_;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> tempoInicio_;
    bool tempoExcedido() const;

    // Memória de longo prazo para Tabu Search
    std::vector<int> frequenciaPedidos_;    // Frequência de uso de cada pedido
    std::vector<int> recenciaPedidos_;      // Última iteração em que o pedido foi movido
    std::vector<double> qualidadePedidos_;  // Qualidade associada a cada pedido

    // Inicializar a memória de longo prazo
    void inicializarMemoriaLongoPrazo(int numPedidos);

    // Algoritmos de busca local
    Solucao buscaTabu(const Solucao& solucaoInicial, int LB, int UB);
    Solucao vns(const Solucao& solucaoInicial, int LB, int UB);
    Solucao ils(const Solucao& solucaoInicial, int LB, int UB);
    
    // Geração de vizinhanças
    std::vector<Movimento> gerarVizinhanca(
        const Solucao& solucao, 
        int LB, int UB, 
        int tipoVizinhanca = 0
    );
    
    std::vector<Movimento> gerarMovimentosSwap(
        const Solucao& solucao, 
        int LB, int UB
    );
    
    std::vector<Movimento> gerarMovimentosChainExchange(
        const Solucao& solucao, 
        int LB, int UB
    );
    
    std::vector<Movimento> gerarMovimentosPathRelinking(
        const Solucao& solucao,
        const Solucao& solucaoGuia,
        int LB, int UB
    );

    std::vector<Movimento> gerarMovimentosIntensificacao(
        const Solucao& solucao, 
        int LB, int UB
    );

    std::vector<Movimento> gerarMovimentosDiversificacao(
        const Solucao& solucao, 
        int LB, int UB
    );
    
    // Manipulação de soluções
    Solucao aplicarMovimento(
        const Solucao& solucao,
        const Movimento& movimento
    );
    
    double avaliarMovimento(
        const Solucao& solucao,
        const Movimento& movimento
    );
    
    double calcularValorObjetivo(Solucao& solucao);
    bool solucaoViavel(const Solucao& solucao, int LB, int UB) const;
    
    // Operações de perturbação e busca local
    Solucao perturbarSolucao(
        const Solucao& solucao,
        double intensidade,
        int LB, int UB
    );
    
    Solucao buscaLocalBasica(
        const Solucao& solucao,
        int tipoVizinhanca,
        int LB, int UB
    );

    // Métodos adicionais
    void recalcularSolucao(Solucao& solucao);
    Solucao aplicarPerturbacaoForte(const Solucao& solucao, int LB, int UB);
};