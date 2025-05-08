#pragma once

#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "otimizador_wave.h"
#include <vector>
#include <unordered_set>
#include <utility>
#include <chrono>
#include <random>
#include <map>
#include <set>

// Nova enumeração para estratégias de perturbação
enum class EstrategiaPerturbacao {
    REMOVER_ALEATORIO,
    REMOVER_MENOS_EFICIENTES,
    REMOVER_MAIS_REDUNDANTES,
    REMOVER_MAIS_CONFLITUOSOS
};

class OtimizadorDinkelbach : public OtimizadorWave {
public:
    struct SolucaoWave {
        std::vector<int> pedidosWave;
        std::vector<int> corredoresWave;
        double valorObjetivo;
        int totalUnidades;
        
        // Operador para comparação e uso em containers ordenados
        bool operator<(const SolucaoWave& other) const {
            return valorObjetivo < other.valorObjetivo;
        }
    };
    
    struct InfoConvergencia {
        std::vector<double> valoresLambda;
        std::vector<double> valoresObjetivo;
        int iteracoesRealizadas;
        double tempoTotal;
        bool convergiu;
        std::map<double, int> frequenciaLambda; // Rastreamento de repetições de lambda
    };

    // Configuração para reinicializações múltiplas
    struct ConfigReinicializacao {
        int numReinicializacoes = 1000;           // Aumentado para maior diversificação
        bool usarSementesAleatorias = true;      // Usar sementes aleatórias diferentes
        bool aumentarIteracoesProgressivamente = true; // Aumentar iterações a cada reinício
        bool variarPerturbacao = true;           // Variar intensidade de perturbação
        bool guardarMelhoresSolucoes = true;     // Manter as melhores soluções encontradas
        int tamanhoPoolSolucoes = 500;            // Aumentado o número de melhores soluções a manter
        double limiarDiversidade = 0.3;          // Novo: limiar para garantir diversidade
        int maxTentativasSemMelhoria = 2000;       // Novo: limite de reinicializações sem melhoria
    };
    
    OtimizadorDinkelbach(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador
    );
    
    void configurarParametros(double epsilon = 0.002, int maxIteracoes = 200000, bool usarBranchAndBound = true) {
        epsilon_ = epsilon;
        maxIteracoes_ = maxIteracoes;
        usarBranchAndBound_ = usarBranchAndBound;
    }
    
    void configurarBuscaLocal(bool usar, double tempoLimite) {
        usarBuscaLocalAvancada_ = usar;
        limiteTempoBuscaLocal_ = tempoLimite;
    }
    
    // Método principal de otimização (versão original)
    SolucaoWave otimizarWave(int LB, int UB);

    // Versão melhorada com reinicializações e diversificação
    SolucaoWave otimizarWaveComReinicializacoes(int LB, int UB);
    
    // Interface unificada - escolhe automaticamente a melhor abordagem
    SolucaoWave otimizarWaveAutomatico(int LB, int UB) {
        if (usarReinicializacoesMultiplas_ && backlog_.numPedidos > 20) {
            return otimizarWaveComReinicializacoes(LB, UB);
        }
        return otimizarWave(LB, UB);
    }
    
    /**
     * @brief Obtém informações sobre a convergência do algoritmo
     * @return Referência constante à estrutura InfoConvergencia
     */
    const InfoConvergencia& obterInfoConvergencia() const {
        return infoConvergencia_;
    }
    
    /**
     * @brief Exibe detalhes da convergência no console
     */
    void exibirDetalhesConvergencia() const;

    /**
     * @brief Define se a busca local avançada deve ser usada
     * @param usar True para usar busca local avançada, false caso contrário
     */
    void setUsarBuscaLocalAvancada(bool usar) {
        usarBuscaLocalAvancada_ = usar;
    }
    
    /**
     * @brief Define o limite de tempo para busca local
     * @param limite Tempo limite em segundos
     */
    void setLimiteTempoBuscaLocal(double limite) {
        limiteTempoBuscaLocal_ = limite;
    }

    // Método para configurar as reinicializações múltiplas
    void configurarReinicializacoes(const ConfigReinicializacao& config) {
        configReinicializacao_ = config;
    }
    
    // Método para habilitar/desabilitar reinicializações
    void habilitarReinicializacoesMultiplas(bool habilitar) {
        usarReinicializacoesMultiplas_ = habilitar;
    }

private:
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    
    double epsilon_;
    int maxIteracoes_;
    bool usarBranchAndBound_;
    bool usarBuscaLocalAvancada_;
    double limiteTempoBuscaLocal_;
    
    InfoConvergencia infoConvergencia_;

    ConfigReinicializacao configReinicializacao_;
    bool usarReinicializacoesMultiplas_ = true;
    std::set<SolucaoWave> poolMelhoresSolucoes_; // Usando set para ordenar automaticamente
    std::mt19937 geradorPrincipal_; // Gerador principal para números aleatórios
    
    // Cache para evitar recálculos
    mutable std::unordered_map<int, int> cacheTotalUnidades_;
    mutable std::unordered_map<std::string, std::vector<int>> cacheCorredores_;

    // Histórico para detectar ciclos e oscilações
    std::vector<double> historicoLambda_;
    std::vector<SolucaoWave> historicoSolucoes_;
    int contadorCiclos_ = 0;
    
    // Métodos auxiliares melhorados
    int calcularTotalUnidades(const SolucaoWave& solucao);
    std::pair<SolucaoWave, double> resolverSubproblemaComBranchAndBound(double lambda, int LB, int UB);
    std::pair<SolucaoWave, double> resolverSubproblemaComHeuristica(double lambda, int LB, int UB);
    double calcularValorObjetivo(const std::vector<int>& pedidosWave);
    std::vector<int> construirListaCorredores(const std::vector<int>& pedidosWave);
    double calcularValorSubproblema(const SolucaoWave& solucao, double lambda);

    // Novos métodos para melhorar a convergência
    bool detectarOscilacao(double novoLambda, double limiar = 0.00001);
    bool detectarCiclo(const std::vector<double>& lambdas, int janela = 4);
    SolucaoWave quebrarCicloOuOscilacao(const SolucaoWave& solucaoAtual, double lambda, int LB, int UB);
    double calcularNovoLambda(const SolucaoWave& solucao, double lambdaAnterior, int iteracao);
    
    // Métodos para reinicializações e diversificação
    SolucaoWave gerarSolucaoInicialDiversificada(int indiceReinicializacao, int LB, int UB);
    double ajustarParametrosDinamicos(int indiceReinicializacao, int totalReinicializacoes);
    
    // Métodos para calcular diversidade e recombinar soluções
    double calcularDiversidade(const SolucaoWave& s1, const SolucaoWave& s2);
    SolucaoWave recombinarSolucoes(const SolucaoWave& sol1, const SolucaoWave& sol2, int LB, int UB);
    SolucaoWave perturbarSolucao(const SolucaoWave& solucao, double nivelPerturbacao, std::mt19937& rng, 
                               EstrategiaPerturbacao estrategia = EstrategiaPerturbacao::REMOVER_ALEATORIO);
    
    // Métodos para melhorar as soluções iniciais
    SolucaoWave construirSolucaoInicial(int LB, int UB);
    SolucaoWave construirSolucaoGulosaPonderada(int LB, int UB, double alpha);
    
    // Métodos para otimizar o processo de busca
    SolucaoWave otimizarWaveInterno(double lambdaInicial, int LB, int UB, int maxIter, bool logProgress = false);
};

// Função melhorada para estimar um bom valor inicial de lambda
double estimarLambdaInicial(const Deposito& deposito, const Backlog& backlog, const LocalizadorItens& localizador);