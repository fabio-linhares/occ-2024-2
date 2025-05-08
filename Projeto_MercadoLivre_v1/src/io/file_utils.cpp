#include "io/file_utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
namespace io {

std::vector<std::string> listarArquivos(const std::string& path) {
    std::vector<std::string> arquivos;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                arquivos.push_back(entry.path().filename().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Erro ao acessar diretório: " << e.what() << std::endl;
    }
    return arquivos;
}

void writeOutputFile(const std::string& outputPath, const std::string& filename, const std::string& content) {
    std::string fullOutputPath = outputPath + "/" + filename;
    std::ofstream outputFile(fullOutputPath);

    if (!outputFile.is_open()) {
        std::cerr << "Erro ao criar o arquivo de saída: " << fullOutputPath << std::endl;
        return;
    }

    outputFile << content;
    outputFile.close();

    if (content.empty()) {
        std::cout << "\nArquivo de saída vazio gerado: " << fullOutputPath << std::endl;
    } else {
        std::cout << "\nArquivo de saída gerado: " << fullOutputPath << std::endl;
    }
}

bool initializePaths(std::string& inputPath, std::string& outputPath) {
    // Caminhos padrão absolutos
    std::string defaultInputPath = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/input";
    std::string defaultOutputPath = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/output";
    
    // Solicitar confirmação dos paths
    std::cout << "Path de entrada padrão: " << defaultInputPath << std::endl;
    std::cout << "Deseja confirmar este path? (S/N): ";
    char resposta;
    std::cin >> resposta;
    if (resposta == 'S' || resposta == 's') {
        inputPath = defaultInputPath;
    } else {
        std::cout << "Informe o novo path de entrada: ";
        std::cin >> inputPath;
    }
    
    std::cout << "Path de saída padrão: " << defaultOutputPath << std::endl;
    std::cout << "Deseja confirmar este path? (S/N): ";
    std::cin >> resposta;
    if (resposta == 'S' || resposta == 's') {
        outputPath = defaultOutputPath;
    } else {
        std::cout << "Informe o novo path de saída: ";
        std::cin >> outputPath;
    }
    
    // Criar diretórios se não existirem
    try {
        if (!fs::exists(inputPath)) fs::create_directories(inputPath);
        if (!fs::exists(outputPath)) fs::create_directories(outputPath);
    } catch (const fs::filesystem_error& e) {
        std::cout << "Erro ao criar diretórios: " << e.what() << std::endl;
        return false;
    }
    
    return true;
}

} // namespace io