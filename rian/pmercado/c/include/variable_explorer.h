#ifndef VARIABLE_EXPLORER_H
#define VARIABLE_EXPLORER_H

#include "config_manager.h"
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <iostream>

/*
 { \  
g++ -std=c++17 -c src/config_extractor.cpp -o exec/config_extractor.o && \
g++ -std=c++17 -c src/config_manager.cpp -o exec/config_manager.o && \
g++ -std=c++17 -c src/objective_function.cpp -o exec/objective_function.o && \
g++ -std=c++17 -c src/constraints.cpp -o exec/constraints.o && \
g++ -std=c++17 -c src/algorithm.cpp -o exec/algorithm.o && \
g++ -std=c++17 -c src/validator.cpp -o exec/validator.o && \
g++ -std=c++17 -c src/variable_explorer.cpp -o exec/variable_explorer.o && \
g++ -std=c++17 -c src/main.cpp -o exec/main.o && \
g++ exec/config_extractor.o exec/config_manager.o exec/objective_function.o exec/constraints.o exec/algorithm.o exec/validator.o exec/variable_explorer.o exec/main.o -o exec/optimizer; \
} 2> exec/erros



Funcionamento
Quando você executa o programa com --ver_variaveis:

Todas as variáveis são listadas por categoria e numeradas
Você pode digitar o número de uma variável para ver seu conteúdo completo
Os valores multilinha são exibidos formatados adequadamente
Você pode continuar explorando variáveis ou digitar 'q' para sair
Esta implementação é muito útil para depuração e para entender a estrutura de configuração de todo o sistema!
*/

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