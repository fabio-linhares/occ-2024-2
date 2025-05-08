#pragma once

/**
 * @brief Exibe o menu principal do programa
 */
void mostrarMenu();

/**
 * @brief Processa a escolha do usuário no menu
 * @param choice Opção escolhida pelo usuário
 */
void processarEscolhaMenu(int choice);

/**
 * @brief Realiza pré-processamento e otimização usando PLI-ALM
 * @param caminhoEntrada Caminho para o arquivo de entrada
 * @param diretorioSaida Diretório para salvar o resultado
 */
void preprocessamentoPLI(const std::string& caminhoEntrada, const std::string& diretorioSaida);