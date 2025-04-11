#include "../include/config_extractor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

std::map<std::string, std::string> parseConfigFile(const std::string& filepath) {
    std::ifstream file(filepath);
    std::map<std::string, std::string> config;
    
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filepath << std::endl;
        return config;
    }
    
    std::string line;
    std::string currentKey;
    std::string currentValue;
    bool inMultilineValue = false;
    
    while (std::getline(file, line)) {
        // Ignorar comentários e linhas vazias
        if (line.empty() || line.find("//") == 0) {
            continue;
        }
        
        // Se estamos processando um valor de múltiplas linhas
        if (inMultilineValue) {
            // Verificar se esta linha inicia uma nova chave (fim do valor atual)
            if (!line.empty() && line[0] != ' ' && line[0] != '\t' && line.find('=') != std::string::npos) {
                // Salvar o valor atual e começar uma nova chave
                config[currentKey] = currentValue;
                inMultilineValue = false;
                // Continue para processar a linha atual como nova chave
            } else {
                // Adicionar esta linha ao valor atual
                currentValue += line + "\n";
                continue;
            }
        }
        
        // Procurar um sinal de igual na linha
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            // Extrair a chave
            currentKey = line.substr(0, equalsPos);
            // Remover espaços em branco
            currentKey.erase(0, currentKey.find_first_not_of(" \t"));
            currentKey.erase(currentKey.find_last_not_of(" \t") + 1);
            
            // Extrair o valor
            currentValue = line.substr(equalsPos + 1);
            // Remover espaços em branco no início
            if (currentValue.find_first_not_of(" \t") != std::string::npos) {
                currentValue.erase(0, currentValue.find_first_not_of(" \t"));
            }
            
            // Se o valor não estiver vazio, remover espaços em branco no final
            if (!currentValue.empty() && currentValue.find_last_not_of(" \t") != std::string::npos) {
                currentValue.erase(currentValue.find_last_not_of(" \t") + 1);
            }
            
            // Verificar se este é o início de um valor de múltiplas linhas
            if (currentValue.empty()) {
                inMultilineValue = true;
                currentValue = "";
            } else {
                // Se não for um valor de múltiplas linhas, salvar imediatamente
                config[currentKey] = currentValue;
            }
        }
    }
    
    // Salvar o último valor, se houver
    if (inMultilineValue) {
        config[currentKey] = currentValue;
    }
    
    return config;
}

void displayConfig(const std::string& title, const std::map<std::string, std::string>& config) {
    std::cout << "\n=== " << title << " ===" << std::endl;
    for (const auto& [key, value] : config) {
        std::cout << key << " = ";
        
        // Verificar se o valor contém múltiplas linhas
        if (value.find('\n') != std::string::npos) {
            std::cout << "[VALOR MULTILINHA]" << std::endl;
            std::istringstream stream(value);
            std::string valueLine;
            while (std::getline(stream, valueLine)) {
                std::cout << "    " << valueLine << std::endl;
            }
        } else {
            std::cout << value << std::endl;
        }
    }
}