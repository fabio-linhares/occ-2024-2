#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void reformatSolutionFile(const std::string& input_file, const std::string& output_file = "") {
    // Se não foi especificado um arquivo de saída, use o mesmo do input
    std::string actual_output_file = output_file.empty() ? input_file : output_file;
    
    std::vector<int> pedidos;
    std::string line;
    
    // Ler o arquivo atual
    std::ifstream input(input_file);
    if (!input.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << input_file << std::endl;
        return;
    }
    
    // Ler todos os IDs de pedidos
    while (std::getline(input, line)) {
        // Remover espaços em branco e verificar se a linha não está vazia
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (!line.empty()) {
            try {
                int pedido_id = std::stoi(line);
                pedidos.push_back(pedido_id);
            } catch (const std::exception& e) {
                std::cerr << "Erro ao converter linha para inteiro: " << line << std::endl;
            }
        }
    }
    input.close();
    
    // Escrever no formato correto
    std::ofstream output(actual_output_file);
    if (!output.is_open()) {
        std::cerr << "Erro ao abrir arquivo para escrita: " << actual_output_file << std::endl;
        return;
    }
    
    // Primeira linha: número de pedidos
    output << pedidos.size() << std::endl;
    
    // Segunda linha: IDs dos pedidos separados por espaço
    for (size_t i = 0; i < pedidos.size(); i++) {
        output << pedidos[i];
        if (i < pedidos.size() - 1) {
            output << " ";
        }
    }
    output << std::endl;
    
    output.close();
    
    std::cout << "Arquivo reformatado com sucesso: " << actual_output_file << std::endl;
    std::cout << "Total de pedidos: " << pedidos.size() << std::endl;
}

// Processa todos os arquivos de solução em um diretório
void reformatAllSolutionFiles(const std::string& directory_path) {
    int files_processed = 0;
    
    for (const auto& entry : fs::directory_iterator(directory_path)) {
        std::string filename = entry.path().filename().string();
        
        // Verificar se é um arquivo de solução
        if (entry.is_regular_file() && filename.find("_solution.txt") != std::string::npos) {
            std::cout << "Processando: " << filename << std::endl;
            reformatSolutionFile(entry.path().string());
            files_processed++;
        }
    }
    
    std::cout << "Total de arquivos processados: " << files_processed << std::endl;
}

// Função principal para executar o formatador
int main(int argc, char* argv[]) {
    std::string directory = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/output";
    
    // Se for passado um diretório específico como argumento
    if (argc > 1) {
        directory = argv[1];
    }
    
    std::cout << "Reformatando arquivos de solução no diretório: " << directory << std::endl;
    reformatAllSolutionFiles(directory);
    
    return 0;
}