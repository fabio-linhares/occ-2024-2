#include <iostream>
#include "menu.h"

/**
 * @brief Função principal do programa
 * @return Código de saída (0 = sucesso)
 */
int main() {
    std::cout << "Projeto MercadoLivre v2 - SBPO 2025\n";
    std::cout << "Sistema de Otimização de Waves para Processamento de Pedidos\n\n";
    
    int choice = 0;
    do {
        mostrarMenu();
        std::cout << "Digite sua escolha: ";
        std::cin >> choice;
        processarEscolhaMenu(choice);
    } while (choice != 0);
    
    return 0;
}
