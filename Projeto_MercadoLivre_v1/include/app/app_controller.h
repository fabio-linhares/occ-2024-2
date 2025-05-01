#pragma once

#include "core/warehouse.h"
#include "core/solution.h"
#include <string>
#include <vector>
#include <map>

/**
 * @brief Controlador principal da aplicação
 */
class AppController {
private:
    // Configurações
    std::string configFile;
    std::string inputDir;
    std::string outputDir;
    double timeLimit;
    
    // Estado interno
    std::vector<std::string> instanceFiles;
    std::map<std::string, double> instanceTimes;
    
    // Métodos privados
    bool discoverInstances();
    void confirmDirectories();
    void displayInstanceInfo(const Warehouse& warehouse, const std::string& fileName);
    void generateExampleOutput(const Warehouse& warehouse, const std::string& fileName);
    
public:
    // Construtores
    AppController();
    AppController(const std::string& configFile, 
                  const std::string& inputDir,
                  const std::string& outputDir,
                  double timeLimit);
    
    // Métodos principais
    int run();
    bool processInstances();
    void showMenu();
    void showDebugMenu();
    void testParser();
};