#pragma once

#include "core/warehouse.h"
#include <string>
#include <stdexcept>

/**
 * @brief Classe responsável por fazer o parsing dos arquivos de entrada
 */
class InputParser {
public:
    /**
     * @brief Realiza o parsing de um arquivo de instância
     * @param filePath Caminho para o arquivo de entrada
     * @return Objeto Warehouse contendo os dados lidos
     * @throws std::runtime_error se ocorrer erro no parsing
     */
    Warehouse parseFile(const std::string& filePath);

private:
    /**
     * @brief Valida os dados lidos para garantir consistência
     * @param warehouse Referência ao objeto Warehouse a ser validado
     * @throws std::runtime_error se os dados forem inválidos
     */
    void validateWarehouse(const Warehouse& warehouse) const;
};