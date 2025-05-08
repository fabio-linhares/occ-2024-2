#include "benchmark_manager.h"
#include "parser.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "otimizador_dinkelbach.h"
#include "busca_local_avancada.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <functional>
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>



BenchmarkManager::BenchmarkManager(const std::string& dirInstancias, const std::string& dirResultados)
    : diretorioInstancias(dirInstancias), diretorioResultados(dirResultados) {
    
    // Criar diretório de resultados se não existir
    std::filesystem::create_directories(dirResultados);
}

void BenchmarkManager::adicionarAlgoritmo(const std::string& nomeAlgoritmo) {
    // Verificar se o algoritmo já foi adicionado
    if (std::find(algoritmosDisponiveis.begin(), algoritmosDisponiveis.end(), nomeAlgoritmo) 
            == algoritmosDisponiveis.end()) {
        algoritmosDisponiveis.push_back(nomeAlgoritmo);
    }
}

void BenchmarkManager::executarBenchmarkCompleto(int repeticoes) {
    // Verificar se o diretório existe
    if (!std::filesystem::exists(diretorioInstancias)) {
        std::cerr << "Erro: O diretório de instâncias '" << diretorioInstancias << "' não existe." << std::endl;
        return;
    }
    
    // Listar todas as instâncias no diretório
    std::vector<std::string> instancias;
    for (const auto& entry : std::filesystem::directory_iterator(diretorioInstancias)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            instancias.push_back(entry.path().string());
        }
    }
    
    if (instancias.empty()) {
        std::cerr << "Erro: Não foram encontradas instâncias válidas no diretório '" 
                  << diretorioInstancias << "'." << std::endl;
        std::cerr << "As instâncias devem ser arquivos .txt com o formato correto." << std::endl;
        return;
    }
    
    // Executar benchmark para cada instância
    for (const auto& instancia : instancias) {
        try {
            executarBenchmarkInstancia(std::filesystem::path(instancia).filename().string(), repeticoes);
        } catch (const std::exception& e) {
            std::cerr << "Erro ao processar instância '" << instancia << "': " << e.what() << std::endl;
            continue; // Continuar com a próxima instância
        }
    }
    
    // Gerar relatório comparativo
    gerarRelatorioComparativo(diretorioResultados + "/relatorio_comparativo.txt");
    
    // Gerar gráficos
    gerarGraficosComparativos(diretorioResultados + "/graficos");
}

void BenchmarkManager::executarBenchmarkInstancia(const std::string& nomeInstancia, int repeticoes) {
    std::string caminhoInstancia = diretorioInstancias + "/" + nomeInstancia;
    std::vector<ResultadoBenchmark> resultados;
    
    // Verificar se o arquivo existe
    if (!std::filesystem::exists(caminhoInstancia)) {
        std::cerr << "Erro: O arquivo de instância '" << caminhoInstancia << "' não existe." << std::endl;
        return;
    }
    
    try {
        // Carregar a instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(caminhoInstancia);
        
        // Preparar estruturas auxiliares
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);
        
        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);
        
        // Para cada algoritmo disponível
        for (const auto& algoritmo : algoritmosDisponiveis) {
            // Implementação dos benchmarks para cada algoritmo
            // ...
        }
        
        // Salvar resultados desta instância em um arquivo
        std::ofstream outFile(diretorioResultados + "/" + nomeInstancia + "_benchmark.txt");
        if (outFile.is_open()) {
            outFile << "Algoritmo,ValorObjetivo,TotalUnidades,TotalCorredores,TempoExecucaoMs,Iteracoes\n";
            
            for (const auto& resultado : resultadosPorInstancia[nomeInstancia]) {
                // Salvar dados do resultado
                // ...
            }
            
            outFile.close();
        } else {
            std::cerr << "Erro: Não foi possível criar o arquivo de resultados." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Erro ao processar instância '" << nomeInstancia << "': " << e.what() << std::endl;
    }
}

void BenchmarkManager::gerarRelatorioComparativo(const std::string& arquivoSaida) {
    std::ofstream outFile(arquivoSaida);
    
    // Cabeçalho do relatório
    outFile << "# Relatório Comparativo de Algoritmos\n\n";
    outFile << "## Resumo de Desempenho\n\n";
    
    // Tabela de resumo por algoritmo (média de todas as instâncias)
    outFile << "| Algoritmo | Valor Objetivo Médio | Tempo Médio (ms) | Melhoria (%)\n";
    outFile << "|-----------|----------------------|-----------------|------------\n";
    
    std::map<std::string, double> valorMedioPorAlgoritmo;
    std::map<std::string, double> tempoMedioPorAlgoritmo;
    
    // Calcular médias por algoritmo
    for (const auto& [instancia, resultados] : resultadosPorInstancia) {
        for (const auto& resultado : resultados) {
            valorMedioPorAlgoritmo[resultado.nomeAlgoritmo] += resultado.valorObjetivo;
            tempoMedioPorAlgoritmo[resultado.nomeAlgoritmo] += resultado.tempoExecucaoMs;
        }
    }
    
    // Número de instâncias
    int numInstancias = resultadosPorInstancia.size();
    
    // Calcular valores médios finais
    for (auto& [algoritmo, valorTotal] : valorMedioPorAlgoritmo) {
        valorTotal /= numInstancias;
    }
    
    for (auto& [algoritmo, tempoTotal] : tempoMedioPorAlgoritmo) {
        tempoTotal /= numInstancias;
    }
    
    // Encontrar o melhor valor médio para calcular melhoria
    double melhorValorMedio = 0;
    if (!valorMedioPorAlgoritmo.empty()) {
        melhorValorMedio = std::max_element(
            valorMedioPorAlgoritmo.begin(),
            valorMedioPorAlgoritmo.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
        )->second;
    }
    
    // Gerar linhas da tabela
    for (const auto& algoritmo : algoritmosDisponiveis) {
        double valorMedio = valorMedioPorAlgoritmo[algoritmo];
        double tempoMedio = tempoMedioPorAlgoritmo[algoritmo];
        double melhoria = 0;
        
        if (melhorValorMedio > 0) {
            melhoria = (valorMedio / melhorValorMedio) * 100.0;
        }
        
        outFile << "| " << algoritmo << " | " 
                << std::fixed << std::setprecision(2) << valorMedio << " | "
                << std::fixed << std::setprecision(2) << tempoMedio << " | "
                << std::fixed << std::setprecision(2) << melhoria << " |\n";
    }
    
    // Detalhes por instância
    outFile << "\n## Desempenho por Instância\n\n";
    
    for (const auto& [instancia, resultados] : resultadosPorInstancia) {
        outFile << "### Instância: " << instancia << "\n\n";
        
        outFile << "| Algoritmo | Valor Objetivo | Total Unidades | Total Corredores | Tempo (ms) |\n";
        outFile << "|-----------|----------------|----------------|------------------|------------|\n";
        
        for (const auto& resultado : resultados) {
            outFile << "| " << resultado.nomeAlgoritmo << " | "
                    << std::fixed << std::setprecision(2) << resultado.valorObjetivo << " | "
                    << resultado.totalUnidades << " | "
                    << resultado.totalCorredores << " | "
                    << std::fixed << std::setprecision(2) << resultado.tempoExecucaoMs << " |\n";
        }
        
        outFile << "\n";
    }
    
    outFile.close();
}

void BenchmarkManager::gerarGraficosComparativos(const std::string& diretorioGraficos) {
    // Criar diretório para gráficos
    std::filesystem::create_directories(diretorioGraficos);

    // Implementação futura...
    
    // Gerar scripts para ferramentas como gnuplot ou pyplot
    // para criar visualizações dos resultados de benchmark
    
    // Para cada tipo de gráfico (tempo x valor objetivo, perfil de desempenho, etc.)
    // geraremos um arquivo de script e os dados correspondentes
    
    // Exemplo: gráfico de barras de valor objetivo por algoritmo
    std::ofstream dadosFile(diretorioGraficos + "/valor_objetivo_por_algoritmo.dat");
    dadosFile << "# Algoritmo ValorObjetivo\n";
    
    for (const auto& algoritmo : algoritmosDisponiveis) {
        double valorMedio = 0.0;
        int count = 0;
        
        for (const auto& [instancia, resultados] : resultadosPorInstancia) {
            for (const auto& resultado : resultados) {
                if (resultado.nomeAlgoritmo == algoritmo) {
                    valorMedio += resultado.valorObjetivo;
                    count++;
                    break;
                }
            }
        }
        
        if (count > 0) {
            valorMedio /= count;
            dadosFile << algoritmo << " " << valorMedio << "\n";
        }
    }
    
    dadosFile.close();
    
    // Script gnuplot para criar o gráfico
    std::ofstream scriptFile(diretorioGraficos + "/gerar_grafico_valor_objetivo.gp");
    scriptFile << "set terminal png size 800,600\n";
    scriptFile << "set output 'valor_objetivo_por_algoritmo.png'\n";
    scriptFile << "set title 'Valor Objetivo Médio por Algoritmo'\n";
    scriptFile << "set style data histogram\n";
    scriptFile << "set style histogram cluster gap 1\n";
    scriptFile << "set style fill solid border -1\n";
    scriptFile << "set boxwidth 0.9\n";
    scriptFile << "set xtic rotate by -45 scale 0\n";
    scriptFile << "set ylabel 'Valor Objetivo'\n";
    scriptFile << "plot 'valor_objetivo_por_algoritmo.dat' using 2:xtic(1) title ''\n";
    
    scriptFile.close();
    
    // Outros gráficos similares seriam gerados aqui...
}

std::map<std::string, std::string> BenchmarkManager::analisarPadroesDesempenho() {
    std::map<std::string, std::string> recomendacoes;
    
    // Para cada instância, determinar o melhor algoritmo
    for (const auto& [instancia, resultados] : resultadosPorInstancia) {
        std::string melhorAlgoritmo;
        double melhorValor = -std::numeric_limits<double>::infinity();
        
        for (const auto& resultado : resultados) {
            if (resultado.valorObjetivo > melhorValor) {
                melhorValor = resultado.valorObjetivo;
                melhorAlgoritmo = resultado.nomeAlgoritmo;
            }
        }
        
        if (!melhorAlgoritmo.empty()) {
            recomendacoes[instancia] = melhorAlgoritmo;
        }
    }
    
    return recomendacoes;
}