#include "parser/instance_parser.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Uso: parser_test <arquivo_instancia>" << std::endl;
        return 1;
    }
    
    std::string inputPath = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/input";
    std::string outputPath = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/output";
    std::string nomeArquivo = argv[1];
    
    parser::testParser(inputPath, outputPath, nomeArquivo);
    
    return 0;
}