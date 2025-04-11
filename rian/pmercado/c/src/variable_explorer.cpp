#include "../include/variable_explorer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

VariableExplorer::VariableExplorer(const ConfigManager& configManager) 
    : configManager(configManager) {
    categoriaParaTitulo = {
        {"problem", "Definição do Problema"},
        {"objective", "Função Objetivo"},
        {"constraints", "Restrições"},
        {"algorithm", "Configuração do Algoritmo"},
        {"data_structures", "Estruturas de Dados"},
        {"variables", "Definições de Variáveis"},
        {"input_format", "Formato de Entrada"},
        {"output_format", "Formato de Saída"}
    };
    carregarVariaveis();
}

void VariableExplorer::carregarVariaveis() {
    todasVariaveis.clear();
    
    // Obter todas as categorias
    std::vector<std::string> categorias = configManager.getCategories();
    
    // Para cada categoria, obter todas as chaves
    for (const auto& categoria : categorias) {
        std::vector<std::string> chaves = configManager.getKeys(categoria);
        for (const auto& chave : chaves) {
            todasVariaveis.push_back({categoria, chave});
        }
    }
}

std::string VariableExplorer::tituloAmigavel(const std::string& categoria) const {
    if (categoriaParaTitulo.find(categoria) != categoriaParaTitulo.end()) {
        return categoriaParaTitulo.at(categoria);
    }
    return categoria;
}

std::string VariableExplorer::formatarValor(const std::string& valor) const {
    if (valor.find('\n') != std::string::npos) {
        std::istringstream stream(valor);
        std::string linha;
        std::string resultado = "[VALOR MULTILINHA]\n";
        while (std::getline(stream, linha)) {
            resultado += "    " + linha + "\n";
        }
        return resultado;
    }
    return valor;
}

void VariableExplorer::listarTodasVariaveis() {
    std::cout << "\nLISTA DE VARIÁVEIS CARREGADAS:" << std::endl;
    std::cout << "=============================" << std::endl;
    
    std::string categoriaAtual = "";
    
    for (size_t i = 0; i < todasVariaveis.size(); ++i) {
        const auto& [categoria, chave] = todasVariaveis[i];
        
        // Mostrar cabeçalho da categoria quando esta mudar
        if (categoria != categoriaAtual) {
            categoriaAtual = categoria;
            std::cout << "\n[" << tituloAmigavel(categoria) << "]" << std::endl;
        }
        
        // Mostrar número, chave e valor resumido
        std::cout << std::setw(3) << (i + 1) << ". " << chave;
        
        // Mostrar tipo de valor (multilinha, etc)
        std::string valor = configManager.getString(categoria, chave);
        if (valor.find('\n') != std::string::npos) {
            std::cout << " = [VALOR MULTILINHA]" << std::endl;
        } else if (valor.length() > 50) {
            std::cout << " = " << valor.substr(0, 47) << "..." << std::endl;
        } else {
            std::cout << " = " << valor << std::endl;
        }
    }
}

void VariableExplorer::exibirVariavel(int numeroVariavel) {
    if (numeroVariavel < 1 || numeroVariavel > static_cast<int>(todasVariaveis.size())) {
        std::cout << "Número de variável inválido. Escolha entre 1 e " 
                  << todasVariaveis.size() << std::endl;
        return;
    }
    
    int indice = numeroVariavel - 1;
    const auto& [categoria, chave] = todasVariaveis[indice];
    std::string valor = configManager.getString(categoria, chave);
    
    std::cout << "\nDETALHES DA VARIÁVEL #" << numeroVariavel << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Categoria: " << tituloAmigavel(categoria) << std::endl;
    std::cout << "Chave: " << chave << std::endl;
    std::cout << "Valor:" << std::endl;
    std::cout << formatarValor(valor) << std::endl;
}

void VariableExplorer::executeModoInterativo() {
    std::string input;
    int numeroVariavel = 0;
    
    while (true) {
        listarTodasVariaveis();
        
        std::cout << "\nDigite o número da variável para ver seu conteúdo completo (ou 'q' para sair): ";
        std::getline(std::cin, input);
        
        // Verificar se o usuário quer sair
        if (input == "q" || input == "Q" || input == "sair" || input == "exit") {
            break;
        }
        
        // Tentar converter a entrada em um número
        try {
            numeroVariavel = std::stoi(input);
            exibirVariavel(numeroVariavel);
            
            std::cout << "\nPressione ENTER para continuar...";
            std::getline(std::cin, input);
            
        } catch (const std::exception& e) {
            std::cout << "Entrada inválida. Por favor, digite um número ou 'q' para sair." << std::endl;
            std::cout << "\nPressione ENTER para continuar...";
            std::getline(std::cin, input);
        }
    }
}