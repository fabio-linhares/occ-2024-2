#include "restricoes.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

RestricoesConfig carregarRestricoesConfig(const std::string& caminho) {
    RestricoesConfig config;
    std::ifstream arquivo(caminho);
    
    if (!arquivo.is_open()) {
        std::cerr << "Aviso: Arquivo de restrições '" << caminho 
                  << "' não encontrado. Usando valores padrão." << std::endl;
        return config; // Retorna configurações padrão
    }
    
    std::string linha;
    while (std::getline(arquivo, linha)) {
        // Ignorar comentários e linhas vazias
        if (linha.empty() || linha[0] == '#' || linha[0] == '/') {
            continue;
        }
        
        // Verificar se é uma linha de configuração (tem um sinal de igual)
        size_t pos_igual = linha.find('=');
        if (pos_igual != std::string::npos) {
            std::string chave = linha.substr(0, pos_igual);
            std::string valor = linha.substr(pos_igual + 1);
            
            // Remover espaços em branco
            chave.erase(0, chave.find_first_not_of(" \t"));
            chave.erase(chave.find_last_not_of(" \t") + 1);
            valor.erase(0, valor.find_first_not_of(" \t"));
            valor.erase(valor.find_last_not_of(" \t") + 1);
            
            // Definir a configuração apropriada
            if (chave == "LIMITE_TEMPO_TOTAL_MS") {
                config.limite_tempo_total_ms = std::stoi(valor);
            }
            // Adicione outras chaves conforme necessário...
        }
    }
    
    std::cout << "Configurações carregadas com sucesso do arquivo '" << caminho << "'" << std::endl;
    return config;
}