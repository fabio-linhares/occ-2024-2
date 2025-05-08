#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace io {

/**
 * @brief Lista todos os arquivos regulares em um diretório
 * @param path Caminho do diretório
 * @return Lista de nomes de arquivos
 */
std::vector<std::string> listarArquivos(const std::string& path);

/**
 * @brief Escreve conteúdo em um arquivo de saída
 * @param outputPath Caminho do diretório de saída
 * @param filename Nome do arquivo de saída
 * @param content Conteúdo a ser escrito
 */
void writeOutputFile(const std::string& outputPath, const std::string& filename, const std::string& content);

/**
 * @brief Inicializa os caminhos de entrada e saída
 * @param inputPath [out] Caminho de entrada selecionado
 * @param outputPath [out] Caminho de saída selecionado
 * @return true se os caminhos foram inicializados com sucesso
 */
bool initializePaths(std::string& inputPath, std::string& outputPath);

} // namespace io