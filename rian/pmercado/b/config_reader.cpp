#include "config_reader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Config readConfig(const std::string& path) {
    Config config;
    std::ifstream file(path);
    std::string line;
    
    // Definir valores padrão
    config.inputDir = "./instances";
    config.outputDir = "./solutions";
    config.maxTime = 600;
    config.minItems = 1000;
    config.maxItems = 2000;
    config.objective = "maximize_items_per_aisle";
    config.algorithm = "dinkelbach";
    config.epsilon = 1e-6;
    config.maxIterations = 100;
    config.validateItemAvailability = true;
    config.validateOrderIds = true;
    config.timeLimitPercentage = 95;
    config.maxItemsRuntime = 10000;
    config.maxAislesRuntime = 1000;
    config.maxOrdersRuntime = 1000;
    config.maxNeighborhoods = 3;
    config.vnsMaxIterationsWithoutImprovement = 5;
    config.maxThreads = 8;
    config.timeFactorStop = 0.95;
    config.timeFactorStrategyChange = 0.8;
    config.perturbationFactor = 0.33;
    
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de configuração: " << path << std::endl;
        return config;
    }
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string key, value;
        
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            // Remover espaços em branco e comentários
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            
            // Remover comentários da parte de valor
            size_t commentPos = value.find('#');
            if (commentPos != std::string::npos) {
                value = value.substr(0, commentPos);
            }
            
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (key == "INPUT_DIR") config.inputDir = value;
            else if (key == "OUTPUT_DIR") config.outputDir = value;
            else if (key == "MAX_TIME") config.maxTime = std::stoi(value);
            else if (key == "MIN_ITEMS") config.minItems = std::stoi(value);
            else if (key == "MAX_ITEMS") config.maxItems = std::stoi(value);
            else if (key == "OBJECTIVE") config.objective = value;
            else if (key == "ALGORITHM") config.algorithm = value;
            else if (key == "EPSILON") config.epsilon = std::stod(value);
            else if (key == "MAX_ITERATIONS") config.maxIterations = std::stoi(value);
            else if (key == "VALIDATE_ITEM_AVAILABILITY") config.validateItemAvailability = (value == "true");
            else if (key == "VALIDATE_ORDER_IDS") config.validateOrderIds = (value == "true");
            else if (key == "TIME_LIMIT_PERCENTAGE") config.timeLimitPercentage = std::stoi(value);
            else if (key == "MAX_ITEMS_RUNTIME") config.maxItemsRuntime = std::stoi(value);
            else if (key == "MAX_AISLES_RUNTIME") config.maxAislesRuntime = std::stoi(value);
            else if (key == "MAX_ORDERS_RUNTIME") config.maxOrdersRuntime = std::stoi(value);
            else if (key == "MAX_NEIGHBORHOODS") config.maxNeighborhoods = std::stoi(value);
            else if (key == "VNS_MAX_ITERATIONS_WITHOUT_IMPROVEMENT") config.vnsMaxIterationsWithoutImprovement = std::stoi(value);
            else if (key == "MAX_THREADS") config.maxThreads = std::stoi(value);
            else if (key == "TIME_FACTOR_STOP") config.timeFactorStop = std::stod(value);
            else if (key == "TIME_FACTOR_STRATEGY_CHANGE") config.timeFactorStrategyChange = std::stod(value);
            else if (key == "PERTURBATION_FACTOR") config.perturbationFactor = std::stod(value);
        }
    }
    
    file.close();
    
    // Log de confirmação das configurações lidas
    std::cout << "Configurações carregadas do arquivo: " << path << std::endl;
    std::cout << "  - Diretórios: entrada=" << config.inputDir << ", saída=" << config.outputDir << std::endl;
    std::cout << "  - Tempo máximo: " << config.maxTime << " segundos" << std::endl;
    std::cout << "  - Itens: min=" << config.minItems << ", max=" << config.maxItems << std::endl;
    std::cout << "  - Parâmetros VNS: max vizinhanças=" << config.maxNeighborhoods 
              << ", max iter sem melhoria=" << config.vnsMaxIterationsWithoutImprovement << std::endl;
    std::cout << "  - Paralelização: max threads=" << config.maxThreads << std::endl;
    
    return config;
}