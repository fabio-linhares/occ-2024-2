#pragma once

#include <string>
#include <vector>
#include <utility>

namespace parser {

/**
 * @brief Estrutura para armazenar os dados de uma instância
 */
struct InstanceData {
    int numPedidos;
    int numItens;
    int numCorredores;
    std::vector<std::vector<std::pair<int, int>>> pedidos;
    std::vector<std::vector<std::pair<int, int>>> corredores;
    int limiteLB;
    int limiteUB;
};

/**
 * @brief Processa e exibe os dados de uma instância do problema
 * @param inputPath Caminho do diretório de entrada
 * @param outputPath Caminho do diretório de saída
 * @param nomeArquivo Nome do arquivo a ser processado
 */
void testParser(const std::string& inputPath, const std::string& outputPath, const std::string& nomeArquivo);

/**
 * @brief Lê os dados de uma instância de um arquivo
 * @param filepath Caminho completo do arquivo
 * @return Estrutura preenchida com os dados da instância
 */
InstanceData parseInstanceFile(const std::string& filepath);

} // namespace parser