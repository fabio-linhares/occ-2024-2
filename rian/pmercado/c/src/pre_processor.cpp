#include "pre_processor.h"
#include "file_manager.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <future>

namespace fs = std::filesystem;

PreProcessor::PreProcessor() 
    : continuar_processamento(true), max_arquivos_processados(-1) {}

// Processa a última linha do arquivo assincronamente
std::pair<int, int> PreProcessor::processarUltimaLinhaAsync(const std::string& arquivo_caminho) {
    std::ifstream arquivo(arquivo_caminho);
    if (!arquivo) {
        std::cerr << "Erro ao abrir o arquivo: " << arquivo_caminho << std::endl;
        return {0, 0};
    }
    
    // Navegar até a última linha
    std::string ultima_linha;
    std::string linha;
    while (std::getline(arquivo, linha)) {
        ultima_linha = linha;
    }
    
    // Processar a última linha para obter LB e UB
    std::istringstream iss(ultima_linha);
    int lb = 0, ub = 0;
    iss >> lb >> ub;
    
    return {lb, ub};
}

// Processa o arquivo de instância e extrai informações
void PreProcessor::processarArquivoInstancia(const std::string& arquivo_caminho) {
    std::ifstream arquivo(arquivo_caminho);
    if (!arquivo) {
        std::cerr << "Erro ao abrir o arquivo: " << arquivo_caminho << std::endl;
        return;
    }
    
    // Ler a primeira linha para obter número de pedidos, itens e corredores
    std::string primeira_linha;
    if (std::getline(arquivo, primeira_linha)) {
        std::istringstream iss(primeira_linha);
        iss >> g_num_pedidos >> g_num_itens >> g_num_corredores;
    }
}

bool PreProcessor::preProcessarArquivo(const std::string& arquivo_caminho, int indice, int total_arquivos, int threads_disponiveis) {
    std::cout << "\n--- PRÉ-PROCESSAMENTO ---" << std::endl;
    std::cout << "Arquivo: " << fs::path(arquivo_caminho).filename().string() << std::endl;
    std::cout << "Caminho completo: " << arquivo_caminho << std::endl;
    std::cout << "Índice: " << indice + 1 << " de " << total_arquivos << std::endl;
    std::cout << "Threads disponíveis: " << threads_disponiveis << std::endl;
    
    // Inicializa a variável de decisão
    bool processar = true;
    
    // Exemplo: verificar o tamanho do arquivo
    std::error_code ec;
    uintmax_t tamanho = fs::file_size(arquivo_caminho, ec);
    
    if (ec) {
        std::cerr << "Erro ao obter tamanho do arquivo: " << ec.message() << std::endl;
        return false; // Não processar este arquivo
    }
    
    std::cout << "Tamanho: " << tamanho << " bytes" << std::endl;
    
    // Exemplo: verificar extensão do arquivo
    std::string extensao = fs::path(arquivo_caminho).extension().string();
    std::cout << "Extensão: " << (extensao.empty() ? "(sem extensão)" : extensao) << std::endl;
    
    // Exemplo: arquivos muito grandes são ignorados (acima de 10MB)
    if (tamanho > 10 * 1024 * 1024) {
        std::cout << "Arquivo muito grande, ignorando." << std::endl;
        processar = false;
    }
    
    // Limpar dados de instância anterior
    FileManager::limparDadosInstancia();
    
    // Iniciar cronômetro
    auto inicio = std::chrono::high_resolution_clock::now();
    
    // Simulação de algum processamento que leva tempo
    std::cout << "Realizando análise prévia do arquivo..." << std::endl;
    
    // Lançar uma thread assíncrona para processar a última linha
    std::future<std::pair<int, int>> future_limites = 
        std::async(std::launch::async, &PreProcessor::processarUltimaLinhaAsync, arquivo_caminho);
    
    // Enquanto a thread está processando a última linha, processar a primeira linha
    processarArquivoInstancia(arquivo_caminho);
    
    // Obter os resultados da thread que processou a última linha
    auto limites = future_limites.get();
    g_limite_inferior = limites.first;
    g_limite_superior = limites.second;
    
    // Exibir as informações extraídas
    std::cout << "Informações da instância:" << std::endl;
    std::cout << "- Número de pedidos: " << g_num_pedidos << std::endl;
    std::cout << "- Número de itens: " << g_num_itens << std::endl;
    std::cout << "- Número de corredores: " << g_num_corredores << std::endl;
    std::cout << "- Limite inferior (LB): " << g_limite_inferior << std::endl;
    std::cout << "- Limite superior (UB): " << g_limite_superior << std::endl;
    
    // Parar cronômetro
    auto fim = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(fim - inicio);
    
    std::cout << "Resultado: " << (processar ? "Arquivo será processado" : "Arquivo será ignorado") << std::endl;
    std::cout << "Tempo de análise prévia: " << duracao.count() << " ms" << std::endl;
    std::cout << "-------------------------" << std::endl;
    
    return processar;
}

bool PreProcessor::deveContinuar() const {
    return continuar_processamento;
}