#include <iostream>
#include <vector>
#include <filesystem>
#include "menu.h"
#include "verificar_instancias.h"
#include "verificar_estruturas_auxiliares.h"
#include "solucionar_desafio.h"
#include "validar_resultados.h"
#include "desafio_info.h"

void mostrarMenu() {
    std::cout << "\n===== Menu Principal =====\n";
    std::cout << "1. Verificar as instâncias\n";
    std::cout << "2. Verificar estruturas auxiliares\n";
    std::cout << "3. Solucionar o desafio\n";
    std::cout << "4. Validar resultados\n";
    std::cout << "5. Exibir informações do desafio\n";
    std::cout << "0. Sair\n";
    std::cout << "=========================\n";
}

// Função auxiliar para selecionar um arquivo de instância
std::string selecionarArquivoInstancia() {
    // Diretório onde estão os arquivos de instância
    const std::string diretorioInstancias = "data/input"; // Ajuste conforme necessário
    
    // Listar arquivos no diretório
    std::vector<std::string> arquivos;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(diretorioInstancias)) {
            if (entry.is_regular_file()) {
                arquivos.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cout << "Erro ao acessar o diretório: " << e.what() << std::endl;
        return "";
    }
    
    if (arquivos.empty()) {
        std::cout << "Nenhum arquivo encontrado no diretório de entrada.\n";
        return "";
    }
    
    // Mostrar lista de arquivos
    std::cout << "Arquivos disponíveis:\n";
    for (size_t i = 0; i < arquivos.size(); i++) {
        std::cout << i + 1 << ". " << arquivos[i] << "\n";
    }
    
    // Obter escolha do usuário
    int escolhaArquivo;
    std::cout << "Selecione um arquivo (1-" << arquivos.size() << "): ";
    std::cin >> escolhaArquivo;
    
    if (escolhaArquivo < 1 || escolhaArquivo > static_cast<int>(arquivos.size())) {
        std::cout << "Seleção inválida.\n";
        return "";
    }
    
    // Formar o caminho completo do arquivo selecionado
    return diretorioInstancias + "/" + arquivos[escolhaArquivo - 1];
}

void processarEscolhaMenu(int choice) {
    switch (choice) {
        case 0:
            std::cout << "Saindo do programa. Até logo!\n";
            break;
        case 1:
            {
                std::string arquivoSelecionado = selecionarArquivoInstancia();
                if (!arquivoSelecionado.empty()) {
                    verificarInstancias(arquivoSelecionado);
                }
            }
            break;
        case 2:
            {
                std::string arquivoSelecionado = selecionarArquivoInstancia();
                if (!arquivoSelecionado.empty()) {
                    verificarEstruturasAuxiliares(arquivoSelecionado);
                }
            }
            break;
        case 3:
            {
                // Definir diretórios de entrada e saída
                const std::string diretorioEntrada = "data/input";
                const std::string diretorioSaida = "data/output";
                
                // Chamar a função para solucionar o desafio
                solucionarDesafio(diretorioEntrada, diretorioSaida);
            }
            break;
        case 4:
            {
                // Definir diretórios de entrada e saída
                const std::string diretorioEntrada = "data/input";
                const std::string diretorioSaida = "data/output";
                const std::string arquivoLog = "data/validation_log.txt";
                
                // Chamar a função para validar os resultados
                validarResultados(diretorioEntrada, diretorioSaida, arquivoLog);
            }
            break;
        case 5:
            exibirInformacoesDesafio();
            break;
        default:
            std::cout << "Opção inválida. Tente novamente.\n";
            break;
    }
}