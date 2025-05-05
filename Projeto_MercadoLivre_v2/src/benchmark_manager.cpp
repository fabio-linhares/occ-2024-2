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
    // Listar todas as instâncias no diretório
    std::vector<std::string> instancias;
    for (const auto& entry : std::filesystem::directory_iterator(diretorioInstancias)) {
        if (entry.is_regular_file()) {
            instancias.push_back(entry.path().filename().string());
        }
    }
    
    // Executar benchmark para cada instância
    for (const auto& instancia : instancias) {
        executarBenchmarkInstancia(instancia, repeticoes);
    }
    
    // Gerar relatório comparativo
    gerarRelatorioComparativo(diretorioResultados + "/relatorio_comparativo.txt");
    
    // Gerar gráficos
    gerarGraficosComparativos(diretorioResultados + "/graficos");
}

void BenchmarkManager::executarBenchmarkInstancia(const std::string& nomeInstancia, int repeticoes) {
    std::string caminhoInstancia = diretorioInstancias + "/" + nomeInstancia;
    std::vector<ResultadoBenchmark> resultados;
    
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
        std::vector<ResultadoBenchmark> resultadosAlgoritmo;
        
        // Realizar múltiplas execuções para obter média
        for (int rep = 0; rep < repeticoes; rep++) {
            ResultadoBenchmark resultado;
            resultado.nomeAlgoritmo = algoritmo;
            
            auto inicio = std::chrono::high_resolution_clock::now();
            
            // Executar o algoritmo apropriado
            if (algoritmo == "Dinkelbach") {
                OtimizadorDinkelbach otimizador(deposito, backlog, localizador, verificador);
                otimizador.configurarParametros(0.0001, 100, true);
                
                auto solucao = otimizador.otimizarWave(backlog.wave.LB, backlog.wave.UB);
                
                resultado.valorObjetivo = solucao.valorObjetivo;
                resultado.totalUnidades = 0;
                for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
                    const auto& itens = backlog.pedido[pedidoId];
                    for (const auto& par : itens) {
                        int itemId = par.first;
                        int quantidade = par.second;
                        resultado.totalUnidades += quantidade;
                    }
                }
                resultado.totalCorredores = solucao.corredoresWave.size();
                const auto& infoConvergencia = otimizador.obterInfoConvergencia();
                resultado.iteracoesRealizadas = infoConvergencia.iteracoesRealizadas;
                resultado.solucaoOtima = infoConvergencia.convergiu;
                
            } else if (algoritmo == "BuscaTabu") {
                // Gerar solução inicial básica usando Dinkelbach para ter uma boa semente
                OtimizadorDinkelbach otimizadorInicial(deposito, backlog, localizador, verificador);
                otimizadorInicial.configurarParametros(0.001, 10, false); // Parâmetros menos rigorosos para inicialização
                OtimizadorDinkelbach::SolucaoWave solucaoInicial = otimizadorInicial.otimizarWave(backlog.wave.LB, backlog.wave.UB);
                
                // Converter para formato da BuscaLocalAvancada
                BuscaLocalAvancada::Solucao solIni;
                solIni.pedidosWave = solucaoInicial.pedidosWave;
                solIni.corredoresWave = solucaoInicial.corredoresWave;
                solIni.valorObjetivo = solucaoInicial.valorObjetivo;
                solIni.totalUnidades = 0;
                for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
                    const auto& itens = backlog.pedido[pedidoId];
                    for (const auto& par : itens) {
                        int itemId = par.first;
                        int quantidade = par.second;
                        solIni.totalUnidades += quantidade;
                    }
                }
                
                BuscaLocalAvancada buscaLocal(deposito, backlog, localizador, verificador, 30.0);
                auto solucao = buscaLocal.otimizar(solIni, backlog.wave.LB, backlog.wave.UB, 
                                                 BuscaLocalAvancada::TipoBuscaLocal::BUSCA_TABU);
                
                resultado.valorObjetivo = solucao.valorObjetivo;
                resultado.totalUnidades = solucao.totalUnidades;
                resultado.totalCorredores = solucao.corredoresWave.size();
                // Outras estatísticas...
                
            } else if (algoritmo == "VNS") {
                // Gerar solução inicial básica usando Dinkelbach para ter uma boa semente
                OtimizadorDinkelbach otimizadorInicial(deposito, backlog, localizador, verificador);
                otimizadorInicial.configurarParametros(0.001, 10, false); // Parâmetros menos rigorosos para inicialização
                OtimizadorDinkelbach::SolucaoWave solucaoInicial = otimizadorInicial.otimizarWave(backlog.wave.LB, backlog.wave.UB);
                
                // Converter para formato da BuscaLocalAvancada
                BuscaLocalAvancada::Solucao solIni;
                solIni.pedidosWave = solucaoInicial.pedidosWave;
                solIni.corredoresWave = solucaoInicial.corredoresWave;
                solIni.valorObjetivo = solucaoInicial.valorObjetivo;
                solIni.totalUnidades = 0;
                for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
                    const auto& itens = backlog.pedido[pedidoId];
                    for (const auto& par : itens) {
                        int itemId = par.first;
                        int quantidade = par.second;
                        solIni.totalUnidades += quantidade;
                    }
                }
                
                BuscaLocalAvancada buscaLocal(deposito, backlog, localizador, verificador, 30.0);
                auto solucao = buscaLocal.otimizar(solIni, backlog.wave.LB, backlog.wave.UB, 
                                                 BuscaLocalAvancada::TipoBuscaLocal::VNS);
                
                resultado.valorObjetivo = solucao.valorObjetivo;
                resultado.totalUnidades = solucao.totalUnidades;
                resultado.totalCorredores = solucao.corredoresWave.size();
                // Outras estatísticas...
                
            } else if (algoritmo == "ILS") {
                // Gerar solução inicial básica usando Dinkelbach para ter uma boa semente
                OtimizadorDinkelbach otimizadorInicial(deposito, backlog, localizador, verificador);
                otimizadorInicial.configurarParametros(0.001, 10, false); // Parâmetros menos rigorosos para inicialização
                OtimizadorDinkelbach::SolucaoWave solucaoInicial = otimizadorInicial.otimizarWave(backlog.wave.LB, backlog.wave.UB);
                
                // Converter para formato da BuscaLocalAvancada
                BuscaLocalAvancada::Solucao solIni;
                solIni.pedidosWave = solucaoInicial.pedidosWave;
                solIni.corredoresWave = solucaoInicial.corredoresWave;
                solIni.valorObjetivo = solucaoInicial.valorObjetivo;
                solIni.totalUnidades = 0;
                for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
                    const auto& itens = backlog.pedido[pedidoId];
                    for (const auto& par : itens) {
                        int itemId = par.first;
                        int quantidade = par.second;
                        solIni.totalUnidades += quantidade;
                    }
                }
                
                BuscaLocalAvancada buscaLocal(deposito, backlog, localizador, verificador, 30.0);
                auto solucao = buscaLocal.otimizar(solIni, backlog.wave.LB, backlog.wave.UB, 
                                                 BuscaLocalAvancada::TipoBuscaLocal::ILS);
                
                resultado.valorObjetivo = solucao.valorObjetivo;
                resultado.totalUnidades = solucao.totalUnidades;
                resultado.totalCorredores = solucao.corredoresWave.size();
                // Outras estatísticas...
            }
            
            auto fim = std::chrono::high_resolution_clock::now();
            resultado.tempoExecucaoMs = std::chrono::duration<double, std::milli>(fim - inicio).count();
            
            resultadosAlgoritmo.push_back(resultado);
        }
        
        // Calcular médias e salvar
        ResultadoBenchmark mediaResultado;
        mediaResultado.nomeAlgoritmo = algoritmo;
        mediaResultado.valorObjetivo = 0;
        mediaResultado.totalUnidades = 0;
        mediaResultado.totalCorredores = 0;
        mediaResultado.tempoExecucaoMs = 0;
        mediaResultado.iteracoesRealizadas = 0;
        
        for (const auto& res : resultadosAlgoritmo) {
            mediaResultado.valorObjetivo += res.valorObjetivo;
            mediaResultado.totalUnidades += res.totalUnidades;
            mediaResultado.totalCorredores += res.totalCorredores;
            mediaResultado.tempoExecucaoMs += res.tempoExecucaoMs;
            mediaResultado.iteracoesRealizadas += res.iteracoesRealizadas;
        }
        
        mediaResultado.valorObjetivo /= repeticoes;
        mediaResultado.totalUnidades /= repeticoes;
        mediaResultado.totalCorredores /= repeticoes;
        mediaResultado.tempoExecucaoMs /= repeticoes;
        mediaResultado.iteracoesRealizadas /= repeticoes;
        
        // Salvar resultado médio para esta instância e algoritmo
        resultadosPorInstancia[nomeInstancia].push_back(mediaResultado);
    }
    
    // Salvar resultados desta instância em um arquivo
    std::ofstream outFile(diretorioResultados + "/" + nomeInstancia + "_benchmark.txt");
    outFile << "Algoritmo,ValorObjetivo,TotalUnidades,TotalCorredores,TempoExecucaoMs,Iteracoes\n";
    
    for (const auto& resultado : resultadosPorInstancia[nomeInstancia]) {
        outFile << resultado.nomeAlgoritmo << ","
                << resultado.valorObjetivo << ","
                << resultado.totalUnidades << ","
                << resultado.totalCorredores << ","
                << resultado.tempoExecucaoMs << ","
                << resultado.iteracoesRealizadas << "\n";
    }
    
    outFile.close();
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
    
    // Esta implementação geraria scripts para ferramentas como gnuplot ou pyplot
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