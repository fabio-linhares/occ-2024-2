#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "solucionar_desafio.h"

/**
 * @brief Estrutura para representar um ponto de quebra na linearização por partes
 */
struct PontoQuebra {
    double valor;
    double valorQuadratico;
};

/**
 * @brief Estrutura para representar uma variável de decisão no modelo PLI
 */
struct VariavelPLI {
    std::string nome;
    bool binaria;
    double lowerBound;
    double upperBound;
    double valorAtual;
};

/**
 * @brief Estrutura para representar uma restrição no modelo PLI
 */
struct RestricaoPLI {
    std::string nome;
    std::vector<std::pair<std::string, double>> coeficientes;
    char sentido; // '=', '<', '>'
    double ladoDireito;
};

/**
 * @brief Estrutura para armazenar estatísticas de execução
 */
struct EstatisticasPLI {
    int iteracoes;
    double tempoTotal;
    double valorInicial;
    double valorFinal;
    double gap;
    int cortesGerados;
    int variaveis;
    int restricoes;
    int naoZeros;
};

/**
 * @brief Implementação do otimizador PLI usando o método do Lagrangeano Aumentado
 */
class OtimizadorPLI_ALM {
public:
    /**
     * @brief Construtor
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param localizador Estrutura para localização de itens
     * @param verificador Estrutura para verificação de disponibilidade
     */
    OtimizadorPLI_ALM(
        const Deposito& deposito,
        const Backlog& backlog,
        const LocalizadorItens& localizador,
        const VerificadorDisponibilidade& verificador
    );
    
    /**
     * @brief Executa o pré-processamento e explica o modelo
     * @param output Stream de saída para as informações
     */
    void exibirModeloMatematico(std::ostream& output);
    
    /**
     * @brief Resolve o problema usando o método ALM
     * @param LB Limite inferior de unidades
     * @param UB Limite superior de unidades
     * @param maxIteracoes Número máximo de iterações
     * @param tolerancia Tolerância de convergência
     * @return Solução otimizada
     */
    Solucao resolver(int LB, int UB, int maxIteracoes = 100, double tolerancia = 1e-4);
    
    /**
     * @brief Obtém estatísticas sobre a última execução
     * @return String formatada com estatísticas
     */
    std::string obterEstatisticas() const;
    
private:
    const Deposito& deposito_;
    const Backlog& backlog_;
    const LocalizadorItens& localizador_;
    const VerificadorDisponibilidade& verificador_;
    
    // Parâmetros do Lagrangeano Aumentado
    std::vector<double> multiplicadoresLagrange_;
    std::vector<double> parametrosReforco_;
    
    // Pontos de quebra para linearização por partes
    std::vector<std::vector<PontoQuebra>> pontosQuebra_;
    
    // Estatísticas de execução
    int iteracoesRealizadas_;
    double tempoExecucao_;
    double valorObjetivo_;
    double gap_;
    
    // Estruturas para o modelo PLI
    std::vector<VariavelPLI> variaveis_;
    std::vector<RestricaoPLI> restricoes_;
    EstatisticasPLI estatisticas_;
    
    // Métodos auxiliares para construção e resolução do modelo
    void construirModeloPLI();
    void atualizarMultiplicadoresLagrange(const Solucao& solucao);
    double calcularViolacaoRestricoes(const Solucao& solucao);
    Solucao gerarSolucaoGulosa();
    Solucao ajustarViabilidade(const Solucao& solucao, int LB, int UB);
    double calcularValorObjetivo(const Solucao& solucao);
    double estimarLimiteSuperior();
    
    // Métodos para linearização por partes
    std::vector<PontoQuebra> gerarPontosQuebra(double min, double max, int numPontos);
    double aproximarFuncaoQuadratica(double valor, const std::vector<PontoQuebra>& pontos, 
                                    std::vector<double>& alphas);
};

/**
 * @brief Função principal para pré-processamento e resolução PLI
 * @param caminhoEntrada Caminho do arquivo de instância
 * @param diretorioSaida Diretório para salvar a solução
 */
void preprocessamentoPLI(const std::string& caminhoEntrada, const std::string& diretorioSaida);