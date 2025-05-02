#pragma once

#include <string>
#include <vector>
#include <future>
#include "armazem.h"
#include "solucionar_desafio.h"

/**
 * @brief Classe para otimização distribuída de instâncias massivas
 */
class OtimizadorDistribuido {
private:
    std::string diretorioEntrada_;
    std::string diretorioSaida_;
    int numUnidadesProcessamento_;
    
public:
    /**
     * @brief Construtor
     * @param diretorioEntrada Diretório com arquivos de instância
     * @param diretorioSaida Diretório para arquivos de solução
     * @param numUnidades Número de unidades de processamento
     */
    OtimizadorDistribuido(
        const std::string& diretorioEntrada,
        const std::string& diretorioSaida,
        int numUnidades = 0
    );
    
    /**
     * @brief Decompõe uma instância massiva em subproblemas
     * @param caminhoInstancia Caminho do arquivo de instância
     * @param numParticoes Número de partições a criar
     * @return Lista de caminhos para os arquivos de subproblemas
     */
    std::vector<std::string> decomporInstancia(
        const std::string& caminhoInstancia,
        int numParticoes
    );
    
    /**
     * @brief Resolve subproblemas em paralelo
     * @param arquivosSubproblemas Lista de arquivos de subproblemas
     * @return Futuros com resultados dos subproblemas
     */
    std::vector<std::future<Solucao>> resolverSubproblemasParalelo(
        const std::vector<std::string>& arquivosSubproblemas
    );
    
    /**
     * @brief Combina soluções de subproblemas em uma solução global
     * @param solucoesSubproblemas Soluções dos subproblemas
     * @param deposito Dados do depósito original
     * @param backlog Dados do backlog original
     * @return Solução global combinada
     */
    Solucao combinarSolucoes(
        const std::vector<Solucao>& solucoesSubproblemas,
        const Deposito& deposito,
        const Backlog& backlog
    );
    
    /**
     * @brief Soluciona uma instância massiva usando decomposição e paralelização
     * @param caminhoInstancia Caminho do arquivo de instância
     * @param numParticoes Número de partições (0 = automático)
     * @return Solução otimizada
     */
    Solucao solucionarInstanciaMassiva(
        const std::string& caminhoInstancia,
        int numParticoes = 0
    );
};