#include "parser.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <unordered_set>

bool validarInstancia(const Deposito& deposito, const Backlog& backlog) {
    std::cout << "Validando instância carregada..." << std::endl;
    
    // 1. Validar limites básicos
    if (backlog.numPedidos <= 0 || deposito.numItens <= 0 || deposito.numCorredores <= 0) {
        std::cerr << "ERRO: Valores inválidos para numPedidos, numItens ou numCorredores" << std::endl;
        return false;
    }
    
    // 2. Validar pedidos
    for (int p = 0; p < backlog.numPedidos; p++) {
        for (const auto& [itemId, quantidade] : backlog.pedido[p]) {
            if (itemId < 0 || itemId >= deposito.numItens) {
                std::cerr << "ERRO: Pedido " << p << " contém item inválido: " << itemId << std::endl;
                return false;
            }
            if (quantidade <= 0) {
                std::cerr << "AVISO: Pedido " << p << " contém item " << itemId 
                         << " com quantidade inválida: " << quantidade << std::endl;
                // Não falhar por isso, apenas avisar
            }
        }
    }
    
    // 3. Validar corredores
    for (int c = 0; c < deposito.numCorredores; c++) {
        for (const auto& [itemId, quantidade] : deposito.corredor[c]) {
            if (itemId < 0 || itemId >= deposito.numItens) {
                std::cerr << "ERRO: Corredor " << c << " contém item inválido: " << itemId << std::endl;
                return false;
            }
            if (quantidade <= 0) {
                std::cerr << "AVISO: Corredor " << c << " contém item " << itemId 
                         << " com quantidade inválida: " << quantidade << std::endl;
                // Não falhar por isso, apenas avisar
            }
        }
    }
    
    // 4. Validar LB/UB
    if (backlog.wave.LB < 0) {
        std::cerr << "ERRO: LB inválido: " << backlog.wave.LB << std::endl;
        return false;
    }
    if (backlog.wave.UB < backlog.wave.LB) {
        std::cerr << "ERRO: UB (" << backlog.wave.UB << ") menor que LB (" << backlog.wave.LB << ")" << std::endl;
        return false;
    }
    
    std::cout << "Instância validada com sucesso!" << std::endl;
    return true;
}

std::pair<Deposito, Backlog> InputParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Não foi possível abrir o arquivo: " + filePath);
    }
    
    Deposito deposito;
    Backlog backlog;
    std::string line;
    
    // Ler primeira linha com números de pedidos, itens e corredores
    if (!std::getline(file, line)) {
        throw std::runtime_error("Arquivo vazio ou corrompido");
    }
    
    std::istringstream headerLine(line);
    int numPedidos, numItens, numCorredores;
    
    // Ler explicitamente em variáveis locais primeiro
    if (!(headerLine >> numPedidos >> numItens >> numCorredores)) {
        throw std::runtime_error("Primeira linha inválida: deve conter 3 números inteiros");
    }
    
    // Verificar valores
    if (numPedidos <= 0 || numItens <= 0 || numCorredores <= 0) {
        throw std::runtime_error("Valores inválidos para numPedidos, numItens ou numCorredores");
    }
    
    // Definir as propriedades após validação
    backlog.numPedidos = numPedidos;
    deposito.numItens = numItens;
    deposito.numCorredores = numCorredores;
    
    std::cout << "Lendo instância com " << backlog.numPedidos << " pedidos, " 
              << deposito.numItens << " itens e " << deposito.numCorredores 
              << " corredores" << std::endl;
    
    // Inicializar estruturas com tamanho fixo
    backlog.pedido.resize(backlog.numPedidos);
    deposito.corredor.resize(deposito.numCorredores);
    
    // Ler pedidos
    for (int i = 0; i < backlog.numPedidos; ++i) {
        if (!std::getline(file, line)) {
            throw std::runtime_error("Arquivo terminado inesperadamente ao ler pedidos");
        }
        
        std::istringstream orderLine(line);
        int numItemsInOrder;
        
        if (!(orderLine >> numItemsInOrder)) {
            throw std::runtime_error("Formato inválido ao ler número de itens no pedido " + std::to_string(i));
        }
        
        for (int j = 0; j < numItemsInOrder; j++) {
            int itemId, quantity;
            if (!(orderLine >> itemId >> quantity)) {
                throw std::runtime_error("Formato inválido ao ler item " + std::to_string(j) + 
                                         " do pedido " + std::to_string(i));
            }
            
            // Validação: ignorar item se itemId inválido
            if (itemId < 0 || itemId >= deposito.numItens) {
                std::cerr << "AVISO: Ignorando item com ID inválido " << itemId 
                         << " no pedido " << i << std::endl;
                continue;
            }
            
            if (quantity <= 0) {
                std::cerr << "AVISO: Quantidade inválida " << quantity 
                         << " para item " << itemId << " no pedido " << i << std::endl;
                continue;
            }
            
            backlog.pedido[i][itemId] = quantity;
        }
    }
    
    // Ler corredores
    for (int i = 0; i < deposito.numCorredores; ++i) {
        if (!std::getline(file, line)) {
            throw std::runtime_error("Arquivo terminado inesperadamente ao ler corredores");
        }
        
        std::istringstream corridorLine(line);
        int numItemsInCorridor;
        
        if (!(corridorLine >> numItemsInCorridor)) {
            throw std::runtime_error("Formato inválido ao ler número de itens no corredor " + std::to_string(i));
        }
        
        for (int j = 0; j < numItemsInCorridor; j++) {
            int itemId, quantity;
            if (!(corridorLine >> itemId >> quantity)) {
                throw std::runtime_error("Formato inválido ao ler item " + std::to_string(j) + 
                                         " do corredor " + std::to_string(i));
            }
            
            // Validação: ignorar item se itemId inválido
            if (itemId < 0 || itemId >= deposito.numItens) {
                std::cerr << "AVISO: Ignorando item com ID inválido " << itemId 
                         << " no corredor " << i << std::endl;
                continue;
            }
            
            if (quantity <= 0) {
                std::cerr << "AVISO: Quantidade inválida " << quantity 
                         << " para item " << itemId << " no corredor " << i << std::endl;
                continue;
            }
            
            deposito.corredor[i][itemId] = quantity;
        }
    }
    
    // Ler a última linha com LB e UB
    if (!std::getline(file, line)) {
        throw std::runtime_error("Arquivo terminado inesperadamente ao ler LB e UB");
    }
    
    std::istringstream lbubLine(line);
    
    // Garantir que lemos exatamente 2 valores e nada mais
    if (!(lbubLine >> backlog.wave.LB >> backlog.wave.UB)) {
        throw std::runtime_error("Última linha inválida: deve conter 2 números inteiros (LB e UB)");
    }
    
    // Verificar se há mais conteúdo na linha (o que seria inválido)
    std::string extraContent;
    if (lbubLine >> extraContent) {
        throw std::runtime_error("Última linha com formato inválido: contém dados extras");
    }
    
    // Validar limites
    if (backlog.wave.LB < 0 || backlog.wave.UB < backlog.wave.LB) {
        throw std::runtime_error("Valores inválidos para LB ou UB");
    }
    
    std::cout << "Limites da instância: LB=" << backlog.wave.LB << ", UB=" << backlog.wave.UB << std::endl;
    
    // Validar a instância carregada
    if (!validarInstancia(deposito, backlog)) {
        throw std::runtime_error("Instância inválida após parser: " + filePath);
    }
    
    return std::make_pair(deposito, backlog);
}