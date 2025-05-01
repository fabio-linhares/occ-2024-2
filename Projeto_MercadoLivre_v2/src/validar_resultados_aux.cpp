// filepath: /home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v2/src/tools/validar_resultados_aux.cpp
#include <iostream>
#include <string>
#include "validar_resultados.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " <diretorio_entrada> <diretorio_saida> <arquivo_log>\n";
        return 1;
    }
    
    std::string diretorioEntrada = argv[1];
    std::string diretorioSaida = argv[2];
    std::string arquivoLog = argv[3];
    
    validarResultados(diretorioEntrada, diretorioSaida, arquivoLog);
    
    return 0;
}
