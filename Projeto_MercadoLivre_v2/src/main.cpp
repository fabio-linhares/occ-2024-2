#include <iostream>
#include <limits>
#include "menu.h"
#include "formatacao_terminal.h" // Adicione este include

// Usar FormatacaoTerminal
using namespace FormatacaoTerminal;

/**
 * @brief Função principal do programa
 * @return Código de saída (0 = sucesso)
 */
int main() {
    std::cout << "Projeto MercadoLivre v2 - SBPO 2025\n";
    std::cout << "Sistema de Otimização de Waves para Processamento de Pedidos\n\n";
    
    int choice = -1;
    do {
        mostrarMenu();
        std::cout << colorir("Digite sua escolha: ", VERDE);
        std::cin >> choice;
        
        // Tratar entrada inválida
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1; // Valor inválido para mostrar mensagem de erro
        }
        
        processarEscolhaMenu(choice);
    } while (choice != 0);
    
    return 0;
}
