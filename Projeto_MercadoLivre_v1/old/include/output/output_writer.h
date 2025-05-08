#pragma once
#include <string>
#include "core/solution.h"

class OutputWriter {
public:
    OutputWriter() = default;
    
    // Escreve a solução em um arquivo
    bool writeSolution(const Solution& solution, const std::string& filePath) const;
};