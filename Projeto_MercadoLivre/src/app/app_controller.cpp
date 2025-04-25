#include "app/app_controller.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include "input/input_parser.h"
#include "algorithm/greedy_algorithm.h"
#include "output/output_writer.h"

namespace fs = std::filesystem;

AppController::AppController() : timeLimit(300) {
}

int AppController::run() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "====== Otimizador de Wave para Mercado Livre ======\n\n";
    
    // Fluxo principal do programa
    if (!requestConfigFiles()) {
        return 1;
    }
    
    if (!loadConfigFiles()) {
        return 1;
    }
    
    if (!discoverInstances()) {
        return 1;
    }
    
    displayConfiguration();
    
    if (!requestConfirmation("Iniciar processamento com esta configuração?")) {
        std::cout << "Processamento cancelado pelo usuário.\n";
        return 0;
    }
    
    if (!processInstances()) {
        return 1;
    }
    
    // Exibir tempo total
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cout << "\nProcessamento concluído em " << elapsed.count() << " segundos.\n";
    
    return 0;
}

bool AppController::requestConfigFiles() {
    // 1. Solicitar arquivo de função objetivo
    while (true) {
        std::cout << "Caminho do arquivo de função objetivo: ";
        std::getline(std::cin, objectiveFunctionFile);
        
        if (objectiveFunctionFile.empty()) {
            std::cout << "O caminho não pode ser vazio. Tente novamente.\n";
            continue;
        }
        
        if (!fileExists(objectiveFunctionFile)) {
            std::cout << "Arquivo não encontrado. Tente novamente.\n";
            continue;
        }
        
        break;
    }
    
    // 2. Solicitar arquivo de restrições
    while (true) {
        std::cout << "Caminho do arquivo de restrições: ";
        std::getline(std::cin, constraintsFile);
        
        if (constraintsFile.empty()) {
            std::cout << "O caminho não pode ser vazio. Tente novamente.\n";
            continue;
        }
        
        if (!fileExists(constraintsFile)) {
            std::cout << "Arquivo não encontrado. Tente novamente.\n";
            continue;
        }
        
        break;
    }
    
    // 3. Solicitar diretório das instâncias
    while (true) {
        std::cout << "Diretório das instâncias: ";
        std::getline(std::cin, instancesPath);
        
        if (instancesPath.empty()) {
            std::cout << "O caminho não pode ser vazio. Tente novamente.\n";
            continue;
        }
        
        if (!fs::exists(instancesPath) || !fs::is_directory(instancesPath)) {
            std::cout << "Diretório não encontrado. Tente novamente.\n";
            continue;
        }
        
        break;
    }
    
    // 4. Solicitar tempo limite
    std::string timeLimitStr;
    std::cout << "Tempo limite em segundos [300]: ";
    std::getline(std::cin, timeLimitStr);
    
    if (!timeLimitStr.empty()) {
        try {
            timeLimit = std::stoi(timeLimitStr);
            if (timeLimit <= 0) {
                std::cout << "Tempo limite inválido. Usando valor padrão (300 segundos).\n";
                timeLimit = 300;
            }
        } catch (...) {
            std::cout << "Tempo limite inválido. Usando valor padrão (300 segundos).\n";
            timeLimit = 300;
        }
    }
    
    return true;
}

bool AppController::loadConfigFiles() {
    try {
        // Carregar função objetivo
        if (!objectiveFunction.loadFromFile(objectiveFunctionFile)) {
            std::cerr << "Erro ao carregar função objetivo.\n";
            return false;
        }
        
        // Carregar restrições
        if (!constraintsManager.loadFromFile(constraintsFile)) {
            std::cerr << "Erro ao carregar restrições.\n";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Erro ao carregar configurações: " << e.what() << std::endl;
        return false;
    }
}

bool AppController::discoverInstances() {
    try {
        instanceFiles.clear();
        
        for (const auto& entry : fs::directory_iterator(instancesPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                instanceFiles.push_back(entry.path().string());
            }
        }
        
        if (instanceFiles.empty()) {
            std::cerr << "Nenhum arquivo de instância .txt encontrado no diretório.\n";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Erro ao listar arquivos de instância: " << e.what() << std::endl;
        return false;
    }
}

void AppController::displayConfiguration() {
    std::cout << "\n===== CONFIGURAÇÃO =====\n";
    std::cout << "Função objetivo: " << objectiveFunction.getDescription() << "\n\n";
    
    std::cout << "Restrições:\n";
    for (const auto& constraint : constraintsManager.getConstraintDescriptions()) {
        std::cout << "  - " << constraint << "\n";
    }
    std::cout << "\n";
    
    std::cout << "Tempo limite: " << timeLimit << " segundos\n\n";
    
    std::cout << "Instâncias a processar (" << instanceFiles.size() << "):\n";
    for (size_t i = 0; i < instanceFiles.size(); i++) {
        std::cout << "  " << (i+1) << ". " << instanceFiles[i] << "\n";
    }
    std::cout << "\n";
}

bool AppController::processInstances() {
    std::cout << "\nIniciando processamento das instâncias...\n";
    
    InputParser parser;
    OutputWriter writer;
    
    for (size_t i = 0; i < instanceFiles.size(); i++) {
        const auto& instanceFile = instanceFiles[i];
        std::cout << "\n[" << (i+1) << "/" << instanceFiles.size() << "] Processando: " << instanceFile << std::endl;
        
        try {
            // Ler a instância
            Warehouse warehouse = parser.parseFile(instanceFile);
            
            std::cout << "  Número de pedidos: " << warehouse.numOrders << std::endl;
            std::cout << "  Número de itens: " << warehouse.numItems << std::endl;
            std::cout << "  Número de corredores: " << warehouse.numCorridors << std::endl;
            std::cout << "  LB: " << warehouse.LB << ", UB: " << warehouse.UB << std::endl;
            
            // Criar e executar o algoritmo
            GreedyAlgorithm algorithm;
            Solution solution = algorithm.solve(warehouse);
            
            // Validar a solução
            if (constraintsManager.validate(solution, warehouse)) {
                std::cout << "  Solução viável encontrada.\n";
            } else {
                std::cout << "  AVISO: Solução não viável.\n";
            }
            
            // Calcular valor da função objetivo
            double objectiveValue = objectiveFunction.evaluate(solution, warehouse);
            std::cout << "  Valor função objetivo: " << objectiveValue << std::endl;
            
            // Estatísticas da solução
            std::cout << "  Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
            std::cout << "  Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
            
            // Salvar a solução
            std::string outputFile = fs::path(instanceFile).stem().string() + "_solution.txt";
            writer.writeSolution(solution, outputFile);
            std::cout << "  Solução salva em: " << outputFile << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "  ERRO: " << e.what() << std::endl;
        }
    }
    
    return true;
}

bool AppController::fileExists(const std::string& filePath) {
    return fs::exists(filePath);
}

std::vector<std::string> AppController::readConfigFile(const std::string& filePath) {
    std::vector<std::string> lines;
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Não foi possível abrir o arquivo: " + filePath);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') { // Ignorar linhas vazias e comentários
            lines.push_back(line);
        }
    }
    
    return lines;
}

bool AppController::requestConfirmation(const std::string& message) {
    std::string confirm;
    std::cout << message << " (s/n): ";
    std::getline(std::cin, confirm);
    
    return (confirm == "s" || confirm == "S");
}