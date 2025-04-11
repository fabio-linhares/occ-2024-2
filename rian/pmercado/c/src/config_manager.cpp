#include "../include/config_manager.h"
#include "../include/config_extractor.h"
#include <iostream>
#include <algorithm>
#include <regex>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::loadAllConfigs(const std::string& configDir) {
    // Lista de arquivos de configuração e suas categorias correspondentes
    std::vector<std::pair<std::string, std::string>> configFiles = {
        {"problem_definition.txt", "problem"},
        {"objective_function.txt", "objective"},
        {"constraints.txt", "constraints"},
        {"algorithm_configuration.txt", "algorithm"},
        {"data_structures.txt", "data_structures"},
        {"variable_definitions.txt", "variables"},
        {"input_instance_format.txt", "input_format"},
        {"output_solution_format.txt", "output_format"}
    };
    
    // Mapear categorias para títulos amigáveis
    categoryTitles = {
        {"problem", "Definição do Problema"},
        {"objective", "Função Objetivo"},
        {"constraints", "Restrições"},
        {"algorithm", "Configuração do Algoritmo"},
        {"data_structures", "Estruturas de Dados"},
        {"variables", "Definições de Variáveis"},
        {"input_format", "Formato de Entrada"},
        {"output_format", "Formato de Saída"}
    };
    
    // Carregar cada arquivo de configuração
    for (const auto& [filename, category] : configFiles) {
        const std::string filepath = configDir + filename;
        auto config = parseConfigFile(filepath);
        configs[category] = config;
    }
    
    // Construir as estruturas de configuração
    buildProblemConfig();
    buildObjectiveConfig();
    buildConstraintConfigs();
    buildAlgorithmConfig();
    buildInputFormatConfig();
    buildOutputFormatConfig();
}

bool ConfigManager::hasCategory(const std::string& category) const {
    return configs.find(category) != configs.end();
}

bool ConfigManager::hasKey(const std::string& category, const std::string& key) const {
    if (!hasCategory(category)) return false;
    return configs.at(category).find(key) != configs.at(category).end();
}

std::string ConfigManager::getString(const std::string& category, const std::string& key) const {
    if (!hasKey(category, key)) {
        throw std::out_of_range("Chave '" + key + "' não encontrada na categoria '" + category + "'");
    }
    return configs.at(category).at(key);
}

int ConfigManager::getInt(const std::string& category, const std::string& key) const {
    std::string value = getString(category, key);
    try {
        return std::stoi(value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Não foi possível converter '" + value + "' para int");
    }
}

double ConfigManager::getDouble(const std::string& category, const std::string& key) const {
    std::string value = getString(category, key);
    try {
        return std::stod(value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Não foi possível converter '" + value + "' para double");
    }
}

bool ConfigManager::getBool(const std::string& category, const std::string& key) const {
    std::string value = getString(category, key);
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value == "true" || value == "1" || value == "yes" || value == "sim";
}

std::vector<std::string> ConfigManager::getKeys(const std::string& category) const {
    if (!hasCategory(category)) {
        throw std::out_of_range("Categoria '" + category + "' não encontrada");
    }
    
    std::vector<std::string> keys;
    for (const auto& [key, _] : configs.at(category)) {
        keys.push_back(key);
    }
    return keys;
}

std::vector<std::string> ConfigManager::getCategories() const {
    std::vector<std::string> categories;
    for (const auto& [category, _] : configs) {
        categories.push_back(category);
    }
    return categories;
}

void ConfigManager::buildProblemConfig() {
    problemConfig.name = getString("problem", "PROBLEM_NAME");
    problemConfig.type = getString("problem", "PROBLEM_TYPE");
    problemConfig.objective_function = getString("problem", "OBJECTIVE_FUNCTION");
    problemConfig.algorithm = getString("problem", "ALGORITHM");
    problemConfig.time_limit = getInt("problem", "TIME_LIMIT");
    problemConfig.description = getString("problem", "DESCRIPTION");
}

void ConfigManager::buildObjectiveConfig() {
    objectiveConfig.name = getString("objective", "NAME");
    objectiveConfig.description = getString("objective", "DESCRIPTION");
    objectiveConfig.formula = getString("objective", "FORMULA");
    objectiveConfig.implementation = getString("objective", "IMPLEMENTATION");
    
    // Extrair variáveis como lista
    std::string varsString = getString("objective", "VARIABLES");
    std::istringstream varStream(varsString);
    std::string var;
    while (std::getline(varStream, var, ',')) {
        // Remover espaços em branco
        var.erase(0, var.find_first_not_of(" \t"));
        var.erase(var.find_last_not_of(" \t") + 1);
        if (!var.empty()) {
            objectiveConfig.variables.push_back(var);
        }
    }
    
    // Extrair descrições das variáveis
    std::string varDescString = getString("objective", "VARIABLE_DESCRIPTIONS");
    std::istringstream descStream(varDescString);
    std::string line;
    while (std::getline(descStream, line)) {
        // Remover espaços em branco iniciais
        line.erase(0, line.find_first_not_of(" \t"));
        
        // Encontrar o separador (:)
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string varName = line.substr(0, colonPos);
            std::string varDesc = line.substr(colonPos + 1);
            
            // Remover espaços em branco
            varName.erase(0, varName.find_first_not_of(" \t"));
            varName.erase(varName.find_last_not_of(" \t") + 1);
            varDesc.erase(0, varDesc.find_first_not_of(" \t"));
            varDesc.erase(varDesc.find_last_not_of(" \t") + 1);
            
            objectiveConfig.variable_descriptions[varName] = varDesc;
        }
    }
}

void ConfigManager::buildConstraintConfigs() {
    // Encontrar todas as chaves que correspondem a CONSTRAINT_X_NAME
    std::regex constraintNamePattern("CONSTRAINT_(\\d+)_NAME");
    std::map<int, ConstraintConfig> tempConstraints;
    
    for (const auto& [key, value] : configs["constraints"]) {
        std::smatch match;
        if (std::regex_match(key, match, constraintNamePattern)) {
            int constraintId = std::stoi(match[1]);
            tempConstraints[constraintId].name = value;
        }
    }
    
    // Preencher os detalhes de cada restrição
    for (auto& [id, constraint] : tempConstraints) {
        std::string idStr = std::to_string(id);
        constraint.description = getString("constraints", "CONSTRAINT_" + idStr + "_DESCRIPTION");
        constraint.formula = getString("constraints", "CONSTRAINT_" + idStr + "_FORMULA");
        constraint.implementation = getString("constraints", "CONSTRAINT_" + idStr + "_IMPLEMENTATION");
    }
    
    // Converter para um vetor ordenado por ID
    constraintConfigs.clear();
    for (const auto& [id, constraint] : tempConstraints) {
        constraintConfigs.push_back(constraint);
    }
}

void ConfigManager::buildAlgorithmConfig() {
    algorithmConfig.name = getString("algorithm", "ALGORITHM_NAME");
    algorithmConfig.type = getString("algorithm", "ALGORITHM_TYPE");
    algorithmConfig.epsilon = getDouble("algorithm", "PARAMETER_1_VALUE");
    algorithmConfig.max_iterations = getInt("algorithm", "PARAMETER_2_VALUE");
}

void ConfigManager::buildInputFormatConfig() {
    inputFormatConfig.line_1 = getString("input_format", "LINE_1");
    inputFormatConfig.line_2_to_o_plus_1 = getString("input_format", "LINE_2_TO_O+1");
    inputFormatConfig.line_o_plus_2_to_o_plus_a_plus_1 = getString("input_format", "LINE_O+2_TO_O+A+1");
    inputFormatConfig.last_line = getString("input_format", "LAST_LINE");
}

void ConfigManager::buildOutputFormatConfig() {
    outputFormatConfig.line_1 = getString("output_format", "LINE_1");
    outputFormatConfig.line_2_to_n_plus_1 = getString("output_format", "LINE_2_TO_N+1");
    outputFormatConfig.line_n_plus_2 = getString("output_format", "LINE_N+2");
    outputFormatConfig.line_n_plus_3_to_n_plus_m_plus_2 = getString("output_format", "LINE_N+3_TO_N+M+2");
}

ProblemConfig ConfigManager::getProblemConfig() const {
    return problemConfig;
}

ObjectiveConfig ConfigManager::getObjectiveConfig() const {
    return objectiveConfig;
}

std::vector<ConstraintConfig> ConfigManager::getConstraintConfigs() const {
    return constraintConfigs;
}

AlgorithmConfig ConfigManager::getAlgorithmConfig() const {
    return algorithmConfig;
}

InputFormatConfig ConfigManager::getInputFormatConfig() const {
    return inputFormatConfig;
}

OutputFormatConfig ConfigManager::getOutputFormatConfig() const {
    return outputFormatConfig;
}

void ConfigManager::printAllConfigs() const {
    std::cout << "CONFIGURAÇÕES CARREGADAS" << std::endl;
    std::cout << "=======================" << std::endl;
    
    // Imprimir configuração do problema
    std::cout << "\n=== Definição do Problema ===" << std::endl;
    std::cout << "Nome: " << problemConfig.name << std::endl;
    std::cout << "Tipo: " << problemConfig.type << std::endl;
    std::cout << "Função Objetivo: " << problemConfig.objective_function << std::endl;
    std::cout << "Algoritmo: " << problemConfig.algorithm << std::endl;
    std::cout << "Limite de Tempo: " << problemConfig.time_limit << " segundos" << std::endl;
    std::cout << "Descrição: " << problemConfig.description << std::endl;
    
    // Imprimir função objetivo
    std::cout << "\n=== Função Objetivo ===" << std::endl;
    std::cout << "Nome: " << objectiveConfig.name << std::endl;
    std::cout << "Descrição: " << objectiveConfig.description << std::endl;
    std::cout << "Fórmula: " << objectiveConfig.formula << std::endl;
    std::cout << "Implementação: [CÓDIGO C++]" << std::endl;
    
    // Imprimir variáveis
    std::cout << "Variáveis:" << std::endl;
    for (const auto& var : objectiveConfig.variables) {
        std::cout << "  - " << var;
        if (objectiveConfig.variable_descriptions.count(var)) {
            std::cout << ": " << objectiveConfig.variable_descriptions.at(var);
        }
        std::cout << std::endl;
    }
    
    // Imprimir restrições
    std::cout << "\n=== Restrições ===" << std::endl;
    for (size_t i = 0; i < constraintConfigs.size(); ++i) {
        const auto& constraint = constraintConfigs[i];
        std::cout << (i+1) << ". " << constraint.name << std::endl;
        std::cout << "   Descrição: " << constraint.description << std::endl;
        std::cout << "   Fórmula: " << constraint.formula << std::endl;
        std::cout << "   Implementação: [CÓDIGO C++]" << std::endl;
        std::cout << std::endl;
    }
    
    // Imprimir configuração do algoritmo
    std::cout << "\n=== Configuração do Algoritmo ===" << std::endl;
    std::cout << "Nome: " << algorithmConfig.name << std::endl;
    std::cout << "Tipo: " << algorithmConfig.type << std::endl;
    std::cout << "Epsilon: " << algorithmConfig.epsilon << std::endl;
    std::cout << "Iterações Máximas: " << algorithmConfig.max_iterations << std::endl;
}