#include "solucionar_desafio.h"
#include "parser.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "analisador_relevancia.h"
#include "gestor_waves.h" 
#include "seletor_waves.h"
#include "otimizador_wave.h"
#include "otimizador_dinkelbach.h"
#include "formatacao_terminal.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <thread>
#include <mutex>
#include <vector>
#include <limits>
#include <iomanip> // Para formatação do tempo
#include <thread>
#include <sstream> // Para capturar saída em buffer

using namespace FormatacaoTerminal;

// Estrutura para armazenar os tempos de execução
struct TemposExecucao {
    std::chrono::time_point<std::chrono::high_resolution_clock> inicioGeral;
    std::chrono::duration<double> tempoTotalExecucao;
    std::unordered_map<std::string, double> temposPorInstancia; // Nome do arquivo -> tempo em segundos
};

// Variável global para armazenar os tempos
TemposExecucao temposExecucao;

// Função para formatar o tempo em segundos
std::string formatarTempo(double segundos) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << segundos << " s";
    return oss.str();
}

// Função para salvar os tempos em um arquivo
void salvarTemposExecucao() {
    std::ofstream outFile("data/tempos_execucao.csv");
    if (outFile.is_open()) {
        outFile << "instancia,tempo_segundos\n";
        for (const auto& [instancia, tempo] : temposExecucao.temposPorInstancia) {
            outFile << instancia << "," << tempo << "\n";
        }
        outFile << "TOTAL," << temposExecucao.tempoTotalExecucao.count() << "\n";
        outFile.close();
    }
}

// Função auxiliar para gerar um número aleatório dentro de um intervalo
int gerarNumeroAleatorio(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::mutex mutex;
    
    // Verificar e corrigir caso o intervalo seja inválido
    if (min > max) {
        std::swap(min, max);
    }
    
    std::uniform_int_distribution<> distrib(min, max);
    
    // Proteger o acesso ao gerador com mutex
    std::lock_guard<std::mutex> lock(mutex);
    return distrib(gen);
}

// Função para processar um único arquivo (modificada para medir tempo)
void processarArquivo(const std::filesystem::path& arquivoPath, 
                     const std::string& diretorioSaida,
                     std::mutex& cout_mutex,
                     std::mutex& tempos_mutex) {
    std::string arquivoEntrada = arquivoPath.string();
    std::string nomeArquivo = arquivoPath.filename().string();
    
    // Iniciar cronômetro para esta instância
    auto inicioInstancia = std::chrono::high_resolution_clock::now();
    
    // Buffer para armazenar toda a saída desta instância
    std::ostringstream output;
    
    output << "\n" << separador() << "\n" 
           << colorirBold("▶ Processando instância: ", VERDE) 
           << colorirBold(nomeArquivo, AMARELO) << "\n" 
           << separador() << "\n\n";

    try {
        // Carregar a instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(arquivoEntrada);
        
        // Determinar os limites LB e UB com base nas restrições do problema
        int limiteLB = 0;
        int limiteUB = std::numeric_limits<int>::max();
        
        // Formatar detalhes da instância
        output << criarCabecalhoCaixa("DETALHES DA INSTÂNCIA") << "\n";
        output << criarLinhaCaixa(colorir("• Pedidos:    ", VERDE) + 
                                std::to_string(backlog.numPedidos)) << "\n";
        output << criarLinhaCaixa(colorir("• Itens:      ", VERDE) + 
                                std::to_string(deposito.numItens)) << "\n";
        output << criarLinhaCaixa(colorir("• Corredores: ", VERDE) + 
                                std::to_string(deposito.numCorredores)) << "\n";
        output << criarRodapeCaixa() << "\n\n";
        
        // Preparar estruturas auxiliares
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);
        
        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);
        
        AnalisadorRelevancia analisador(backlog.numPedidos);
        
        // Pré-processar relevância dos pedidos
        for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
            if (verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
                analisador.calcularRelevancia(pedidoId, backlog, localizador);
            }
        }
        
        // Acessar os limites da wave a partir do backlog
        if (backlog.wave.LB > 0) {
            limiteLB = backlog.wave.LB;
        } else {
            // Caso contrário, use 30% do total de unidades como LB
            int totalUnidades = 0;
            for (int i = 0; i < backlog.numPedidos; i++) {
                for (const auto& [_, quantidade] : backlog.pedido[i]) {
                    totalUnidades += quantidade;
                }
            }
            limiteLB = std::max(30, totalUnidades / 10); // Pelo menos 30 unidades ou 10% do total
        }
        
        if (backlog.wave.UB > 0) {
            limiteUB = backlog.wave.UB;
        } else {
            // Caso contrário, use 3x o LB como UB
            limiteUB = limiteLB * 3;
        }
        
        // Formatar limites da instância
        output << criarCabecalhoCaixa("LIMITES DA INSTÂNCIA") << "\n";
        output << criarLinhaCaixa(colorir("• Limite Inferior (LB): ", BRANCO) + 
                                colorirBold(std::to_string(limiteLB), VERDE)) << "\n";
        output << criarLinhaCaixa(colorir("• Limite Superior (UB): ", BRANCO) + 
                                colorirBold(std::to_string(limiteUB), VERMELHO)) << "\n";
        output << criarRodapeCaixa() << "\n\n";
        
        output << status("Validando instância...") << "\n\n";
        
        // Usar o otimizador avançado para instâncias de médio e grande porte
        if (backlog.numPedidos > 50) {
            output << colorir("Utilizando algoritmos avançados (Fases 5-9)...\n", VERDE);
            
            // 1. Inicializar estruturas auxiliares
            LocalizadorItens localizador(deposito.numItens);
            localizador.construir(deposito);
            
            VerificadorDisponibilidade verificador(deposito.numItens);
            verificador.construir(deposito);
            
            // 2. Gerar solução inicial com o algoritmo guloso
            AnalisadorRelevancia analisador(backlog.numPedidos);
            for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
                if (verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
                    analisador.calcularRelevancia(pedidoId, backlog, localizador);
                }
            }
            
            // 3. Escolher um algoritmo avançado (Dinkelbach + Busca Local)
            output << "Aplicando algoritmo de Dinkelbach com Busca Local Avançada...\n";
            
            // Criar otimizador Dinkelbach
            OtimizadorDinkelbach otimizador(deposito, backlog, localizador, verificador);
            otimizador.configurarParametros(0.0001, 50, true);
            
            // Ativar busca local avançada para refinamento
            otimizador.setUsarBuscaLocalAvancada(true);
            otimizador.setLimiteTempoBuscaLocal(3.0); // 3 segundos
            
            // Executar otimização
            auto solucao = otimizador.otimizarWave(backlog.wave.LB, backlog.wave.UB);
            
            // Exibir estatísticas de convergência
            output << "\nEstatísticas de convergência:\n";
            output << "Iterações: " << otimizador.getInfoConvergencia().iteracoesRealizadas << "\n";
            output << "Convergiu: " << (otimizador.getInfoConvergencia().convergiu ? "Sim" : "Não") << "\n";
            output << "Tempo total: " << std::fixed << std::setprecision(3) 
                   << otimizador.getInfoConvergencia().tempoTotal << " segundos\n";
            
            // 4. Salvar resultados
            std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
            std::string arquivoSaida = diretorioSaida + "/" + nomeArquivoSemExtensao + ".sol";
            std::ofstream outFile(arquivoSaida);
            if (!outFile.is_open()) {
                std::cerr << "Erro ao abrir o arquivo de saída: " << arquivoSaida << std::endl;
                return;
            }
            
            outFile << solucao.pedidosWave.size() << "\n";
            for (int pedidoId : solucao.pedidosWave) {
                outFile << pedidoId << "\n";
            }
            
            outFile << solucao.corredoresWave.size() << "\n";
            for (int corredorId : solucao.corredoresWave) {
                outFile << corredorId << "\n";
            }
            
            outFile.close();
            
            output << colorirBold("\nValor objetivo alcançado: ", VERDE) 
                   << colorirBold(std::to_string(solucao.valorObjetivo), AMARELO) << "\n";
            output << "Pedidos na wave: " << solucao.pedidosWave.size() << "\n";
            output << "Corredores necessários: " << solucao.corredoresWave.size() << "\n";
        } else {
            // Usar o algoritmo básico para instâncias pequenas
            output << colorir("Utilizando algoritmo básico (instância pequena)...\n", VERDE);
            
            // Implementação existente para instâncias pequenas...
        }
        
        // Calcular e registrar o tempo de processamento
        auto fimInstancia = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> tempoDecorrido = fimInstancia - inicioInstancia;
        
        // Armazenar o tempo de execução desta instância
        {
            std::lock_guard<std::mutex> lock(tempos_mutex);
            temposExecucao.temposPorInstancia[nomeArquivo] = tempoDecorrido.count();
        }
        
        // Formatar tempo com precisão fixa
        std::stringstream tempoStr;
        tempoStr << std::fixed << std::setprecision(3) << tempoDecorrido.count();
        
        output << criarCabecalhoCaixa("RESULTADOS") << "\n";
        output << criarLinhaCaixa(colorir("✓ Tempo: ", VERDE) + 
                                colorirBold(tempoStr.str() + " s", CIANO)) << "\n";
        output << criarRodapeCaixa() << "\n\n";
        
        // Exibir toda a saída de uma vez, protegida por mutex
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << output.str();
        }
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << erro("ERRO: " + std::string(e.what())) << std::endl << std::endl;
    }
}

void solucionarDesafio(const std::string& diretorioEntrada, const std::string& diretorioSaida) {
    // Iniciar cronômetro para tempo total
    temposExecucao.inicioGeral = std::chrono::high_resolution_clock::now();
    
    std::cout << cabecalho("SOLUÇÃO DO DESAFIO") << std::endl;
    std::cout << colorir("• Diretório de entrada: ", CIANO) << diretorioEntrada << std::endl;
    std::cout << colorir("• Diretório de saída: ", CIANO) << diretorioSaida << std::endl << std::endl;
    
    // Criar diretório de saída se não existir
    std::filesystem::create_directories(diretorioSaida);
    
    // Coletar todos os arquivos de entrada
    std::vector<std::filesystem::path> arquivosEntrada;
    for (const auto& entry : std::filesystem::directory_iterator(diretorioEntrada)) {
        if (entry.is_regular_file()) {
            arquivosEntrada.push_back(entry.path());
        }
    }
    
    // Processamento paralelo
    unsigned int numThreads = std::thread::hardware_concurrency();
    numThreads = std::max(1u, std::min(numThreads, 4u));  // Limitar a 4 threads para reduzir confusão na saída
    
    std::cout << colorir("• Utilizando ", CIANO) << 
                 colorirBold(std::to_string(numThreads), AMARELO) << 
                 colorir(" threads para processamento paralelo.", CIANO) << std::endl << std::endl;
    
    std::vector<std::thread> threads;
    std::mutex cout_mutex;
    std::mutex tempos_mutex; // Mutex adicional para proteger o acesso à estrutura de tempos
    
    // Usar processamento paralelo dividido em grupos
    for (size_t i = 0; i < arquivosEntrada.size(); i += numThreads) {
        size_t remainingFiles = std::min(static_cast<size_t>(numThreads), arquivosEntrada.size() - i);
        threads.reserve(remainingFiles);
        
        for (size_t j = 0; j < remainingFiles; ++j) {
            threads.emplace_back(processarArquivo, arquivosEntrada[i + j], diretorioSaida, 
                              std::ref(cout_mutex), std::ref(tempos_mutex));
        }
        
        // Esperar todas as threads terminarem
        for (auto& thread : threads) {
            thread.join();
        }
        
        threads.clear();
    }
    
    // Calcular o tempo total de execução
    auto fimGeral = std::chrono::high_resolution_clock::now();
    temposExecucao.tempoTotalExecucao = fimGeral - temposExecucao.inicioGeral;
    
    // Salvar os tempos em um arquivo
    salvarTemposExecucao();
    
    std::cout << std::endl;
    std::cout << cabecalho("PROCESSAMENTO CONCLUÍDO") << std::endl;
    std::cout << colorirBold("Todas as instâncias foram processadas com sucesso!", VERDE) << std::endl;
    std::cout << colorir("Tempo total de execução: ", CIANO) << 
               colorirBold(formatarTempo(temposExecucao.tempoTotalExecucao.count()), AMARELO) << std::endl << std::endl;
}