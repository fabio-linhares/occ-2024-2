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
        
        // ======= Escolha do otimizador baseado no tamanho da instância =======
        
        // Extrair número de instância do nome do arquivo
        std::string instanciaStr = nomeArquivo;
        std::transform(instanciaStr.begin(), instanciaStr.end(), instanciaStr.begin(), ::tolower);
        
        // Decidir qual otimizador usar com base na instância
        bool usarDinkelbach = false;
        
        // Determinar o tamanho da instância para decidir qual otimizador usar
        int totalUnidades = 0;
        std::unordered_set<int> itensUnicos;

        for (int i = 0; i < backlog.numPedidos; i++) {
            for (const auto& [itemId, quantidade] : backlog.pedido[i]) {
                totalUnidades += quantidade;
                itensUnicos.insert(itemId);
            }
        }

        // Calcular estatísticas da instância
        int numItensUnicos = itensUnicos.size();
        int numCorredoresUnicos = 0;
        std::unordered_set<int> corredoresSet;

        // Obter número de corredores utilizados usando LocalizadorItens
        for (int itemId : itensUnicos) {
            const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
            for (const auto& [corredorId, _] : corredoresComItem) {
                corredoresSet.insert(corredorId);
            }
        }
        numCorredoresUnicos = corredoresSet.size();

        // Usar Dinkelbach para instâncias menores
        bool complexidadeBaixa = (
            backlog.numPedidos < 200 &&
            numItensUnicos < 300 &&
            numCorredoresUnicos < 50 &&
            totalUnidades < 5000
        );

        usarDinkelbach = complexidadeBaixa;
        
        // Estrutura comum para armazenar soluções de ambos os otimizadores
        struct SolucaoComum {
            std::vector<int> pedidosWave;
            std::vector<int> corredoresWave;
            double valorObjetivo;
        };
        
        // Soluções alternativas para comparar
        SolucaoComum melhorWave;
        
        std::string nomeOtimizador;
        if (usarDinkelbach) {
            nomeOtimizador = "Dinkelbach";
            output << criarCabecalhoCaixa("MÉTODO DE OTIMIZAÇÃO") << "\n";
            output << criarLinhaCaixa(colorir("• Algoritmo: ", BRANCO) + 
                                    colorirBold(nomeOtimizador, CIANO)) << "\n";
            output << criarRodapeCaixa() << "\n\n";
            
            OtimizadorDinkelbach otimizador(deposito, backlog, localizador, verificador);
            
            // Configurar parâmetros baseados no tamanho da instância
            bool usarBranchAndBound = (backlog.numPedidos <= 200);
            double epsilon = 0.0001;
            int maxIteracoes = 100;
            
            // Ajustar parâmetros para instâncias maiores
            if (backlog.numPedidos > 500) {
                epsilon = 0.001;
                maxIteracoes = 50;
            }
            
            otimizador.configurarParametros(epsilon, maxIteracoes, usarBranchAndBound);
            
            auto solucao = otimizador.otimizarWave(limiteLB, limiteUB);
            melhorWave.pedidosWave = solucao.pedidosWave;
            melhorWave.corredoresWave = solucao.corredoresWave;
            melhorWave.valorObjetivo = solucao.valorObjetivo;
            
            // Se a instância for pequena, mostrar detalhes de convergência
            if (backlog.numPedidos <= 100) {
                const auto& infoConvergencia = otimizador.getInfoConvergencia();
                
                output << criarCabecalhoCaixa("DETALHES DE CONVERGÊNCIA") << "\n";
                output << criarLinhaCaixa(colorir("• Iterações: ", BRANCO) + 
                                        colorirBold(std::to_string(infoConvergencia.iteracoesRealizadas), VERDE)) << "\n";
                output << criarLinhaCaixa(colorir("• Convergiu: ", BRANCO) + 
                                        colorirBold(infoConvergencia.convergiu ? "Sim" : "Não", infoConvergencia.convergiu ? VERDE : AMARELO)) << "\n";
                
                std::stringstream ss;
                ss << std::fixed << std::setprecision(4) << infoConvergencia.tempoTotal;
                output << criarLinhaCaixa(colorir("• Tempo: ", BRANCO) + 
                                        colorirBold(ss.str() + " segundos", CIANO)) << "\n";
                output << criarRodapeCaixa() << "\n\n";
            }
        } else {
            nomeOtimizador = "Simulated Annealing";
            output << criarCabecalhoCaixa("MÉTODO DE OTIMIZAÇÃO") << "\n";
            output << criarLinhaCaixa(colorir("• Algoritmo: ", BRANCO) + 
                                    colorirBold(nomeOtimizador, CIANO)) << "\n";
            output << criarRodapeCaixa() << "\n\n";
            
            OtimizadorWave otimizador(deposito, backlog, localizador, verificador);
            auto solucao = otimizador.otimizarWave(limiteLB, limiteUB);
            melhorWave.pedidosWave = solucao.pedidosWave;
            melhorWave.corredoresWave = solucao.corredoresWave;
            melhorWave.valorObjetivo = solucao.valorObjetivo;
        }
        
        // Extrair os dados da solução
        std::vector<int> pedidosWave = melhorWave.pedidosWave;
        std::vector<int> corredoresWave = melhorWave.corredoresWave;
        
        // Ordenar os IDs dos pedidos e corredores
        std::sort(pedidosWave.begin(), pedidosWave.end());
        std::sort(corredoresWave.begin(), corredoresWave.end());
        
        // Gerar arquivo de solução
        std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
        std::string arquivoSaida = diretorioSaida + "/" + nomeArquivoSemExtensao + ".sol";
        
        std::ofstream outFile(arquivoSaida);
        if (!outFile.is_open()) {
            std::cerr << "Erro ao abrir o arquivo de saída: " << arquivoSaida << std::endl;
            return;
        }
        
        // Escrever o arquivo de solução
        outFile << pedidosWave.size() << "\n";
        for (int pedidoId : pedidosWave) {
            outFile << pedidoId << "\n";
        }
        
        outFile << corredoresWave.size() << "\n";
        for (int corredorId : corredoresWave) {
            outFile << corredorId << "\n";
        }
        
        outFile.close();
        
        // Calcular e registrar o tempo de processamento
        auto fimInstancia = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> tempoDecorrido = fimInstancia - inicioInstancia;
        
        // Armazenar o tempo de execução desta instância
        {
            std::lock_guard<std::mutex> lock(tempos_mutex);
            temposExecucao.temposPorInstancia[nomeArquivoSemExtensao] = tempoDecorrido.count();
        }
        
        // Formatar resultados
        // Formatar BOV com precisão fixa
        std::stringstream bovStr;
        bovStr << std::fixed << std::setprecision(6) << melhorWave.valorObjetivo;
        
        // Formatar tempo com precisão fixa
        std::stringstream tempoStr;
        tempoStr << std::fixed << std::setprecision(3) << tempoDecorrido.count();
        
        output << criarCabecalhoCaixa("RESULTADOS") << "\n";
        output << criarLinhaCaixa(colorir("✓ Arquivo de saída: ", VERDE) + "\n" + arquivoSaida) << "\n";
        output << criarLinhaCaixa(colorir("✓ BOV: ", VERDE) + 
                                colorirBold(bovStr.str(), AZUL)) << "\n";
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