#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config_structures.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <sstream>

/**
 * Classe singleton para gerenciar todas as configurações do sistema
 */
class ConfigManager {
public:
    // Obter a instância singleton
    static ConfigManager& getInstance();
    
    // Carregar todas as configurações
    void loadAllConfigs(const std::string& configDir);
    
    // Métodos de acesso a configurações específicas
    ProblemConfig getProblemConfig() const;
    ObjectiveConfig getObjectiveConfig() const;
    std::vector<ConstraintConfig> getConstraintConfigs() const;
    AlgorithmConfig getAlgorithmConfig() const;
    InputFormatConfig getInputFormatConfig() const;
    OutputFormatConfig getOutputFormatConfig() const;
    
    // Métodos de acesso a pares chave-valor
    bool hasCategory(const std::string& category) const;
    bool hasKey(const std::string& category, const std::string& key) const;
    std::string getString(const std::string& category, const std::string& key) const;
    int getInt(const std::string& category, const std::string& key) const;
    double getDouble(const std::string& category, const std::string& key) const;
    
    // MÉTODOS FALTANTES QUE CAUSARAM OS ERROS
    bool getBool(const std::string& category, const std::string& key) const;
    std::vector<std::string> getKeys(const std::string& category) const;
    std::vector<std::string> getCategories() const;
    
    // Exibir todas as configurações
    void printAllConfigs() const;

private:
    // Construtor privado (padrão singleton)
    ConfigManager() = default;
    
    // Mapa de configurações: categoria -> (chave -> valor)
    std::map<std::string, std::map<std::string, std::string>> configs;
    
    // Categorias e seus títulos amigáveis
    std::map<std::string, std::string> categoryTitles;
    
    // Configurações estruturadas
    ProblemConfig problemConfig;
    ObjectiveConfig objectiveConfig;
    std::vector<ConstraintConfig> constraintConfigs;
    AlgorithmConfig algorithmConfig;
    InputFormatConfig inputFormatConfig;
    OutputFormatConfig outputFormatConfig;
    
    // Métodos auxiliares para construir as configurações
    void buildProblemConfig();
    void buildObjectiveConfig();
    void buildConstraintConfigs();
    void buildAlgorithmConfig();
    void buildInputFormatConfig();
    void buildOutputFormatConfig();
};

#endif // CONFIG_MANAGER_H