#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "armazem.h"
#include "solucionar_desafio.h"

/**
 * @brief Estrutura para armazenar resultados de benchmark de um algoritmo
 */
struct ResultadoBenchmark {
    std::string nomeAlgoritmo;
    double valorObjetivo;
    int totalUnidades;
    int totalCorredores;
    double tempoExecucaoMs;
    int iteracoesRealizadas;
    bool solucaoOtima;
    double gapOtimalidade;
};

/**
 * @brief Classe para gerenciamento de benchmarks de algoritmos
 */
class BenchmarkManager {
private:
    std::map<std::string, std::vector<ResultadoBenchmark>> resultadosPorInstancia;
    std::vector<std::string> algoritmosDisponiveis;
    std::string diretorioInstancias;
    std::string diretorioResultados;
    
public:
    /**
     * @brief Construtor
     * @param dirInstancias Diretório contendo as instâncias
     * @param dirResultados Diretório para salvar resultados
     */
    BenchmarkManager(const std::string& dirInstancias, const std::string& dirResultados);
    
    /**
     * @brief Adiciona um algoritmo ao conjunto de benchmarking
     * @param nomeAlgoritmo Nome identificador do algoritmo
     */
    void adicionarAlgoritmo(const std::string& nomeAlgoritmo);
    
    /**
     * @brief Executa o benchmark completo em todas as instâncias e algoritmos
     * @param repeticoes Número de repetições para cada par instância-algoritmo
     */
    void executarBenchmarkCompleto(int repeticoes = 5);
    
    /**
     * @brief Executa benchmark para uma instância específica
     * @param nomeInstancia Nome do arquivo da instância
     * @param repeticoes Número de repetições
     */
    void executarBenchmarkInstancia(const std::string& nomeInstancia, int repeticoes = 5);
    
    /**
     * @brief Gera relatório comparativo entre algoritmos
     * @param arquivoSaida Caminho para o arquivo de saída
     */
    void gerarRelatorioComparativo(const std::string& arquivoSaida);
    
    /**
     * @brief Gera gráficos comparativos de desempenho
     * @param diretorioGraficos Diretório para salvar os gráficos
     */
    void gerarGraficosComparativos(const std::string& diretorioGraficos);
    
    /**
     * @brief Analisa padrões de instâncias e recomenda algoritmos
     * @return Mapa relacionando características de instância ao melhor algoritmo
     */
    std::map<std::string, std::string> analisarPadroesDesempenho();
};