#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem> // Necessário para iterar sobre diretórios
#include "spo_solver.h"
#include "config_reader.h"

/*
 * Este programa implementa uma solução para o Problema de Seleção de Pedidos Ótima (SPO).
 * O objetivo é maximizar a razão entre itens coletados e corredores visitados.
 * As configurações são lidas de um arquivo e o programa processa todas as instâncias
 * presentes no diretório de entrada, gerando soluções no diretório de saída.
 */

int main() {
    // Lê a configuração usando readConfig
    Config config = readConfig("config.txt");
    
    // Cria o diretório de saída se não existir
    std::filesystem::create_directories(config.outputDir);

    // Imprimir as configurações lidas para debug
    std::cout << "Configurações lidas:" << std::endl;
    std::cout << "  Input Directory: " << config.inputDir << std::endl;
    std::cout << "  Output Directory: " << config.outputDir << std::endl;
    std::cout << "  Max Time: " << config.maxTime << std::endl;
    std::cout << "  Min Items: " << config.minItems << std::endl;
    std::cout << "  Max Items: " << config.maxItems << std::endl;
    std::cout << "  Objective: " << config.objective << std::endl;
    std::cout << "  Algorithm: " << config.algorithm << std::endl;
    std::cout << "  Epsilon: " << config.epsilon << std::endl;
    std::cout << "  Max Iterations: " << config.maxIterations << std::endl;
    std::cout << "  Validate Item Availability: " << config.validateItemAvailability << std::endl;
    std::cout << "  Validate Order IDs: " << config.validateOrderIds << std::endl;
    std::cout << std::endl;

    // Iterar sobre os arquivos de instância
    for (const auto& entry : std::filesystem::directory_iterator(config.inputDir)) {
        if (entry.path().extension() == ".txt") {
            std::string instancePath = entry.path().string();
            std::string outputPath = config.outputDir + "/" + entry.path().stem().string() + "_out.txt";

            std::cout << "Processando instância: " << instancePath << std::endl;

            // Ler a instância
            Instance instance = readInstance(instancePath);

            // Resolver o problema
            Solution solution = solveSPO(instance, config);

            // Escrever a solução
            writeSolution(solution, outputPath);

            // Validar a solução
            validateSolution(instance, solution, config);

            std::cout << "Solução escrita em: " << outputPath << std::endl;
            std::cout << std::endl;
        }
    }

    std::cout << "Processamento concluído." << std::endl;
    return 0;
}