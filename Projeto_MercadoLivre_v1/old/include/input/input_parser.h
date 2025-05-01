#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <string>
#include "core/warehouse.h"

// Função auxiliar para validar a consistência da instância carregada
bool validarInstancia(const Warehouse& warehouse);

class InputParser {
public:
    Warehouse parseFile(const std::string& filePath);
};

#endif