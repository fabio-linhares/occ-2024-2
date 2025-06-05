#include "desafio_info.h"
#include <iostream>
#include <fstream>
#include <string>

void exibirInformacoesDesafio() {
    std::ifstream arquivo("desafio_info.txt");
    std::string linha;

    if (arquivo.is_open()) {
        std::cout << "\n=== Informações do Desafio SBPO 2025 ===\n";
        while (std::getline(arquivo, linha)) {
            std::cout << linha << std::endl;
        }
        std::cout << "=========================================\n";
        arquivo.close();
    } else {
        std::cerr << "Erro ao abrir o arquivo desafio_info.txt" << std::endl;
    }
}