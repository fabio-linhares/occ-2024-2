#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>
#include "armazem.h"
#include "solucionar_desafio.h"

/**
 * @brief Estrutura para armazenar resultados de benchmark
 */
struct ResultadoBenchmark {
    std::string nomeAlgoritmo;
    std::string nomeInstancia;         // Adicionado para facilitar referência cruzada
    double valorObjetivo;
    int totalUnidades;
    int totalCorredores;
    double tempoExecucaoMs;
    int iteracoesRealizadas;
    bool solucaoOtima;
    double gapOtimalidade;
    std::string timestamp;             // Adicionado para rastreabilidade
    double memoriaPico = 0;            // Adicionado para monitorar uso de memória
    std::vector<double> historicoValores; // Adicionado para acompanhar convergência
};

/**
 * @brief Configuração para execução de benchmarks
 */
struct BenchmarkConfig {
    int repeticoes = 5;
    double limiteTempo = 60.0;         // Segundos por execução
    bool validarSolucoes = true;       // Validar soluções geradas
    bool executarEmParalelo = true;    // Usar paralelismo
    int numThreads = 0;                // 0 = automático
    bool compararComBOV = true;        // Comparar com BOVs oficiais
    std::string formatoRelatorio = "md"; // Formato do relatório (md, txt, html)
};

/**
 * @brief Estrutura para calcular métricas estatísticas por algoritmo
 */
struct EstatisticasAlgoritmo {
    double valorMedio;
    double desvioPadrao;
    double melhorValor;
    double piorValor;
    double tempoMedio;
    double taxaExito;       // Percentual de vezes que encontrou solução viável
    double percentualBOV;   // Média do percentual relativo ao BOV oficial
};

EstatisticasAlgoritmo calcularEstatisticas(const std::string& algoritmo);

/**
 * @brief Classe para gerenciamento de benchmarks de algoritmos
 */
class BenchmarkManager {
private:
    std::map<std::string, std::vector<ResultadoBenchmark>> resultadosPorInstancia;
    std::vector<std::string> algoritmosDisponiveis;
    std::string diretorioInstancias;
    std::string diretorioResultados;
    BenchmarkConfig config_;
    std::map<std::string, double> bovsOficiais_;
    
    // Callback para progressos
    std::function<void(const std::string&, double)> progressCallback_;
    
public:
    /**
     * @brief Construtor
     * @param dirInstancias Diretório contendo as instâncias
     * @param dirResultados Diretório para salvar resultados
     */
    BenchmarkManager(const std::string& dirInstancias, const std::string& dirResultados);
    
    /**
     * @brief Configura o gerenciador de benchmarks
     * @param config Configuração para execução de benchmarks
     */
    void configurar(const BenchmarkConfig& config);
    
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
    
    /**
     * @brief Define um callback para progresso
     * @param callback Função de callback
     */
    void definirCallbackProgresso(std::function<void(const std::string&, double)> callback);
    
    /**
     * @brief Carrega BOVs oficiais a partir de um arquivo
     * @param arquivoBOVs Caminho para o arquivo de BOVs
     */
    void carregarBOVsOficiais(const std::string& arquivoBOVs);
    
    /**
     * @brief Salva os resultados em um arquivo
     * @param arquivo Caminho para o arquivo de saída
     */
    void salvarResultados(const std::string& arquivo);
    
    /**
     * @brief Carrega resultados de um arquivo
     * @param arquivo Caminho para o arquivo de entrada
     */
    void carregarResultados(const std::string& arquivo);
    
    /**
     * @brief Exporta resultados em um formato específico
     * @param arquivo Caminho para o arquivo de saída
     * @param formato Formato de exportação (padrão: csv)
     */
    void exportarResultados(const std::string& arquivo, const std::string& formato = "csv");
    
    /**
     * @brief Obtém resultados para uma instância específica
     * @param instancia Nome da instância
     * @return Vetor de resultados
     */
    std::vector<ResultadoBenchmark> obterResultadosInstancia(const std::string& instancia);
    
    /**
     * @brief Obtém resultados para um algoritmo específico
     * @param algoritmo Nome do algoritmo
     * @return Vetor de resultados
     */
    std::vector<ResultadoBenchmark> obterResultadosAlgoritmo(const std::string& algoritmo);
    
    /**
     * @brief Limpa todos os resultados armazenados
     */
    void limparResultados();
    
    /**
     * @brief Para a execução de benchmarks
     */
    void pararExecucao();
    
    /**
     * @brief Verifica se há execução em progresso
     * @return Verdadeiro se há execução em progresso
     */
    bool execucaoEmProgresso() const;
    
    /**
     * @brief Calcula o desvio padrão para um algoritmo
     * @param algoritmo Nome do algoritmo
     * @return Desvio padrão calculado
     */
    double calcularDesvioPadrao(const std::string& algoritmo) const;
    
    /**
     * @brief Gera uma matriz comparativa entre algoritmos
     * @param arquivoSaida Caminho para o arquivo de saída
     */
    void gerarMatrizComparativa(const std::string& arquivoSaida);
    
    /**
     * @brief Gera um perfil de desempenho dos algoritmos
     * @param arquivoSaida Caminho para o arquivo de saída
     */
    void gerarPerfilDesempenho(const std::string& arquivoSaida);
    
    /**
     * @brief Gerar visualizações avançadas
     */
    void gerarGraficoConvergencia(const std::string& instancia, const std::string& algoritmo, const std::string& arquivo);
    void gerarHeatmapDesempenho(const std::string& arquivo);
    void gerarComparativoAlgoritmos3D(const std::string& arquivo);

    // Exportar para ferramentas externas
    void exportarParaTableau(const std::string& arquivo);
    void exportarParaJupyter(const std::string& arquivo);
};