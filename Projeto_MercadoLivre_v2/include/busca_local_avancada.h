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
 */
class BuscaLocalAvancada {
public:
    /**
     * @brief Estrutura para representar uma solução
     */
    struct Solucao {
        std::vector<int> pedidosWave;
        std::vector<int> corredoresWave;
        double valorObjetivo;
        int totalUnidades;
    };
    
    /**
     * @brief Enumeração para definir o tipo de algoritmo de busca local
     */
    enum class TipoBuscaLocal {
        BUSCA_TABU,
        VNS,
        ILS
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
        TipoMovimento tipo;        
        std::vector<int> pedidosRemover;
        std::vector<int> pedidosAdicionar;
        double deltaValorObjetivo;
    };
    
    /**
     * @brief Estrutura para configurar a busca Tabu
     */
    struct ConfigTabu {
        int tamanhoListaTabu = 20;
        int maxIteracoesSemMelhoria = 100;
        int duracaoTabuBase = 10;
        bool usarMemoriaLongoPrazo = true;
        int ciclosIntensificacao = 5;
        int ciclosDiversificacao = 10;
        int maxIteracoes = 1000;
    };
    
    /**
     * @brief Estrutura para configurar o VNS
     */
    struct ConfigVNS {
        int kMax = 4;
        int maxIteracoesSemMelhoria = 100;
        int iteracoesPorVizinhanca = 20;
        int maxIteracoes = 500;
        double intensidadeShakeBase = 0.3;
        int numVizinhancas = 3;
    };

    /**
     * @brief Estrutura para configurar o ILS
     */
    struct ConfigILS {
        int maxIteracoes = 500;
        int iteracoesInternas = 50;
        double intensidadePerturbacaoInicial = 0.3;
        double fatorAumentoPerturbacao = 1.2;
        int maxIteracoesSemMelhoria = 100;
        bool usarReinicioPeriodico = true;
        int frequenciaReinicio = 10;
    };
    
    /**
     * @brief Estrutura para estatísticas de execução
     */
    struct Estatisticas {
        std::string algoritmoUsado;
        int iteracoesTotais = 0;
        int melhorias = 0;
        double valorObjetivoInicial = 0.0;
        double melhorValorObjetivo = 0.0;
        double melhoria = 0.0;
        double tempoTotalMs = 0.0;
        double tempoExecucaoMs = 0.0;
        int movimentosGerados = 0;
        int movimentosAplicados = 0;
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
     * @param limiteTempo Tempo limite para execução (em segundos)
     */
    BuscaLocalAvancada(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador,
        double limiteTempo
    );
    
    Solucao otimizar(
        const Solucao& solucaoInicial, 
        int LB, int UB, 
        TipoBuscaLocal tipoBusca = TipoBuscaLocal::BUSCA_TABU
    );
    
    void configurarTabu(const ConfigTabu& config);
    void configurarVNS(const ConfigVNS& config);
    void configurarILS(const ConfigILS& config);
    std::string obterEstatisticas() const;
    void iniciarEstatisticas(const Solucao& solucaoInicial);

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
    std::vector<int> frequenciaPedidos_;
    std::vector<int> recenciaPedidos_;
    std::vector<double> qualidadePedidos_;
    void inicializarMemoriaLongoPrazo(int numPedidos);
    
    Solucao buscaTabu(const Solucao& solucaoInicial, int LB, int UB);
    Solucao vns(const Solucao& solucaoInicial, int LB, int UB);
    Solucao ils(const Solucao& solucaoInicial, int LB, int UB);
    
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
    
    /**
     * @brief Gera movimentos específicos para redução do número de corredores
     * 
     * Este método implementa uma vizinhança especializada focada em minimizar o número 
     * de corredores necessários, uma estratégia crítica para maximizar o BOV.
     * 
     * @param solucao Solução atual
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @return Vetor de movimentos para redução de corredores
     */
    std::vector<Movimento> gerarMovimentosReducaoCorredores(
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
    
    void recalcularSolucao(Solucao& solucao);
    Solucao aplicarPerturbacaoForte(const Solucao& solucao, int LB, int UB);
};