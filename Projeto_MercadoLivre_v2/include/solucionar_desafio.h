#pragma once

#include <string>
#include <vector>
#include "armazem.h"

/**
 * @brief Resolve o desafio para todas as instâncias no diretório de entrada
 * @param diretorioEntrada Caminho para o diretório com os arquivos de instância
 * @param diretorioSaida Caminho para o diretório onde os resultados serão salvos
 */
void solucionarDesafio(const std::string& diretorioEntrada, const std::string& diretorioSaida);

/**
 * @brief Estrutura para representar uma solução para uma instância
 */
struct Solucao {
    std::vector<int> pedidosWave;   // IDs dos pedidos na wave
    std::vector<int> corredoresWave; // IDs dos corredores usados na wave
    double valorObjetivo;          // Valor da função objetivo
};

/**
 * @brief Implementa o algoritmo guloso para gerar uma solução inicial
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @return Solucao Solução inicial gerada
 */
Solucao gerarSolucaoInicial(const Deposito& deposito, const Backlog& backlog);

/**
 * @brief Implementa o algoritmo de Dinkelbach modificado para perturbar a solução
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucaoAtual Solução atual a ser perturbada
 * @return Solucao Nova solução perturbada
 */
Solucao perturbarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoAtual);

/**
 * @brief Implementa o algoritmo de Dinkelbach para otimização
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucaoInicial Solução inicial para o algoritmo de Dinkelbach
 * @return Solucao Melhor solução encontrada pelo algoritmo de Dinkelbach
 */
Solucao otimizarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoInicial);

/**
 * @brief Calcula o valor da função objetivo para uma dada solução
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucao Solução para a qual calcular o valor objetivo
 * @return double Valor da função objetivo
 */
double calcularValorObjetivo(const Deposito& deposito, const Backlog& backlog, const Solucao& solucao);

/**
 * @brief Salva a solução em um arquivo de saída
 * @param diretorioSaida Caminho para o diretório de saída
 * @param nomeArquivo Nome do arquivo de saída
 * @param solucao Solução a ser salva
 */
void salvarSolucao(const std::string& diretorioSaida, const std::string& nomeArquivo, const Solucao& solucao);

/**
 * @brief Ajusta a solução removendo pedidos com estoque insuficiente e garantindo que o limite inferior seja atendido
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucao Solução a ser ajustada
 * @return Solucao Solução ajustada
 */
Solucao ajustarSolucao(const Deposito& deposito, const Backlog& backlog, Solucao solucao);