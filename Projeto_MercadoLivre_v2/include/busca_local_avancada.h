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
     * @brief Estrutura para representar um movimento na busca local
     */
    struct Movimento {
        enum class Tipo {
            ADICIONAR,
            REMOVER,
            SWAP,
            CHAIN_EXCHANGE,
            PATH_RELINKING
        } tipo;
        
        std::vector<int> pedidosRemover;     // Pedidos a remover
        std::vector<int> pedidosAdicionar;   // Pedidos a adicionar
        double deltaValorObjetivo;           // Mudança estimada no valor objetivo
        
        // Construtor para movimentos simples
        Movimento(Tipo t, int pedido, double delta) 
            : tipo(t), deltaValorObjetivo(delta) {
            if (t == Tipo::ADICIONAR) {
                pedidosAdicionar.push_back(pedido);
            } else if (t == Tipo::REMOVER) {
                pedidosRemover.push_back(pedido);
            }
        }
        
        // Construtor para trocas simples
        Movimento(Tipo t, int pedidoRemover, int pedidoAdicionar, double delta)
            : tipo(t), deltaValorObjetivo(delta) {
            pedidosRemover.push_back(pedidoRemover);
            pedidosAdicionar.push_back(pedidoAdicionar);
        }
        
        // Construtor sem argumentos
        Movimento() : tipo(Tipo::SWAP), deltaValorObjetivo(0.0) {}
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
    };
    
    /**
     * @brief Estrutura para configurar o VNS
     */
    struct ConfigVNS {
        int kMax = 4;                     // Número máximo de estruturas de vizinhança
        int maxIteracoesSemMelhoria = 100;// Máximo de iterações sem melhoria
        int iteracoesPorVizinhanca = 20;  // Iterações por estrutura de vizinhança
    };
    
    /**
     * @brief Estrutura para configurar o ILS
     */
    struct ConfigILS {
        int maxIteracoes = 100;           // Número máximo de iterações
        double nivelPerturbacao = 0.3;    // Nível de perturbação (0.0 a 1.0)
        int maxIteracoesSemMelhoria = 50; // Máximo de iterações sem melhoria
    };
    
    /**
     * @brief Estrutura para estatísticas de execução
     */
    struct Estatisticas {
        int iteracoesTotais;                // Total de iterações realizadas
        int melhorias;                      // Número de melhorias encontradas
        int tempoExecucaoMs;                // Tempo de execução em milissegundos
        std::string algoritmoUsado;         // Nome do algoritmo utilizado
        
        // Estatísticas adicionais
        int movimentosAceitos;              // Número de movimentos aceitos
        int movimentosRejeitados;           // Número de movimentos rejeitados
        int iteracoesIntensificacao;        // Número de iterações em modo intensificação
        int iteracoesDiversificacao;        // Número de iterações em modo diversificação
        double melhorValorObjetivo;         // Melhor valor objetivo encontrado
        double valorObjetivoInicial;        // Valor objetivo da solução inicial
        double melhoria;                    // Percentual de melhoria
        
        // Estatísticas específicas para métodos
        // Busca Tabu
        int movimentosTabu;                 // Número de movimentos tabu encontrados
        int aspiracoesSucedidas;            // Número de vezes que o critério de aspiração foi usado
        
        // VNS
        int mudancasVizinhanca;             // Número de mudanças de estrutura de vizinhança
        int shakesSucedidos;                // Número de perturbações que levaram a melhorias
        
        // ILS
        int perturbacoes;                   // Número de perturbações realizadas
        int buscasLocais;                   // Número de buscas locais completas executadas
        
        // Construtor com inicialização
        Estatisticas() : 
            iteracoesTotais(0), melhorias(0), tempoExecucaoMs(0), algoritmoUsado("Nenhum"),
            movimentosAceitos(0), movimentosRejeitados(0), iteracoesIntensificacao(0),
            iteracoesDiversificacao(0), melhorValorObjetivo(0.0), valorObjetivoInicial(0.0),
            melhoria(0.0), movimentosTabu(0), aspiracoesSucedidas(0), mudancasVizinhanca(0),
            shakesSucedidos(0), perturbacoes(0), buscasLocais(0) {}
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
};