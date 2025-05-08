#include "ui/menu.h"
#include "io/file_utils.h"
#include "parser/instance_parser.h"
#include <iostream>

namespace ui {

// Implementação do DebugMenu
DebugMenu::DebugMenu(const std::string& inputPath, const std::string& outputPath)
    : inputPath(inputPath), outputPath(outputPath) {}

bool DebugMenu::show() {
    int opcao = 0;
    std::cout << "\n===== MENU DE DEBUG =====" << std::endl;
    std::cout << "1. Test do parser" << std::endl;
    std::cout << "2. Outras opções" << std::endl;
    std::cout << "3. Voltar" << std::endl;
    std::cout << "Escolha uma opção: ";
    std::cin >> opcao;
    
    switch (opcao) {
        case 1:
            testarParser();
            return true;
        case 2:
            std::cout << "Outras opções de debug não implementadas ainda." << std::endl;
            return true;
        case 3:
            std::cout << "Voltando ao menu principal..." << std::endl;
            return false;
        default:
            std::cout << "Opção inválida!" << std::endl;
            return true;
    }
}

void DebugMenu::testarParser() {
    // Listar arquivos do diretório de entrada
    std::vector<std::string> arquivos = io::listarArquivos(inputPath);
    
    if (arquivos.empty()) {
        std::cout << "Nenhum arquivo encontrado no diretório de entrada." << std::endl;
    } else {
        std::cout << "\nArquivos disponíveis:" << std::endl;
        for (size_t i = 0; i < arquivos.size(); i++) {
            std::cout << i+1 << ". " << arquivos[i] << std::endl;
        }
        
        int escolha;
        std::cout << "Selecione um arquivo (1-" << arquivos.size() << "): ";
        std::cin >> escolha;
        
        if (escolha >= 1 && escolha <= static_cast<int>(arquivos.size())) {
            parser::testParser(inputPath, outputPath, arquivos[escolha-1]);
        } else {
            std::cout << "Opção inválida!" << std::endl;
        }
    }
}

// Implementação do MainMenu
MainMenu::MainMenu(const std::string& inputPath, const std::string& outputPath)
    : inputPath(inputPath), outputPath(outputPath) {}

void MainMenu::show() {
    int opcao = 0;
    while (opcao != 3) {
        std::cout << "\n===== MENU PRINCIPAL =====" << std::endl;
        std::cout << "1. Executar" << std::endl;
        std::cout << "2. Debug" << std::endl;
        std::cout << "3. Sair" << std::endl;
        std::cout << "Escolha uma opção: ";
        std::cin >> opcao;
        
        switch (opcao) {
            case 1:
                executar();
                break;
            case 2:
                mostrarMenuDebug();
                break;
            case 3:
                std::cout << "Saindo do programa..." << std::endl;
                break;
            default:
                std::cout << "Opção inválida!" << std::endl;
        }
    }
}

void MainMenu::mostrarMenuDebug() {
    DebugMenu debugMenu(inputPath, outputPath);
    while (debugMenu.show()) {
        // Continua mostrando o menu de debug até o usuário sair
    }
}

void MainMenu::executar() {
    std::cout << "Execução não implementada ainda." << std::endl;
}

} // namespace ui