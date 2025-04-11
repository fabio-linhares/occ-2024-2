#ifndef VARIABLE_EXPLORER_H
#define VARIABLE_EXPLORER_H

#include "config_manager.h"
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <iostream>

class VariableExplorer {
public:
    // Inicializa o explorador com o ConfigManager
    VariableExplorer(const ConfigManager& configManager);
    
    // Executa o modo interativo para explorar variáveis
    void executeModoInterativo();
    
    // Lista todas as variáveis por categoria
    void listarTodasVariaveis();
    
    // Exibe uma variável específica
    void exibirVariavel(int numeroVariavel);
    
private:
    const ConfigManager& configManager;
    std::vector<std::pair<std::string, std::string>> todasVariaveis; // Categoria, Chave
    std::map<std::string, std::string> categoriaParaTitulo;
    
    // Carrega todas as variáveis do ConfigManager
    void carregarVariaveis();
    
    // Retorna um título amigável para uma categoria
    std::string tituloAmigavel(const std::string& categoria) const;
    
    // Formata um valor para exibição
    std::string formatarValor(const std::string& valor) const;
};

#endif // VARIABLE_EXPLORER_H