#pragma once

#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "otimizador_wave.h"
#include <vector>
#include <unordered_set>
#include <utility>
#include <chrono>

class OtimizadorDinkelbach : public OtimizadorWave {
public:
    struct SolucaoWave {
        std::vector<int> pedidosWave;
        std::vector<int> corredoresWave;
        double valorObjetivo;
    };
    
    struct InfoConvergencia {
        std::vector<double> valoresLambda;
        std::vector<double> valoresObjetivo;
        int iteracoesRealizadas;
        double tempoTotal;
        bool convergiu;
    };

    // Configuração para reinicializações múltiplas
    struct ConfigReinicializacao {
        int numReinicializacoes = 5;           // Número de reinicializações diferentes
        bool usarSementesAleatorias = true;    // Usar sementes aleatórias diferentes
        bool aumentarIteracoesProgressivamente = true; // Aumentar iterações a cada reinício
        bool variarPerturbacao = true;         // Variar intensidade de perturbação
        bool guardarMelhoresSolucoes = true;   // Manter as melhores soluções encontradas
        int tamanhoPoolSolucoes = 3;           // Número de melhores soluções a manter
    };
    
    OtimizadorDinkelbach(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador
    );
    
    void configurarParametros(double epsilon = 0.0001, int maxIteracoes = 10000, bool usarBranchAndBound = true) {
        epsilon_ = epsilon;
        maxIteracoes_ = maxIteracoes; // Aumentado para 10.000
        usarBranchAndBound_ = usarBranchAndBound;
    }
    
    void configurarBuscaLocal(bool usar, double tempoLimite) {
        usarBuscaLocalAvancada_ = usar;
        limiteTempoBuscaLocal_ = tempoLimite;
    }
    
    SolucaoWave otimizarWave(int LB, int UB);

    // Nova versão do método otimizarWave que usa reinicializações
    SolucaoWave otimizarWaveComReinicializacoes(int LB, int UB);
    
    /**
     * @brief Obtém informações sobre a convergência do algoritmo
     * @return Referência constante à estrutura InfoConvergencia
     */
    const InfoConvergencia& obterInfoConvergencia() const { return infoConvergencia_; }
    
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
    bool usarReinicializacoesMultiplas_ = false;
    
    int calcularTotalUnidades(const SolucaoWave& solucao);
    
    std::pair<SolucaoWave, double> resolverSubproblemaComBranchAndBound(double lambda, int LB, int UB);
    std::pair<SolucaoWave, double> resolverSubproblemaComHeuristica(double lambda, int LB, int UB);
    
    /**
     * @brief Calcula o valor objetivo para um conjunto de pedidos
     * @param pedidosWave Vetor de IDs dos pedidos
     * @return Valor objetivo calculado
     */
    double calcularValorObjetivo(const std::vector<int>& pedidosWave);
    
    /**
     * @brief Constrói a lista de corredores necessários para um conjunto de pedidos
     * @param pedidosWave Vetor de IDs dos pedidos
     * @return Vetor de IDs dos corredores necessários
     */
    std::vector<int> construirListaCorredores(const std::vector<int>& pedidosWave);
    
    /**
     * @brief Calcula o valor do subproblema linearizado F(x) - lambda*G(x)
     * @param solucao Solução a avaliar
     * @param lambda Valor atual de lambda
     * @return Valor do subproblema
     */
    double calcularValorSubproblema(const SolucaoWave& solucao, double lambda);

    // Métodos auxiliares para reinicializações
    SolucaoWave gerarSolucaoInicialDiversificada(int indiceReinicializacao, int LB, int UB);
    double ajustarParametrosDinamicos(int indiceReinicializacao, int totalReinicializacoes);

    // Métodos para reinicializações múltiplas
    double calcularBOV(const SolucaoWave& solucao);
    int getTotalUnidades(const SolucaoWave& solucao);
    SolucaoWave perturbarSolucao(const SolucaoWave& solucao, double nivelPerturbacao, std::mt19937& rng);
    SolucaoWave recombinarSolucoes(const SolucaoWave& solucao1, const SolucaoWave& solucao2, int LB, int UB);
    SolucaoWave otimizarWave(int LB, int UB, const SolucaoWave& solucaoInicial);
};

// Função para estimar um bom valor inicial de lambda
double estimarLambdaInicial(const Deposito& deposito, const Backlog& backlog, const LocalizadorItens& localizador);