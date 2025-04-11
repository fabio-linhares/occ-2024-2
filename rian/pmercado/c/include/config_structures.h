#ifndef CONFIG_STRUCTURES_H
#define CONFIG_STRUCTURES_H

#include <string>
#include <vector>
#include <map>

// Configuração do problema
struct ProblemConfig {
    std::string name;
    std::string type;
    std::string objective_function;
    std::string algorithm;
    int time_limit;
    std::string description;
};

// Configuração da função objetivo
struct ObjectiveConfig {
    std::string name;
    std::string description;
    std::string formula;
    std::string implementation;
    std::vector<std::string> variables;
    std::map<std::string, std::string> variable_descriptions;
};

// Configuração de uma restrição
struct ConstraintConfig {
    std::string name;
    std::string description;
    std::string formula;
    std::string implementation;
};

// Configuração do algoritmo
struct AlgorithmConfig {
    std::string name;
    std::string type;
    double epsilon;
    int max_iterations;
};

// Configuração de formato de entrada
struct InputFormatConfig {
    std::string line_1;
    std::string line_2_to_o_plus_1;
    std::string line_o_plus_2_to_o_plus_a_plus_1;
    std::string last_line;
};

// Configuração de formato de saída
struct OutputFormatConfig {
    std::string line_1;
    std::string line_2_to_n_plus_1;
    std::string line_n_plus_2;
    std::string line_n_plus_3_to_n_plus_m_plus_2;
};

#endif // CONFIG_STRUCTURES_H