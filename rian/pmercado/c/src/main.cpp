#include "../include/config_manager.h"
#include "../include/objective_function.h"
#include "../include/constraints.h"
#include "../include/algorithm.h"
#include "../include/validator.h"
#include "../include/data_structures.h"
#include "../include/variable_explorer.h"
#include "file_manager.h"  // Incluir o novo cabeçalho
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

/*


{
  g++ -std=c++17 -c src/config_extractor.cpp -o exec/config_extractor.o && \
  g++ -std=c++17 -c src/config_manager.cpp -o exec/config_manager.o && \
  g++ -std=c++17 -c src/objective_function.cpp -o exec/objective_function.o && \
  g++ -std=c++17 -c src/constraints.cpp -o exec/constraints.o && \
  g++ -std=c++17 -c src/algorithm.cpp -o exec/algorithm.o && \
  g++ -std=c++17 -c src/validator.cpp -o exec/validator.o && \
  g++ -std=c++17 -c src/variable_explorer.cpp -o exec/variable_explorer.o && \
  g++ -std=c++17 -c src/file_manager.cpp -o exec/file_manager.o && \
  g++ -std=c++17 -c src/pre_processor.cpp -o exec/pre_processor.o && \
  g++ -std=c++17 -c src/main.cpp -o exec/main.o && \
  g++ exec/*.o -o exec/optimizer
} 2> exec/erros


*/

// Mostrar ajuda sobre como usar o programa
void mostrarAjuda() {
    std::cout << "Uso: ./exec/optimizer [opções]" << std::endl;
    std::cout << "Opções:" << std::endl;
    std::cout << "  --ajuda, -h             Mostra esta mensagem de ajuda" << std::endl;
    std::cout << "  --ver_variaveis, -v     Mostra o explorador interativo de variáveis" << std::endl;
    std::cout << "  --executar, -e          Executa o algoritmo de otimização" << std::endl;
    std::cout << "  --listar_arquivos, -l   Lista os arquivos no diretório de entrada" << std::endl;
}

// Função para criar uma instância de exemplo
Instance criarInstanciaExemplo() {
    Instance instancia;
    instancia.num_pedidos = 3;
    instancia.num_itens = 4;
    instancia.num_corredores = 2;
    instancia.lb = 5;
    instancia.ub = 15;
    
    // Configurar pedidos
    Pedido pedido1 = {0, {{0, 2}, {1, 3}}, 5};
    Pedido pedido2 = {1, {{1, 1}, {2, 2}}, 3};
    Pedido pedido3 = {2, {{0, 1}, {3, 2}}, 3};
    
    instancia.pedidos = {pedido1, pedido2, pedido3};
    
    // Configurar corredores
    Corredor corredor1 = {0, {{0, 5}, {1, 4}}};
    Corredor corredor2 = {1, {{2, 3}, {3, 4}}};
    
    instancia.corredores = {corredor1, corredor2};
    
    return instancia;
}

// Função para executar o algoritmo
void executarAlgoritmo(const ConfigManager& configManager) {
    // Listar arquivos antes de executar o algoritmo
    FileManager fileManager(configManager);
    fileManager.listarArquivosEntrada();
    
    std::cout << "\nCriando instância de exemplo..." << std::endl;
    Instance instancia = criarInstanciaExemplo();
    
    // Acessar configurações do algoritmo
    std::cout << "\nAcessando configurações do algoritmo..." << std::endl;
    AlgorithmConfig algoConfig = loadAlgorithmConfig();
    std::cout << "Algoritmo: " << algoConfig.name << std::endl;
    std::cout << "Epsilon: " << algoConfig.epsilon << std::endl;
    std::cout << "Iterações máximas: " << algoConfig.max_iterations << std::endl;
    
    // Executar algoritmo
    std::cout << "\nExecutando algoritmo..." << std::endl;
    Solution solucao = dinkelbachAlgorithm(instancia, algoConfig.epsilon, algoConfig.max_iterations);
    
    // Exibir resultado
    std::cout << "\nResultado:" << std::endl;
    std::cout << "Pedidos selecionados: ";
    for (int pedido : solucao.pedidos_selecionados) {
        std::cout << pedido << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Corredores visitados: ";
    for (int corredor : solucao.corredores_visitados) {
        std::cout << corredor << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Valor objetivo: " << std::fixed << std::setprecision(4) << solucao.valor_objetivo << std::endl;
    
    // Validar solução
    bool valida = validarSolucao(solucao, instancia);
    std::cout << "Solução válida: " << (valida ? "Sim" : "Não") << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string configDir = "/home/zerocopia/Projetos/occ-2024-2/rian/pmercado/c/config/";
    
    try {
        // Carregar configurações
        std::cout << "Carregando configurações..." << std::endl;
        ConfigManager& configManager = ConfigManager::getInstance();
        configManager.loadAllConfigs(configDir);
        
        // Verificar parâmetros de linha de comando
        bool modoInterativo = false;
        bool mostrarHelp = false;
        bool executar = false;
        bool listarArquivos = false;  // Nova opção
        
        if (argc > 1) {
            std::string arg1(argv[1]);
            if (arg1 == "--ver_variaveis" || arg1 == "-v") {
                modoInterativo = true;
            } else if (arg1 == "--ajuda" || arg1 == "-h") {
                mostrarHelp = true;
            } else if (arg1 == "--executar" || arg1 == "-e") {
                executar = true;
            } else if (arg1 == "--listar_arquivos" || arg1 == "-l") {  // Nova opção
                listarArquivos = true;
            } else {
                std::cout << "Opção desconhecida: " << arg1 << std::endl;
                mostrarHelp = true;
            }
        } else {
            // Comportamento padrão sem argumentos
            executar = true;
        }
        
        if (mostrarHelp) {
            mostrarAjuda();
            return 0;
        }
        
        // ===== SEÇÃO DE MÓDULOS ADICIONAIS =====
        if (listarArquivos) {
            FileManager fileManager(configManager);
            // Usar o novo método em vez do original
            fileManager.listarArquivosComPreProcessamento();
            return 0;
        }
        // ===== FIM DA SEÇÃO DE MÓDULOS ADICIONAIS =====
        
        if (modoInterativo) {
            VariableExplorer explorer(configManager);
            explorer.executeModoInterativo();
        } else if (executar) {
            executarAlgoritmo(configManager);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
