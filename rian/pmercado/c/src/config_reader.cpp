#include "config_reader.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

ConfigReader::ConfigReader(const std::string& path) : config_path(path) {
    lerConfiguracoes();
}

bool ConfigReader::lerConfiguracoes() {
    std::ifstream arquivo(config_path);
    
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de configuração: " << config_path << std::endl;
        return false;
    }
    
    std::string linha;
    while (std::getline(arquivo, linha)) {
        // Ignorar comentários e linhas vazias
        if (linha.empty() || linha.substr(0, 2) == "//") continue;
        
        // Procurar por "=" para separar chave e valor
        size_t pos = linha.find("=");
        if (pos != std::string::npos) {
            std::string chave = linha.substr(0, pos);
            std::string valor = linha.substr(pos + 1);
            
            // Remover espaços em branco
            chave.erase(0, chave.find_first_not_of(" \t"));
            chave.erase(chave.find_last_not_of(" \t") + 1);
            valor.erase(0, valor.find_first_not_of(" \t"));
            valor.erase(valor.find_last_not_of(" \t") + 1);
            
            configs[chave] = valor;
        }
    }
    
    arquivo.close();
    return true;
}

std::string ConfigReader::getValor(const std::string& chave) const {
    auto it = configs.find(chave);
    if (it != configs.end()) {
        return it->second;
    }
    return "";
}

std::vector<std::string> ConfigReader::listarArquivosEntrada() const {
    std::vector<std::string> arquivos;
    std::string input_dir = getValor("INPUT_DIRECTORY");
    
    if (input_dir.empty()) {
        std::cerr << "Diretório de entrada não encontrado na configuração" << std::endl;
        return arquivos;
    }
    
    try {
        // Verificar se o diretório existe
        if (!fs::exists(input_dir)) {
            std::cerr << "O diretório de entrada não existe: " << input_dir << std::endl;
            return arquivos;
        }
        
        // Listar arquivos
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

void ConfigReader::mostrarArquivosEntrada() const {
    std::string input_dir = getValor("INPUT_DIRECTORY");
    std::vector<std::string> arquivos = listarArquivosEntrada();
    
    std::cout << "Listando arquivos do diretório: " << input_dir << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& arquivo : arquivos) {
        std::cout << arquivo << std::endl;
    }
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Total de arquivos: " << arquivos.size() << std::endl;
}
