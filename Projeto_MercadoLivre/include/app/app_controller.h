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
    // Métodos existentes
    bool requestConfigFiles();
    bool loadConfigFiles();
    bool discoverInstances();
    void displayConfiguration();
    bool processInstances();
    bool fileExists(const std::string& filePath);
    std::vector<std::string> readConfigFile(const std::string& filePath);
    bool requestConfirmation(const std::string& message);
    
    // Métodos para processamento modular
    bool executeModuleCriaAuxiliares(const Warehouse& warehouse, Solution& solution);
    bool executeModulePreprocess(const Warehouse& warehouse, Solution& solution);
    bool executeModuleProcess(const Warehouse& warehouse, Solution& solution);
    bool executeModulePostprocess(const Warehouse& warehouse, Solution& solution);
    
    // Atributos existentes
    std::string objectiveFunctionFile;
    std::string constraintsFile;
    std::string instancesPath;
    std::string outputPath;
    int timeLimit;
    std::vector<std::string> instanceFiles;
    ObjectiveFunction objectiveFunction;
    ConstraintsManager constraintsManager;
};

