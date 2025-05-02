#pragma once

#include <string>
#include <vector>
#include "armazem.h"
#include "solucionar_desafio.h"

/**
 * @brief Classe para visualização avançada de resultados
 */
class VisualizadorResultados {
public:
    /**
     * @brief Gera representação visual do depósito
     * @param deposito Dados do depósito
     * @param caminhoSaida Caminho para o arquivo de saída
     */
    static void visualizarDeposito(const Deposito& deposito, const std::string& caminhoSaida);
    
    /**
     * @brief Gera visualização da wave selecionada
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param pedidosWave IDs dos pedidos na wave
     * @param corredoresWave IDs dos corredores na wave
     * @param caminhoSaida Caminho para o arquivo de saída
     */
    static void visualizarWave(
        const Deposito& deposito,
        const Backlog& backlog,
        const std::vector<int>& pedidosWave,
        const std::vector<int>& corredoresWave,
        const std::string& caminhoSaida
    );
    
    /**
     * @brief Gera gráfico de calor mostrando relação entre pedidos e corredores
     * @param deposito Dados do depósito
     * @param backlog Dados do backlog
     * @param caminhoSaida Caminho para o arquivo de saída
     */
    static void gerarMapaCalor(
        const Deposito& deposito,
        const Backlog& backlog,
        const std::string& caminhoSaida
    );
    
    /**
     * @brief Gera gráfico comparativo entre diferentes soluções
     * @param resultados Lista de soluções a comparar
     * @param caminhoSaida Caminho para o arquivo de saída
     */
    static void gerarComparativoSolucoes(
        const std::vector<std::pair<std::string, Solucao>>& resultados,
        const std::string& caminhoSaida
    );
    
    /**
     * @brief Gera dashboard interativo com informações completas
     * @param diretorioEntrada Diretório com arquivos de instância
     * @param diretorioSaida Diretório com arquivos de solução
     * @param arquivoDashboard Caminho para o arquivo HTML do dashboard
     */
    static void gerarDashboardInterativo(
        const std::string& diretorioEntrada,
        const std::string& diretorioSaida,
        const std::string& arquivoDashboard
    );
};