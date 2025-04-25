#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <string>
#include "core/warehouse.h"

class InputParser {
public:
    Warehouse parseFile(const std::string& filePath);
};

#endif