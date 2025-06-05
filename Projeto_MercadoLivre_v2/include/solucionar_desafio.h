#pragma once

#include <string>
#include <vector>
#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "analisador_relevancia.h"

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
 * @param localizador Estrutura auxiliar para localização de itens nos corredores
 * @param verificador Estrutura auxiliar para verificação de disponibilidade
 * @param analisador Estrutura auxiliar para análise de relevância dos pedidos
 * @return Solucao Solução inicial gerada
 */
Solucao gerarSolucaoInicial(const Deposito& deposito, const Backlog& backlog,
                           const LocalizadorItens& localizador,
                           const VerificadorDisponibilidade& verificador,
                           const AnalisadorRelevancia& analisador);

/**
 * @brief Implementa o algoritmo de Dinkelbach modificado para perturbar a solução
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucaoAtual Solução atual a ser perturbada
 * @param localizador Estrutura auxiliar para localização de itens nos corredores
 * @param verificador Estrutura auxiliar para verificação de disponibilidade
 * @param analisador Estrutura auxiliar para análise de relevância dos pedidos
 * @return Solucao Nova solução perturbada
 */
Solucao perturbarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoAtual,
                         const LocalizadorItens& localizador,
                         const VerificadorDisponibilidade& verificador,
                         const AnalisadorRelevancia& analisador);

/**
 * @brief Implementa o algoritmo de Dinkelbach para otimização
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucaoInicial Solução inicial para o algoritmo de Dinkelbach
 * @param localizador Estrutura auxiliar para localização de itens nos corredores
 * @param verificador Estrutura auxiliar para verificação de disponibilidade
 * @param analisador Estrutura auxiliar para análise de relevância dos pedidos
 * @return Solucao Melhor solução encontrada pelo algoritmo de Dinkelbach
 */
Solucao otimizarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoInicial,
                       const LocalizadorItens& localizador,
                       const VerificadorDisponibilidade& verificador,
                       const AnalisadorRelevancia& analisador);

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
 * @param localizador Estrutura auxiliar para localização de itens nos corredores
 * @param verificador Estrutura auxiliar para verificação de disponibilidade
 * @return Solucao Solução ajustada
 */
Solucao ajustarSolucao(const Deposito& deposito, const Backlog& backlog, Solucao solucao,
                      const LocalizadorItens& localizador,
                      const VerificadorDisponibilidade& verificador);