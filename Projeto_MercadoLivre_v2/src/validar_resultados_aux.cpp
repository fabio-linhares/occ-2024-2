// filepath: /home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v2/src/tools/validar_resultados_aux.cpp
#include <iostream>
#include <string>
#include "validar_resultados.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <diretorio_entrada> <diretorio_saida>\n";
        return 1;
    }
    
    std::string diretorioEntrada = argv[1];
    std::string diretorioSaida = argv[2];
    
    // Removido o terceiro argumento (arquivoLog) que não é mais necessário
    validarResultados(diretorioEntrada, diretorioSaida);
    
    return 0;
}
