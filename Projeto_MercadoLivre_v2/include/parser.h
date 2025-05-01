#pragma once

#include <string>
#include <utility>
#include "armazem.h"

/**
 * @brief Função auxiliar para validar a consistência da instância carregada
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @return true se a instância for válida, false caso contrário
 */
bool validarInstancia(const Deposito& deposito, const Backlog& backlog);

/**
 * @brief Classe para leitura e processamento de arquivos de entrada
 */
class InputParser {
public:
    /**
     * @brief Analisa um arquivo de entrada e retorna as estruturas de dados correspondentes
     * @param filePath Caminho para o arquivo de entrada
     * @return Um par contendo o depósito e o backlog
     */
    std::pair<Deposito, Backlog> parseFile(const std::string& filePath);
};