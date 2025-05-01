#pragma once

#include <string>

namespace ui {

/**
 * @brief Classe para gerenciar o menu de debug
 */
class DebugMenu {
public:
    /**
     * @brief Construtor
     * @param inputPath Caminho do diretório de entrada
     * @param outputPath Caminho do diretório de saída
     */
    DebugMenu(const std::string& inputPath, const std::string& outputPath);
    
    /**
     * @brief Exibe e gerencia o menu de debug
     * @return true se o menu deve continuar, false para sair
     */
    bool show();

private:
    std::string inputPath;
    std::string outputPath;
    
    /**
     * @brief Executa a opção de teste do parser
     */
    void testarParser();
};

/**
 * @brief Classe para gerenciar o menu principal
 */
class MainMenu {
public:
    /**
     * @brief Construtor
     * @param inputPath Caminho do diretório de entrada
     * @param outputPath Caminho do diretório de saída
     */
    MainMenu(const std::string& inputPath, const std::string& outputPath);
    
    /**
     * @brief Exibe e gerencia o menu principal
     */
    void show();

private:
    std::string inputPath;
    std::string outputPath;
    
    /**
     * @brief Exibe o menu de debug
     */
    void mostrarMenuDebug();
    
    /**
     * @brief Executa o algoritmo principal (não implementado)
     */
    void executar();
};

} // namespace ui