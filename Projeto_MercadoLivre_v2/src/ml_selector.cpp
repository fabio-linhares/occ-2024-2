#include "ml_selector.h"
#include "parser.h"
#include <numeric>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <climits>

MLSelector::MLSelector() : modeloTreinado(false) {
    // Inicializar pesos com valores padrão
    pesos["numPedidos"] = 0.15;
    pesos["numItens"] = 0.1;
    pesos["numCorredores"] = 0.1;
    pesos["densidadeItensCorredores"] = 0.2;
    pesos["razaoLimites"] = 0.25;
    pesos["mediaItensPorPedido"] = 0.2;
}

InstanciaFeatures MLSelector::extrairFeatures(const Deposito& deposito, const Backlog& backlog) {
    InstanciaFeatures features;
    
    // Características básicas
    features.numPedidos = backlog.numPedidos;
    features.numItens = deposito.numItens;
    features.numCorredores = deposito.numCorredores;
    features.limiteLB = backlog.wave.LB;
    features.limiteUB = backlog.wave.UB;
    features.razaoLimites = (features.limiteUB > 0) ? 
                          (double)features.limiteLB / features.limiteUB : 0.0;
    
    // Características derivadas
    int totalItensPorPedido = 0;
    features.maxItensPorPedido = 0;
    features.minItensPorPedido = INT_MAX;
    
    for (int p = 0; p < backlog.numPedidos; p++) {
        int itensPedido = backlog.pedido[p].size();
        totalItensPorPedido += itensPedido;
        features.maxItensPorPedido = std::max(features.maxItensPorPedido, itensPedido);
        features.minItensPorPedido = std::min(features.minItensPorPedido, itensPedido);
    }
    
    features.mediaItensPorPedido = (backlog.numPedidos > 0) ? 
                                 (double)totalItensPorPedido / backlog.numPedidos : 0;
    
    // Calcular densidade (itens por corredor)
    int totalItensCorredores = 0;
    for (int c = 0; c < deposito.numCorredores; c++) {
        totalItensCorredores += deposito.corredor[c].size();
    }
    
    features.densidadeItensCorredores = (deposito.numCorredores > 0) ? 
                                      (double)totalItensCorredores / deposito.numCorredores : 0;
    
    return features;
}

void MLSelector::adicionarExemplo(const InstanciaFeatures& features, const std::string& algoritmoOtimo) {
    dadosTreinamento.push_back(features);
    algoritmosOtimos.push_back(algoritmoOtimo);
    modeloTreinado = false; // Precisa treinar novamente
}

void MLSelector::treinarModelo() {
    // Implementação simples baseada em peso de características
    // Um modelo mais sofisticado seria usado em produção (ex: Random Forest, SVM)
    
    // Nada a fazer se não há dados de treinamento
    if (dadosTreinamento.empty()) {
        std::cerr << "Sem dados para treinar o modelo!" << std::endl;
        return;
    }
    
    // Em uma implementação real, calcularíamos médias e desvios padrão
    // e aplicaríamos normalização adequada aos dados
    
    // Simplesmente marcamos o modelo como treinado
    modeloTreinado = true;
    std::cout << "Modelo treinado com " << dadosTreinamento.size() << " exemplos." << std::endl;
}

std::string MLSelector::selecionarAlgoritmo(const InstanciaFeatures& features) {
    if (!modeloTreinado) {
        // Se o modelo não foi treinado, usar heurística simples
        if (features.numPedidos > 500 || features.numItens > 1000) {
            return "HeuristicaGulosa";
        } else if (features.densidadeItensCorredores > 10.0) {
            return "BuscaTabu";
        } else {
            return "Dinkelbach+BnB";
        }
    }
    
    // Calcular similaridade com exemplos no conjunto de treinamento
    // e retornar o algoritmo do exemplo mais similar
    double melhorSimilaridade = -1.0;
    int melhorIndice = 0;
    
    for (size_t i = 0; i < dadosTreinamento.size(); i++) {
        double similaridade = calcularSimilaridade(features, dadosTreinamento[i]);
        if (similaridade > melhorSimilaridade) {
            melhorSimilaridade = similaridade;
            melhorIndice = i;
        }
    }
    
    return algoritmosOtimos[melhorIndice];
}

// Método privado para calcular similaridade entre instâncias
double MLSelector::calcularSimilaridade(const InstanciaFeatures& a, const InstanciaFeatures& b) {
    // Distância euclidiana ponderada por importância de features
    double distancia = 0.0;
    
    distancia += pesos["numPedidos"] * pow((a.numPedidos - b.numPedidos) / 500.0, 2);
    distancia += pesos["numItens"] * pow((a.numItens - b.numItens) / 1000.0, 2);
    distancia += pesos["numCorredores"] * pow((a.numCorredores - b.numCorredores) / 100.0, 2);
    distancia += pesos["densidadeItensCorredores"] * 
                pow((a.densidadeItensCorredores - b.densidadeItensCorredores) / 10.0, 2);
    distancia += pesos["razaoLimites"] * pow(a.razaoLimites - b.razaoLimites, 2);
    distancia += pesos["mediaItensPorPedido"] * 
                pow((a.mediaItensPorPedido - b.mediaItensPorPedido) / 5.0, 2);
    
    return 1.0 / (1.0 + sqrt(distancia));
}

void MLSelector::salvarModelo(const std::string& caminhoArquivo) {
    std::ofstream outFile(caminhoArquivo);
    
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo para salvar modelo: " << caminhoArquivo << std::endl;
        return;
    }
    
    // Salvar pesos
    outFile << "# Pesos para cada caracteristica\n";
    for (const auto& [caracteristica, peso] : pesos) {
        outFile << caracteristica << "," << peso << "\n";
    }
    
    // Salvar exemplos de treinamento
    outFile << "\n# Exemplos de treinamento\n";
    outFile << "numPedidos,numItens,numCorredores,mediaItensPorPedido,maxItensPorPedido,";
    outFile << "minItensPorPedido,densidadeItensCorredores,limiteLB,limiteUB,razaoLimites,algoritmoOtimo\n";
    
    for (size_t i = 0; i < dadosTreinamento.size(); i++) {
        const auto& features = dadosTreinamento[i];
        outFile << features.numPedidos << ","
                << features.numItens << ","
                << features.numCorredores << ","
                << features.mediaItensPorPedido << ","
                << features.maxItensPorPedido << ","
                << features.minItensPorPedido << ","
                << features.densidadeItensCorredores << ","
                << features.limiteLB << ","
                << features.limiteUB << ","
                << features.razaoLimites << ","
                << algoritmosOtimos[i] << "\n";
    }
    
    outFile.close();
    std::cout << "Modelo salvo com sucesso em: " << caminhoArquivo << std::endl;
}

void MLSelector::carregarModelo(const std::string& caminhoArquivo) {
    std::ifstream inFile(caminhoArquivo);
    
    if (!inFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo para carregar modelo: " << caminhoArquivo << std::endl;
        return;
    }
    
    pesos.clear();
    dadosTreinamento.clear();
    algoritmosOtimos.clear();
    
    std::string linha;
    
    // Ler pesos (ignorar linhas de comentário)
    while (std::getline(inFile, linha)) {
        if (linha.empty() || linha[0] == '#') continue;
        
        std::istringstream iss(linha);
        std::string caracteristica;
        double peso;
        
        std::getline(iss, caracteristica, ',');
        iss >> peso;
        
        pesos[caracteristica] = peso;
        
        // Se encontrarmos uma linha em branco, passamos para a próxima seção
        if (linha.empty()) break;
    }
    
    // Pular até a linha com os cabeçalhos dos exemplos
    while (std::getline(inFile, linha)) {
        if (linha[0] != '#' && !linha.empty() && linha.find(',') != std::string::npos) {
            break; // Encontramos a linha de cabeçalho
        }
    }
    
    // Ler exemplos
    while (std::getline(inFile, linha)) {
        if (linha.empty()) continue;
        
        std::istringstream iss(linha);
        InstanciaFeatures features;
        std::string algoritmoOtimo;
        
        iss >> features.numPedidos;
        iss.ignore(); // pular vírgula
        iss >> features.numItens;
        iss.ignore();
        iss >> features.numCorredores;
        iss.ignore();
        iss >> features.mediaItensPorPedido;
        iss.ignore();
        iss >> features.maxItensPorPedido;
        iss.ignore();
        iss >> features.minItensPorPedido;
        iss.ignore();
        iss >> features.densidadeItensCorredores;
        iss.ignore();
        iss >> features.limiteLB;
        iss.ignore();
        iss >> features.limiteUB;
        iss.ignore();
        iss >> features.razaoLimites;
        iss.ignore();
        std::getline(iss, algoritmoOtimo);
        
        dadosTreinamento.push_back(features);
        algoritmosOtimos.push_back(algoritmoOtimo);
    }
    
    modeloTreinado = !dadosTreinamento.empty();
    
    inFile.close();
    std::cout << "Modelo carregado com sucesso de: " << caminhoArquivo << std::endl;
    std::cout << "Carregados " << dadosTreinamento.size() << " exemplos de treinamento." << std::endl;
}