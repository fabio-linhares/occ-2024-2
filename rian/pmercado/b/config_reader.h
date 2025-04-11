#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <string>

struct Config {
    // Parâmetros básicos
    std::string inputDir;
    std::string outputDir;
    int maxTime;
    int minItems;
    int maxItems;
    std::string objective;
    std::string algorithm;
    double epsilon;
    int maxIterations;
    bool validateItemAvailability;
    bool validateOrderIds;
    
    // Parâmetros adicionais
    int timeLimitPercentage;
    
    // Limites de estruturas de dados
    int maxItemsRuntime;
    int maxAislesRuntime;
    int maxOrdersRuntime;
    
    // Parâmetros do VNS
    int maxNeighborhoods;
    int vnsMaxIterationsWithoutImprovement;
    
    // Paralelização
    int maxThreads;
    
    // Fatores de tempo
    double timeFactorStop;
    double timeFactorStrategyChange;
    
    // Parâmetros de perturbação
    double perturbationFactor;
};

Config readConfig(const std::string& path);

#endif // CONFIG_READER_H