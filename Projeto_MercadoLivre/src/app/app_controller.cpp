#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <vector>
#include <utility>
#include <stdexcept>
#include "app/app_controller.h"
#include "input/input_parser.h"
#include "output/output_writer.h"

// Include the separate module files - CAMINHOS CORRIGIDOS
#include "modules/cria_auxiliares.h"
#include "modules/preprocess.h"
#include "modules/process.h"
#include "modules/postprocess.h"
#include "report/report_generator.h"
#include "utils/time_utils.h"

namespace fs = std::filesystem;

AppController::AppController() : timeLimit(300), outputPath("output") {
}

int AppController::run() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Fallback
    std::cout << "Executando com " << numThreads << " threads" << std::endl;

    std::cout << "====== Otimizador de Wave para Mercado Livre ======\n\n";
    
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
    
    while (true) {
        std::cout << "\n===== MENU PRINCIPAL =====\n";
        std::cout << "1. Processar instâncias\n";
        std::cout << "2. Gerar relatório HTML\n";
        std::cout << "0. Sair\n";
        std::cout << "Selecione uma opção: ";
        
        int option;
        std::cin >> option;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        switch (option) {
            case 0:
                return 0;
            case 1:
                if (!processInstances()) {
                    std::cerr << "Falha ao processar instâncias.\n";
                    return 1;
                }
                break;
            case 2:
                showReportMenu();
                break;
            default:
                std::cout << "Opção inválida.\n";
                break;
        }
    }
    
    return 0;
}

bool AppController::requestConfigFiles() {
    // Valores padrão
    std::string defaultObjectiveFunctionFile = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/config/funcao_objetivo.txt";
    std::string defaultConstraintsFile = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/config/restricoes.txt";
    std::string defaultInstancesPath = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/input";
    std::string defaultOutputPath = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/output";
    
    // Perguntar se o usuário quer usar valores padrão
    std::cout << "Deseja usar as configurações padrão? (s/n): ";
    std::string useDefault;
    std::getline(std::cin, useDefault);
    
    bool useDefaultValues = (useDefault == "s" || useDefault == "S");
    
    if (useDefaultValues) {
        // Usar valores padrão
        std::cout << "\nUtilizando configurações padrão:\n";
        
        objectiveFunctionFile = defaultObjectiveFunctionFile;
        std::cout << "Função objetivo: " << objectiveFunctionFile << std::endl;
        
        constraintsFile = defaultConstraintsFile;
        std::cout << "Restrições: " << constraintsFile << std::endl;
        
        instancesPath = defaultInstancesPath;
        std::cout << "Diretório de instâncias: " << instancesPath << std::endl;
        
        outputPath = defaultOutputPath;
        std::cout << "Diretório de saída: " << outputPath << std::endl;
        
        // Verificar se os arquivos e diretórios padrão existem
        bool allFilesExist = true;
        
        if (!fileExists(objectiveFunctionFile)) {
            std::cerr << "ERRO: Arquivo de função objetivo padrão não encontrado.\n";
            allFilesExist = false;
        }
        
        if (!fileExists(constraintsFile)) {
            std::cerr << "ERRO: Arquivo de restrições padrão não encontrado.\n";
            allFilesExist = false;
        }
        
        if (!fs::exists(instancesPath) || !fs::is_directory(instancesPath)) {
            std::cerr << "ERRO: Diretório de instâncias padrão não encontrado.\n";
            allFilesExist = false;
        }
        
        if (!allFilesExist) {
            std::cout << "\nAlguns arquivos ou diretórios padrão não existem. Por favor, informe os caminhos manualmente.\n\n";
            useDefaultValues = false;
        } else {
            // Solicitar apenas o tempo limite
            std::string timeLimitStr;
            std::cout << "\nTempo limite em segundos (máximo 600) [300]: ";
            std::getline(std::cin, timeLimitStr);
            
            if (!timeLimitStr.empty()) {
                try {
                    timeLimit = std::stoi(timeLimitStr);
                    if (timeLimit <= 0) {
                        std::cout << "Tempo limite inválido. Usando valor padrão (300 segundos).\n";
                        timeLimit = 300;
                    } else if (timeLimit > 600) {
                        std::cout << "Tempo limite excede o máximo permitido. Usando valor máximo (600 segundos).\n";
                        timeLimit = 600;
                    }
                } catch (...) {
                    std::cout << "Tempo limite inválido. Usando valor padrão (300 segundos).\n";
                    timeLimit = 300;
                }
            }
            
            return true;
        }
    }
    
    // Se não usar valores padrão ou algum arquivo padrão não existe, continue com o fluxo normal
    
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
    
    // 4. Solicitar diretório de saída
    std::cout << "Diretório para salvar as soluções [output]: ";
    std::string outputPathStr;
    std::getline(std::cin, outputPathStr);
    
    if (!outputPathStr.empty()) {
        outputPath = outputPathStr;
    }
    
    // 5. Solicitar tempo limite (agora por último)
    std::string timeLimitStr;
    std::cout << "Tempo limite em segundos (máximo 600) [300]: ";
    std::getline(std::cin, timeLimitStr);
    
    if (!timeLimitStr.empty()) {
        try {
            timeLimit = std::stoi(timeLimitStr);
            if (timeLimit <= 0) {
                std::cout << "Tempo limite inválido. Usando valor padrão (300 segundos).\n";
                timeLimit = 300;
            } else if (timeLimit > 600) {
                std::cout << "Tempo limite excede o máximo permitido. Usando valor máximo (600 segundos).\n";
                timeLimit = 600;
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

//#################################################################################################
//                                   Processamento das instâncias
//#################################################################################################
bool AppController::processInstances() {
    std::cout << "\nIniciando processamento das instâncias...\n";
    
    InputParser parser;
    OutputWriter writer;
    
    // Métricas globais
    auto globalStartTime = std::chrono::high_resolution_clock::now();
    std::vector<std::pair<std::string, double>> instanceTimes;
    
    for (size_t i = 0; i < instanceFiles.size(); i++) {
        const auto& instanceFile = instanceFiles[i];
        std::cout << "\n[" << (i+1) << "/" << instanceFiles.size() << "] Processando: " << instanceFile << std::endl;
        
        try {
            // Iniciar cronômetro global para esta instância
            auto instanceStartTime = std::chrono::high_resolution_clock::now();
            
            // Ler a instância e armazenar dados básicos
            Warehouse warehouse = parser.parseFile(instanceFile);
            
            // Exibir informações básicas da instância
            std::cout << "  Número de pedidos: " << warehouse.numOrders << std::endl;
            std::cout << "  Número de itens: " << warehouse.numItems << std::endl;
            std::cout << "  Número de corredores: " << warehouse.numCorridors << std::endl;
            std::cout << "  LB: " << warehouse.LB << ", UB: " << warehouse.UB << std::endl;
            
            // Calcular tempo restante antes de cada módulo
            auto getCurrentRemainingTime = [&]() -> double {
                auto currentTime = std::chrono::high_resolution_clock::now();
                double elapsedSecs = std::chrono::duration<double>(currentTime - instanceStartTime).count();
                return std::max(0.0, timeLimit - elapsedSecs);
            };
            
            // Preparar solução
            Solution solution;
            
            // ETAPA 1: Estruturas auxiliares
            double remainingTime = getCurrentRemainingTime();
            if (remainingTime <= 0) {
                std::cout << "  Tempo limite excedido antes de iniciar processamento" << std::endl;
                throw std::runtime_error("Tempo limite excedido");
            }
            
            if (!executeModuleCriaAuxiliares(warehouse, solution)) {
                throw std::runtime_error("Falha na criação de estruturas auxiliares");
            }
            
            // ETAPA 2: Pré-processamento
            remainingTime = getCurrentRemainingTime();
            if (remainingTime <= 0) {
                std::cout << "  Tempo limite excedido após criação de estruturas" << std::endl;
                throw std::runtime_error("Tempo limite excedido");
            }
            
            std::cout << "----------------------------------" << std::endl;
            std::cout << "  Executando: pré-processamento (tempo restante: " 
                      << remainingTime << "s)..." << std::endl;
            std::cout << "----------------------------------" << std::endl;
            
            if (!executeModulePreprocess(warehouse, solution)) {
                throw std::runtime_error("Falha no pré-processamento");
            }
            
            // ETAPA 3: Processamento principal
            remainingTime = getCurrentRemainingTime();
            if (remainingTime <= 0) {
                std::cout << "  Tempo limite excedido após pré-processamento" << std::endl;
                // Se temos solução viável, salvamos mesmo assim
                if (solution.isFeasible()) {
                    goto save_solution;
                }
                throw std::runtime_error("Tempo limite excedido sem solução viável");
            }
            
            std::cout << "----------------------------------------" << std::endl;
            std::cout << "  Executando: processamento principal (tempo restante: " 
                      << remainingTime << "s)..." << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            
            // Passar o tempo restante para o módulo
            if (!executeModuleProcess(warehouse, solution, remainingTime)) {
                // Verificar se falhou por tempo ou outro motivo
                if (getCurrentRemainingTime() <= 0) {
                    std::cout << "  Tempo limite atingido durante processamento" << std::endl;
                    // Se temos solução viável, seguimos para salvar
                    if (solution.isFeasible()) {
                        goto save_solution;
                    }
                    throw std::runtime_error("Tempo limite excedido sem solução viável");
                }
                throw std::runtime_error("Falha no processamento principal");
            }
            
            // ETAPA 4: Pós-processamento (apenas se houver tempo)
            remainingTime = getCurrentRemainingTime();
            if (remainingTime > 0) {
                std::cout << "----------------------------------" << std::endl;
                std::cout << "  Executando: pós-processamento (tempo restante: " 
                          << remainingTime << "s)..." << std::endl;
                std::cout << "----------------------------------" << std::endl;
                
                if (!executeModulePostprocess(warehouse, solution, remainingTime)) {
                    std::cout << "  Aviso: pós-processamento não completado" << std::endl;
                }
            } else {
                std::cout << "  Pulando pós-processamento (tempo esgotado)" << std::endl;
            }
            
        save_solution:
            // Salvar a solução se for viável
            if (solution.isFeasible()) {
                std::string inputFilename = fs::path(instanceFile).filename().string();
                std::string outputFile = outputPath + "/" + inputFilename + "_solution.txt";
                
                // Garantir que o diretório de saída existe
                if (!fs::exists(outputPath)) {
                    fs::create_directories(outputPath);
                }
                
                // Salvar solução usando formato correto
                OutputWriter writer;
                if (writer.writeSolution(solution, outputFile)) {
                    std::cout << "  Solução salva em: " << outputFile << std::endl;
                } else {
                    std::cout << "  ERRO: Falha ao salvar solução" << std::endl;
                }
            } else {
                std::cout << "  AVISO: Nenhuma solução viável encontrada para salvar" << std::endl;
            }
            
            // Encerrar cronômetro para esta instância
            auto instanceEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> instanceElapsed = instanceEndTime - instanceStartTime;
            std::cout << "  Tempo de processamento: " << instanceElapsed.count() << " segundos" << std::endl;
            
            // Armazenar tempo para estatísticas
            instanceTimes.push_back({fs::path(instanceFile).filename().string(), instanceElapsed.count()});
            
        } catch (const std::exception& e) {
            std::cerr << "  ERRO: " << e.what() << std::endl;
        }
    }
    
    // Encerrar cronômetro global
    auto globalEndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> globalElapsed = globalEndTime - globalStartTime;
    
    // Exibir resumo

    std::cout << "\n===== RESUMO DO PROCESSAMENTO =====\n";
    std::cout << "Total de instâncias processadas: " << instanceTimes.size() << "/" << instanceFiles.size() << std::endl;
    std::cout << "Tempo total de processamento: " << globalElapsed.count() << " segundos\n\n";
    std::cout << "Tempos por instância:\n";
    
    for (const auto& [instance, time] : instanceTimes) {
        std::cout << "  " << instance << ": " << time << " segundos\n";
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

// Implementação dos métodos de módulos que utilizam os módulos importados
bool AppController::executeModuleCriaAuxiliares(const Warehouse& warehouse, Solution& solution, double remainingTime) {
    // Chamar a implementação do módulo cria_auxiliares
    // Create auxiliary data structures needed before building the solution
    return cria_auxiliares(warehouse, solution);
}

bool AppController::executeModulePreprocess(const Warehouse& warehouse, Solution& solution, double remainingTime) {
    // Chamar a implementação do módulo preprocess
    return preprocess(warehouse, solution);
}

bool AppController::executeModuleProcess(const Warehouse& warehouse, Solution& solution, double remainingTime) {
    // Corrigir chamada da função passando o tempo restante
    return process(warehouse, solution, remainingTime);
}

bool AppController::executeModulePostprocess(const Warehouse& warehouse, Solution& solution, double remainingTime) {
    // Chamar a implementação do módulo postprocess
    return postprocess(warehouse, solution);
}

bool AppController::showReportMenu() {
    std::cout << "\n===== GERAÇÃO DE RELATÓRIO =====\n";
    
    // Verificar se temos instâncias descobertas
    if (instanceFiles.empty()) {
        if (!discoverInstances()) {
            return false;
        }
    }
    
    // Mostrar instâncias disponíveis
    std::cout << "Instâncias disponíveis:\n";
    for (size_t i = 0; i < instanceFiles.size(); i++) {
        std::cout << "  " << (i+1) << ". " << instanceFiles[i] << "\n";
    }
    std::cout << "\n";
    
    // Pedir ao usuário para selecionar uma instância
    int selection;
    std::cout << "Selecione o número da instância para gerar o relatório (0 para voltar): ";
    std::cin >> selection;
    
    if (selection == 0) {
        return true;
    }
    
    if (selection < 1 || selection > (int)instanceFiles.size()) {
        std::cout << "Seleção inválida.\n";
        return false;
    }
    
    std::string selectedInstance = instanceFiles[selection - 1];
    
    // Definir pasta para salvar o relatório
    std::string reportPath = "reports";
    if (!fs::exists(reportPath)) {
        fs::create_directories(reportPath);
    }
    
    // Gerar o relatório
    bool success = ReportGenerator::generateReport(selectedInstance, reportPath);
    
    if (success) {
        std::cout << "Relatório gerado com sucesso na pasta '" << reportPath << "'.\n";
    } else {
        std::cout << "Falha ao gerar o relatório.\n";
    }
    
    return success;
}
