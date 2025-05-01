#pragma once

#include <string>

/**
 * @brief Gera um nome de arquivo com timestamp no formato DDMMAA-HHMM
 * @return String com o nome do arquivo gerado
 */
std::string gerarNomeArquivoComTimestamp();

/**
 * @brief Valida os arquivos de solução comparando-os com os arquivos de entrada
 * @param diretorioEntrada Caminho para o diretório com os arquivos de instância
 * @param diretorioSaida Caminho para o diretório com os arquivos de solução
 */
void validarResultados(const std::string& diretorioEntrada, const std::string& diretorioSaida);