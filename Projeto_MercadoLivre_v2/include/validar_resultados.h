#pragma once

#include <string>

/**
 * @brief Valida os arquivos de solução comparando-os com os arquivos de entrada
 * @param diretorioEntrada Caminho para o diretório com os arquivos de instância
 * @param diretorioSaida Caminho para o diretório com os arquivos de solução
 * @param arquivoLog Caminho para o arquivo de log de validação
 */
void validarResultados(const std::string& diretorioEntrada, const std::string& diretorioSaida, const std::string& arquivoLog);