#pragma once

#include <vector>
#include <string>
#include <map>
#include "armazem.h"

/**
 * @brief Estrutura para características de instâncias
 */
struct InstanciaFeatures {
    int numPedidos;
    int numItens;
    int numCorredores;
    int mediaItensPorPedido;
    int maxItensPorPedido;
    int minItensPorPedido;
    double densidadeItensCorredores;
    int limiteLB;
    int limiteUB;
    double razaoLimites;
    // Outras características relevantes
};

/**
 * @brief Classe para seleção de algoritmos usando aprendizado de máquina
 */
class MLSelector {
private:
    std::vector<InstanciaFeatures> dadosTreinamento;
    std::vector<std::string> algoritmosOtimos;
    std::map<std::string, double> pesos;
    bool modeloTreinado;
    
    /**
     * @brief Calcula a similaridade entre duas instâncias
     * @param a Primeira instância
     * @param b Segunda instância
     * @return Valor de similaridade entre 0 e 1
     */
    double calcularSimilaridade(const InstanciaFeatures& a, const InstanciaFeatures& b);
    
public:
    /**
     * @brief Construtor
     */
    MLSelector();
    
    /**
     * @brief Extrai características de uma instância
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @return Features extraídas da instância
     */
    InstanciaFeatures extrairFeatures(const Deposito& deposito, const Backlog& backlog);
    
    /**
     * @brief Adiciona uma instância ao conjunto de treinamento
     * @param features Características da instância
     * @param algoritmoOtimo Algoritmo com melhor desempenho para esta instância
     */
    void adicionarExemplo(const InstanciaFeatures& features, const std::string& algoritmoOtimo);
    
    /**
     * @brief Treina o modelo de seleção
     */
    void treinarModelo();
    
    /**
     * @brief Seleciona o melhor algoritmo para uma instância
     * @param features Características da instância
     * @return Nome do algoritmo recomendado
     */
    std::string selecionarAlgoritmo(const InstanciaFeatures& features);
    
    /**
     * @brief Salva o modelo treinado
     * @param caminhoArquivo Caminho para salvar o modelo
     */
    void salvarModelo(const std::string& caminhoArquivo);
    
    /**
     * @brief Carrega um modelo previamente treinado
     * @param caminhoArquivo Caminho do arquivo do modelo
     */
    void carregarModelo(const std::string& caminhoArquivo);
};