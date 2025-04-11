#include "file_manager.h"
#include "../include/config_manager.h"
#include "pre_processor.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstdlib> // Para std::system
#include <string>
#include <fstream>
#include <thread>
#include <future> // Para std::async
#include <chrono> // Para medição de tempo

namespace fs = std::filesystem;

// Definição de variáveis globais
int g_num_pedidos = 0;
int g_num_itens = 0;
int g_num_corredores = 0;
int g_limite_inferior = 0;
int g_limite_superior = 0;

FileManager::FileManager(const ConfigManager& configManager) 
    : configManager(configManager) {}

// Função para consultar o número de threads disponíveis no sistema
int FileManager::consultarThreadsDisponiveis() const {
    std::cout << "Consultando número de threads disponíveis..." << std::endl;
    
    // Obter o número máximo de threads que este hardware suporta
    int max_threads = std::thread::hardware_concurrency();
    
    // Verificar carga do sistema para determinar quantas threads estão livres
    // Este é um exemplo simples; em um sistema real, você pode querer
    // implementar uma lógica mais sofisticada para determinar threads disponíveis
    
    // Abrir o arquivo /proc/loadavg para obter a carga do sistema
    std::ifstream loadavg("/proc/loadavg");
    float load1, load5, load15;
    if (loadavg >> load1 >> load5 >> load15) {
        // Carga do sistema nos últimos 1, 5 e 15 minutos
        std::cout << "Carga do sistema: " << load1 << " (1 min), " 
                  << load5 << " (5 min), " << load15 << " (15 min)" << std::endl;
        
        // Calcular threads disponíveis (este é um cálculo simplificado)
        int threads_em_uso = static_cast<int>(load1 + 0.5f); // Arredonda para o inteiro mais próximo
        int threads_disponiveis = std::max(1, max_threads - threads_em_uso);
        
        std::cout << "Threads totais: " << max_threads << std::endl;
        std::cout << "Threads em uso (aproximado): " << threads_em_uso << std::endl;
        std::cout << "Threads disponíveis: " << threads_disponiveis << std::endl;
        
        return threads_disponiveis;
    }
    
    // Se não conseguir obter a carga, retornar metade das threads disponíveis
    int default_threads = std::max(1, max_threads / 2);
    std::cout << "Não foi possível determinar a carga do sistema." << std::endl;
    std::cout << "Usando valor padrão: " << default_threads << " threads disponíveis." << std::endl;
    return default_threads;
}

// Função para ordenar arquivos de entrada usando uma thread dedicada
std::vector<std::string> FileManager::ordenarArquivos(const std::string& input_dir) const {
    std::cout << "Iniciando ordenação de arquivos em uma thread dedicada...\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::string> arquivos;
    
    try {
        if (!fs::exists(input_dir)) {
            std::cerr << "O diretório não existe: " << input_dir << std::endl;
            return arquivos;
        }
        
        // Coletar nomes dos arquivos
        for (const auto& entrada : fs::directory_iterator(input_dir)) {
            if (entrada.is_regular_file()) {
                arquivos.push_back(entrada.path().string());
            }
        }
        
        // Ordenar arquivos por nome
        std::sort(arquivos.begin(), arquivos.end(), [](const std::string& a, const std::string& b) {
            std::string nome_a = fs::path(a).filename().string();
            std::string nome_b = fs::path(b).filename().string();
            return nome_a < nome_b;
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Ordenação de " << arquivos.size() << " arquivos concluída em " 
                  << duration.count() << " ms\n";
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Erro ao acessar arquivos: " << e.what() << std::endl;
    }
    
    return arquivos;
}

// Função para processar um arquivo com base nas threads disponíveis
bool FileManager::processarArquivo(const std::string& arquivo_caminho, int threads_disponiveis) const {
    // Removi as mensagens detalhadas
    
    // Exemplo simples: simular processamento
    std::string comando = "echo \"Processando " + arquivo_caminho + " com " + 
                         std::to_string(threads_disponiveis) + " threads\" > /dev/null";
    
    int resultado = std::system(comando.c_str());
    
    if (resultado != 0) {
        std::cerr << "Erro ao processar arquivo: " << arquivo_caminho << std::endl;
        return false;
    }
    
    // Exibir apenas a mensagem simplificada
    std::cout << "\n--- ARQUIVO-PROCESSADO ---\n" << std::endl;
    return true;
}

void FileManager::listarArquivosEntrada() const {
    // Usar o caminho absoluto em vez do relativo
    std::string input_dir = "/home/zerocopia/Projetos/occ-2024-2/rian/pmercado/input";
    
    std::cout << "\nListando arquivos do diretório: " << input_dir << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    try {
        // Verificar se o diretório existe
        if (!fs::exists(input_dir)) {
            std::cerr << "O diretório de entrada não existe: " << input_dir << std::endl;
            return;
        }
        
        // Usar uma thread dedicada para ordenar os arquivos
        // Executamos de forma assíncrona para não bloquear a thread principal
        std::future<std::vector<std::string>> future_arquivos = 
            std::async(std::launch::async, &FileManager::ordenarArquivos, this, input_dir);
        
        // Enquanto espera a ordenação, podemos fazer outras tarefas se necessário
        std::cout << "Aguardando ordenação dos arquivos...\n";
        
        // Obter a lista ordenada de arquivos
        std::vector<std::string> arquivos_ordenados = future_arquivos.get();
        
        if (arquivos_ordenados.empty()) {
            std::cout << "Nenhum arquivo encontrado no diretório.\n";
            return;
        }
        
        std::cout << "Arquivos ordenados por nome (do menor para o maior):\n";
        for (const auto& arquivo : arquivos_ordenados) {
            std::cout << "- " << fs::path(arquivo).filename().string() << std::endl;
        }
        
        std::cout << "\nIniciando processamento dos arquivos em ordem...\n";
        std::cout << "----------------------------------------\n";
        
        // Processar arquivos em ordem
        int processados = 0;
        for (const auto& arquivo : arquivos_ordenados) {
            // Exibir nome do arquivo antes de processá-lo
            std::cout << "\nArquivo: " << fs::path(arquivo).filename().string() << std::endl;
            
            // Consultar threads disponíveis antes de processar o arquivo
            int threads_disponiveis = consultarThreadsDisponiveis();
            
            // Processar o arquivo com o número de threads disponíveis
            if (processarArquivo(arquivo, threads_disponiveis)) {
                processados++;
            }
        }
        
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Total de arquivos: " << arquivos_ordenados.size() << std::endl;
        std::cout << "Arquivos processados: " << processados << std::endl;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Erro ao acessar o sistema de arquivos: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Erro durante o processamento: " << e.what() << std::endl;
    }
}

std::vector<std::string> FileManager::obterArquivosEntrada() const {
    std::vector<std::string> arquivos;
    
    // Usar o caminho absoluto em vez do relativo
    std::string input_dir = "/home/zerocopia/Projetos/occ-2024-2/rian/pmercado/input";
    
    if (!fs::exists(input_dir)) {
        return arquivos;
    }
    
    try {
        for (const auto& entrada : fs::directory_iterator(input_dir)) {
            if (entrada.is_regular_file()) {
                arquivos.push_back(entrada.path().filename().string());
            }
        }
        
        // Ordenar arquivos por nome
        std::sort(arquivos.begin(), arquivos.end());
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Erro ao acessar o sistema de arquivos: " << e.what() << std::endl;
    }
    
    return arquivos;
}

int FileManager::contarArquivosEntrada() const {
    return obterArquivosEntrada().size();
}

void FileManager::listarArquivosComPreProcessamento() const {
    // Usar o caminho absoluto em vez do relativo
    std::string input_dir = "/home/zerocopia/Projetos/occ-2024-2/rian/pmercado/input";
    
    std::cout << "\nListando arquivos do diretório: " << input_dir << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    try {
        // Verificar se o diretório existe
        if (!fs::exists(input_dir)) {
            std::cerr << "O diretório de entrada não existe: " << input_dir << std::endl;
            return;
        }
        
        // Usar uma thread dedicada para ordenar os arquivos
        // Executamos de forma assíncrona para não bloquear a thread principal
        std::future<std::vector<std::string>> future_arquivos = 
            std::async(std::launch::async, &FileManager::ordenarArquivos, this, input_dir);
        
        // Enquanto espera a ordenação, podemos fazer outras tarefas se necessário
        std::cout << "Aguardando ordenação dos arquivos...\n";
        
        // Obter a lista ordenada de arquivos
        std::vector<std::string> arquivos_ordenados = future_arquivos.get();
        
        if (arquivos_ordenados.empty()) {
            std::cout << "Nenhum arquivo encontrado no diretório.\n";
            return;
        }
        
        // Removida a exibição da lista de arquivos ordenados
        
        std::cout << "\nIniciando processamento dos arquivos em ordem...\n";
        std::cout << "----------------------------------------\n";
        
        // Criar o pré-processador
        PreProcessor preProcessor;
        
        // Processar arquivos em ordem
        int processados = 0;
        for (size_t i = 0; i < arquivos_ordenados.size(); ++i) {
            const auto& arquivo = arquivos_ordenados[i];
            
            // Consultar threads disponíveis antes do pré-processamento
            int threads_disponiveis = consultarThreadsDisponiveis();
            
            // Executar pré-processamento com informação de threads
            bool deve_processar = preProcessor.preProcessarArquivo(
                arquivo, i, arquivos_ordenados.size(), threads_disponiveis);
            
            // Verificar se deve continuar o loop
            if (!preProcessor.deveContinuar()) {
                std::cout << "Interrompendo o processamento por solicitação do pré-processador.\n";
                break;
            }
            
            // Se o pré-processador indicar que não deve processar, pular para o próximo
            if (!deve_processar) {
                std::cout << "Pulando arquivo: " << fs::path(arquivo).filename().string() << std::endl;
                continue;
            }
            
            // Processar o arquivo usando as mesmas threads já consultadas
            if (processarArquivo(arquivo, threads_disponiveis)) {
                processados++;
            }
        }
        
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Total de arquivos: " << arquivos_ordenados.size() << std::endl;
        std::cout << "Arquivos processados: " << processados << std::endl;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Erro ao acessar o sistema de arquivos: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Erro durante o processamento: " << e.what() << std::endl;
    }
}

// Implementação do método para limpar os dados da instância
void FileManager::limparDadosInstancia() {
    g_num_pedidos = 0;
    g_num_itens = 0;
    g_num_corredores = 0;
    g_limite_inferior = 0;
    g_limite_superior = 0;
}