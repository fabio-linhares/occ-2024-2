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

    // Medição do tempo total de execução
    auto inicio_total = high_resolution_clock::now();

    // Loop principal para processar os arquivos de entrada
    #pragma omp parallel for if(modo_paralelo) schedule(dynamic)
    for (size_t i = 0; i < arquivos_entrada.size(); ++i) {
        const auto& arquivo = arquivos_entrada[i];
        
        try {
            // Leitura do problema a partir do arquivo
            Problema problema = parseEntrada(arquivo.string());
            
            // Medição do tempo de execução da resolução do problema
            auto inicio = high_resolution_clock::now();
            Solucao solucao = resolverProblemaAdaptativo(problema);
            auto fim = high_resolution_clock::now();
            
            auto duracao = duration_cast<milliseconds>(fim - inicio);
            
            // Proteção da seção crítica para evitar race conditions
            #pragma omp critical
            {
                resultados.push_back({arquivo.filename().string(), solucao, duracao.count()});
                cout << "Instância " << arquivo.filename().string() << " processada em " << duracao.count() << "ms" << endl;
            }
            
            // Geração do arquivo de saída
            string nome_saida = "output/" + arquivo.stem().string() + "_out.txt";
            gerarSaida(nome_saida, solucao);
            
        } catch (const exception& e) {
            // Tratamento de erros
            #pragma omp critical
            {
                cerr << "Erro ao processar " << arquivo.filename().string() << ": " << e.what() << endl;
            }
        }
    }

    // Medição do tempo total de execução
    auto fim_total = high_resolution_clock::now();
    auto duracao_total = duration_cast<milliseconds>(fim_total - inicio_total);

    // Geração do relatório detalhado
    gerarRelatorioDetalhado(resultados, duracao_total.count());
    
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