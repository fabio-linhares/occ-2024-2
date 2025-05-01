#include "app/app_controller.h"
#include "core/warehouse.h"
#include "ui/menu.h"
#include "io/file_utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

void showUsage(const char* programName) {
    std::cout << "Uso: " << programName << " [opções]\n\n"
              << "Opções:\n"
              << "  -c, --config ARQUIVO    Arquivo de configuração (padrão: config.txt)\n"
              << "  -i, --input DIR         Diretório de entrada (padrão: data/input)\n"
              << "  -o, --output DIR        Diretório de saída (padrão: data/output)\n" 
              << "  -t, --time SEGUNDOS     Limite de tempo em segundos (padrão: 300)\n"
              << "  -h, --help              Mostra esta mensagem\n";
}

// Função para listar arquivos em um diretório
std::vector<std::string> listarArquivos(const std::string& path) {
    std::vector<std::string> arquivos;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                arquivos.push_back(entry.path().filename().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Erro ao acessar diretório: " << e.what() << std::endl;
    }
    return arquivos;
}

// Função para testar o parser e exibir dados da instância
// Função para escrever o conteúdo em um arquivo de saída
void writeOutputFile(const std::string& outputPath, const std::string& filename, const std::string& content) {
    std::string fullOutputPath = outputPath + "/" + filename;
    std::ofstream outputFile(fullOutputPath);

    if (!outputFile.is_open()) {
        std::cerr << "Erro ao criar o arquivo de saída: " << fullOutputPath << std::endl;
        return;
    }

    outputFile << content;
    outputFile.close();

    if (content.empty()) {
        std::cout << "\nArquivo de saída vazio gerado: " << fullOutputPath << std::endl;
    } else {
         std::cout << "\nArquivo de saída gerado: " << fullOutputPath << std::endl;
    }
}


// Função para testar o parser e exibir dados da instância
void testParser(const std::string& inputPath, const std::string& outputPath, const std::string& nomeArquivo) {
    std::string caminhoCompleto = inputPath + "/" + nomeArquivo;
    std::ifstream arquivo(caminhoCompleto);

    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << caminhoCompleto << std::endl;
        return;
    }

    // Leitura dos dados
    int numPedidos, numItens, numCorredores;
    arquivo >> numPedidos >> numItens >> numCorredores;

    std::cout << "\n====== DADOS DA INSTÂNCIA ======" << std::endl;
    std::cout << "Primeira linha: " << numPedidos << " " << numItens << " " << numCorredores << std::endl;
    std::cout << "- Número de pedidos (o): " << numPedidos << std::endl;
    std::cout << "- Número de itens (i): " << numItens << std::endl;
    std::cout << "- Número de corredores (a): " << numCorredores << std::endl;

    // Leitura dos pedidos
    std::cout << "\n----- PEDIDOS -----" << std::endl;
    std::vector<std::vector<std::pair<int, int>>> pedidos;
    std::string line;
    // Consumir o resto da primeira linha
    std::getline(arquivo, line);

    for (int i = 0; i < numPedidos; i++) {
        std::getline(arquivo, line);
        std::stringstream ss(line);
        int k;
        ss >> k; // Lê o número de itens no pedido
        std::cout << "Pedido " << i << ": " << k << " itens - ";

        std::vector<std::pair<int, int>> itensNoPedido;
        for (int j = 0; j < k; j++) {
            int item, quantidade;
            ss >> item >> quantidade;
            itensNoPedido.push_back({item, quantidade});
            std::cout << "[Item " << item << ": " << quantidade << " unidades] ";
        }
        pedidos.push_back(itensNoPedido);
        std::cout << std::endl;
    }

    // Leitura dos corredores
    std::cout << "\n----- CORREDORES -----" << std::endl;
    std::vector<std::vector<std::pair<int, int>>> corredores;

    for (int i = 0; i < numCorredores; i++) {
         std::getline(arquivo, line);
         std::stringstream ss(line);
        int l;
        ss >> l; // Lê o número de itens no corredor
        std::cout << "Corredor " << i << ": " << l << " itens - ";

        std::vector<std::pair<int, int>> itensNoCorredor;
        for (int j = 0; j < l; j++) {
            int item, quantidade;
            ss >> item >> quantidade;
            itensNoCorredor.push_back({item, quantidade});
            std::cout << "[Item " << item << ": " << quantidade << " unidades] ";
        }
        corredores.push_back(itensNoCorredor);
        std::cout << std::endl;
    }

    // Leitura dos limites
    int LB, UB;
    arquivo >> LB >> UB;
    std::cout << "\n----- LIMITES -----" << std::endl;
    std::cout << "Limite inferior (LB): " << LB << std::endl;
    std::cout << "Limite superior (UB): " << UB << std::endl;

    arquivo.close();

    // Gerar arquivo de saída vazio por enquanto
    std::string nomeArquivoSaida = nomeArquivo + ".out";
    std::string conteudoSaida = ""; // Conteúdo vazio por enquanto

    writeOutputFile(outputPath, nomeArquivoSaida, conteudoSaida);
}

#include "io/file_utils.h"
#include "ui/menu.h"
#include <iostream>

int main() {
    std::string inputPath, outputPath;
    
    // Inicializar caminhos com valores padrão
    if (!io::initializePaths(inputPath, outputPath)) {
        return 1;
    }
    
    // Iniciar menu principal
    ui::MainMenu menu(inputPath, outputPath);
    menu.show();
    
    return 0;
}