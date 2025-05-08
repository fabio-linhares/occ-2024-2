#include "parser/instance_parser.h"
#include "io/file_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace parser {

InstanceData parseInstanceFile(const std::string& filepath) {
    InstanceData data;
    std::ifstream arquivo(filepath);
    
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filepath << std::endl;
        return data;
    }
    
    // Leitura dos dados básicos
    arquivo >> data.numPedidos >> data.numItens >> data.numCorredores;
    
    // Consumir o resto da primeira linha
    std::string line;
    std::getline(arquivo, line);
    
    // Leitura dos pedidos
    for (int i = 0; i < data.numPedidos; i++) {
        std::getline(arquivo, line);
        std::stringstream ss(line);
        int k;
        ss >> k; // Número de itens no pedido
        
        std::vector<std::pair<int, int>> itensNoPedido;
        for (int j = 0; j < k; j++) {
            int item, quantidade;
            ss >> item >> quantidade;
            itensNoPedido.push_back({item, quantidade});
        }
        data.pedidos.push_back(itensNoPedido);
    }
    
    // Leitura dos corredores
    for (int i = 0; i < data.numCorredores; i++) {
        std::getline(arquivo, line);
        std::stringstream ss(line);
        int l;
        ss >> l; // Número de itens no corredor
        
        std::vector<std::pair<int, int>> itensNoCorredor;
        for (int j = 0; j < l; j++) {
            int item, quantidade;
            ss >> item >> quantidade;
            itensNoCorredor.push_back({item, quantidade});
        }
        data.corredores.push_back(itensNoCorredor);
    }
    
    // Leitura dos limites
    arquivo >> data.limiteLB >> data.limiteUB;
    
    arquivo.close();
    return data;
}

void testParser(const std::string& inputPath, const std::string& outputPath, const std::string& nomeArquivo) {
    std::string caminhoCompleto = inputPath + "/" + nomeArquivo;
    InstanceData data = parseInstanceFile(caminhoCompleto);
    
    // Exibir dados da instância
    std::cout << "\n====== DADOS DA INSTÂNCIA ======" << std::endl;
    std::cout << "Primeira linha: " << data.numPedidos << " " << data.numItens << " " << data.numCorredores << std::endl;
    std::cout << "- Número de pedidos (o): " << data.numPedidos << std::endl;
    std::cout << "- Número de itens (i): " << data.numItens << std::endl;
    std::cout << "- Número de corredores (a): " << data.numCorredores << std::endl;
    
    // Exibir pedidos
    std::cout << "\n----- PEDIDOS -----" << std::endl;
    for (int i = 0; i < data.numPedidos; i++) {
        std::cout << "Pedido " << i << ": " << data.pedidos[i].size() << " itens - ";
        for (const auto& item : data.pedidos[i]) {
            std::cout << "[Item " << item.first << ": " << item.second << " unidades] ";
        }
        std::cout << std::endl;
    }
    
    // Exibir corredores
    std::cout << "\n----- CORREDORES -----" << std::endl;
    for (int i = 0; i < data.numCorredores; i++) {
        std::cout << "Corredor " << i << ": " << data.corredores[i].size() << " itens - ";
        for (const auto& item : data.corredores[i]) {
            std::cout << "[Item " << item.first << ": " << item.second << " unidades] ";
        }
        std::cout << std::endl;
    }
    
    // Exibir limites
    std::cout << "\n----- LIMITES -----" << std::endl;
    std::cout << "Limite inferior (LB): " << data.limiteLB << std::endl;
    std::cout << "Limite superior (UB): " << data.limiteUB << std::endl;
    
    // Gerar arquivo de saída vazio por enquanto
    std::string nomeArquivoSaida = nomeArquivo + ".out";
    std::string conteudoSaida = ""; // Conteúdo vazio por enquanto
    
    io::writeOutputFile(outputPath, nomeArquivoSaida, conteudoSaida);
}

} // namespace parser