// filepath: /home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v2/src/tools/verificar_estruturas_aux.cpp
#include <iostream>
#include <string>
#include "verificar_estruturas_auxiliares.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_instancia>\n";
        return 1;
    }
    
    std::string filePath = argv[1];
    verificarEstruturasAuxiliares(filePath);
    
    return 0;
}
