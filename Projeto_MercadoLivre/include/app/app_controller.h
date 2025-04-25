#pragma once
#include <string>
#include <vector>
#include "config/objective_function.h"
#include "config/constraints_manager.h"
#include "core/warehouse.h"

class AppController {
public:
    AppController();
    
    // Método principal que executa o fluxo do programa
    int run();
    
private:
    // Configurações
    std::string objectiveFunctionFile;
    std::string constraintsFile;
    std::string instancesPath;
    int timeLimit;
    
    ObjectiveFunction objectiveFunction;
    ConstraintsManager constraintsManager;
    std::vector<std::string> instanceFiles;
    
    // Métodos privados para cada etapa do fluxo
    bool requestConfigFiles();
    bool loadConfigFiles();
    bool discoverInstances();
    void displayConfiguration();
    bool processInstances();
    
    // Métodos auxiliares
    bool fileExists(const std::string& filePath);
    std::vector<std::string> readConfigFile(const std::string& filePath);
    bool requestConfirmation(const std::string& message);
};