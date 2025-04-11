#include "problema.h"
#include "solucao.h"
#include "algoritmos.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <omp.h>
#include <filesystem>
#include "controle.h"
#include "timercontrol.h"
#include "restricoes.h"

/*
As correções implementadas abordam os dois problemas principais:

Disponibilidade de itens: A nova implementação de verificarCorrigirDisponibilidade garante que todos os itens dos pedidos selecionados estarão disponíveis nos corredores escolhidos, mesmo que isso signifique adicionar mais corredores.

Controle de timeout: Adicionei vários níveis de timeout:

Timeout global para a execução completa (12 horas)
Timeout adaptativo por instância (baseado no tamanho do problema)
Timeout para algoritmos individuais (busca local, ILS, etc.)
Estas correções devem garantir que o algoritmo:

Sempre gere soluções válidas que respeitam todas as restrições
Nunca fique preso em execuções excessivamente longas
Funcione de forma confiável tanto em modo sequencial quanto paralelo
O timeout adaptativo baseado no tamanho da instância é uma boa abordagem, pois permite que instâncias maiores tenham mais tempo de processamento, enquanto evita que o algoritmo fique preso em casos problemáticos.
*/

using namespace std;
using namespace chrono;
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Verificação dos argumentos de linha de comando
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " [--sequential | --parallel <num_threads>]" << endl;
        cerr << "Exemplo: " << argv[0] << " --parallel 4" << endl;
        cerr << "Exemplo: " << argv[0] << " --sequential" << endl;
        return 1;
    }

    bool modo_paralelo = false;
    int num_threads = 1;

    // Interpretação dos argumentos
    if (string(argv[1]) == "--parallel") {
        if (argc < 3) {
            cerr << "Número de threads não especificado para o modo paralelo." << endl;
            return 1;
        }
        modo_paralelo = true;
        num_threads = stoi(argv[2]);
        cout << "Executando em modo paralelo com " << num_threads << " threads." << endl;
    } else if (string(argv[1]) == "--sequential") {
        cout << "Executando em modo sequencial." << endl;
    }
    else {
        cerr << "Modo inválido. Use --sequential ou --parallel." << endl;
        return 1;
    }

    // Leitura dos arquivos de entrada
    vector<fs::path> arquivos_entrada;
    string diretorio_instancias = "../dataset";
    if (!fs::exists(diretorio_instancias) || !fs::is_directory(diretorio_instancias)) {
        cerr << "Diretório '" << diretorio_instancias << "' não encontrado ou não é um diretório." << endl;
        return 1;
    }
    for (const auto& entry : fs::directory_iterator(diretorio_instancias)) {
        if (entry.path().extension() == ".txt") {
            arquivos_entrada.push_back(entry.path());
        }
    }

    if (arquivos_entrada.empty()) {
        cerr << "Nenhum arquivo .txt encontrado no diretório 'instancias'." << endl;
        return 1;
    }

    // Configuração do OpenMP
    if (modo_paralelo) {
        omp_set_num_threads(num_threads);
        cout << "Número máximo de threads disponíveis: " << omp_get_max_threads() << endl;
    }

    // Calibração do algoritmo
    Parametros parametros;
    if (!calibrarAlgoritmo(parametros)) {
        cerr << "Falha na calibração do algoritmo." << endl;
        return 1;
    }

    vector<ResultadoInstancia> resultados;

    // Carregar configurações de restrições
    RestricoesConfig config = carregarRestricoesConfig();

    // Medição do tempo total de execução
    const auto inicio_total = high_resolution_clock::now();
    const auto tempo_maximo_total = std::chrono::milliseconds(config.limite_tempo_total_ms);

    // Inicializar o timer global com o limite de tempo da configuração
    TimerControl::inicializar(config.limite_tempo_total_ms);

    // Loop principal para processar os arquivos de entrada
    #pragma omp parallel for if(modo_paralelo) schedule(dynamic)
    for (size_t i = 0; i < arquivos_entrada.size(); ++i) {
        // Verificar se ainda há tempo suficiente usando a margem de segurança configurável
        if (TimerControl::tempoExcedido(config.margem_seguranca_ms)) {
            #pragma omp critical
            {
                std::cout << "Tempo limite próximo, pulando instâncias restantes." << std::endl;
            }
            continue;
        }

        // Calcular timeout adaptativo para esta instância usando o limite configurável
        int timeout_instancia_ms = std::min(
            config.limite_tempo_instancia_ms, // Máximo configurável por instância
            TimerControl::tempoDisponivel(0.4) // Máximo de 40% do tempo restante
        );

        // Definir timeout para cada instância
        auto inicio_instancia = std::chrono::high_resolution_clock::now();
        const auto timeout_instancia = std::chrono::milliseconds(timeout_instancia_ms);
        
        const auto& arquivo = arquivos_entrada[i];
        
        try {
            // Leitura do arquivo
            Problema problema = parseEntrada(arquivo.string());
            
            // Aplicar configurações ao problema (ajustar LB e UB conforme configurado)
            config.aplicarAoProblema(problema);
            
            // Verificação de timeout
            if (std::chrono::high_resolution_clock::now() - inicio_instancia > timeout_instancia) {
                throw std::runtime_error("Timeout na leitura do arquivo");
            }
            
            // Resolver o problema usando as configurações carregadas
            Solucao solucao = resolverProblemaAdaptativo(problema, config);
            
            // Verificar se a solução é válida usando as configurações carregadas
            bool valida = validarSolucao(problema, solucao, config);
            
            #pragma omp critical
            {
                std::string status = valida ? "VÁLIDA" : "INVÁLIDA";
                std::cout << "Instância " << arquivo.filename().string() 
                          << " processada. Status: " << status 
                          << ", Razão: " << solucao.custo_total << std::endl;
            }
            
            // Gerar arquivo de saída
            string nome_saida = "output/" + arquivo.stem().string() + "_out.txt";
            gerarSaida(nome_saida, solucao);
            
        } catch (const std::exception& e) {
            #pragma omp critical
            {
                std::cerr << "Erro ao processar " << arquivo.filename().string() 
                          << ": " << e.what() << std::endl;
            }
        }
    }

    // Medição do tempo total de execução
    auto fim_total = high_resolution_clock::now();
    auto duracao_total_final = duration_cast<milliseconds>(fim_total - inicio_total);

    // Geração do relatório detalhado
    gerarRelatorioDetalhado(resultados, duracao_total_final.count());
    
    // Armazenar problemas para análises
    std::vector<Problema> problemas_processados;
    for (const auto& arquivo : arquivos_entrada) {
        try {
            problemas_processados.push_back(parseEntrada(arquivo.string()));
        } catch (const std::exception& e) {
            cerr << "Erro ao processar " << arquivo.filename().string() << ": " << e.what() << endl;
        }
    }
    
    // Gerar estatísticas e relatórios de desempenho
    exibirEstatisticasTerminal(resultados, problemas_processados);
    gerarRelatorioCompleto(resultados, problemas_processados);
    
    // Salvar histórico de desempenho
    registrarDesempenho(resultados, problemas_processados);

    return 0;
}