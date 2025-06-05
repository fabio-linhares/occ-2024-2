#include "validar_resultados.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>  // Adicionando este cabeçalho faltante
#include <filesystem>     // Adicionando este cabeçalho faltante
#include <iomanip>

/**
 * @brief Estrutura para armazenar os dados de um arquivo de solução
 */
struct SolucaoValidacao {
    std::vector<int> pedidosWave;
    std::vector<int> corredoresWave;
};

/**
 * @brief Lê um arquivo de solução e retorna os dados
 * @param arquivoSolucao Caminho para o arquivo de solução
 * @return SolucaoValidacao Estrutura com os dados do arquivo de solução
 */
SolucaoValidacao lerArquivoSolucao(const std::string& arquivoSolucao) {
    SolucaoValidacao solucao;
    std::ifstream file(arquivoSolucao);
    std::string linha;

    if (!file.is_open()) {
        throw std::runtime_error("Não foi possível abrir o arquivo de solução: " + arquivoSolucao);
    }

    // Ler número de pedidos na wave
    int numPedidos;
    if (std::getline(file, linha)) {
        std::stringstream ss(linha);
        if (!(ss >> numPedidos)) {
            throw std::runtime_error("Erro ao ler o número de pedidos na wave");
        }
    } else {
        throw std::runtime_error("Arquivo de solução incompleto: número de pedidos ausente");
    }

    // Ler IDs dos pedidos na wave
    for (int i = 0; i < numPedidos; ++i) {
        if (std::getline(file, linha)) {
            std::stringstream ss(linha);
            int pedidoId;
            if (!(ss >> pedidoId)) {
                throw std::runtime_error("Erro ao ler o ID do pedido na linha " + std::to_string(i + 2));
            }
            solucao.pedidosWave.push_back(pedidoId);
        } else {
            throw std::runtime_error("Arquivo de solução incompleto: IDs de pedidos ausentes");
        }
    }

    // Ler número de corredores visitados
    int numCorredores;
    if (std::getline(file, linha)) {
        std::stringstream ss(linha);
        if (!(ss >> numCorredores)) {
            throw std::runtime_error("Erro ao ler o número de corredores visitados");
        }
    } else {
        throw std::runtime_error("Arquivo de solução incompleto: número de corredores ausente");
    }

    // Ler IDs dos corredores visitados
    for (int i = 0; i < numCorredores; ++i) {
        if (std::getline(file, linha)) {
            std::stringstream ss(linha);
            int corredorId;
            if (!(ss >> corredorId)) {
                throw std::runtime_error("Erro ao ler o ID do corredor na linha " + std::to_string(i + numPedidos + 3));
            }
            solucao.corredoresWave.push_back(corredorId);
        } else {
            throw std::runtime_error("Arquivo de solução incompleto: IDs de corredores ausentes");
        }
    }

    file.close();
    return solucao;
}

/**
 * @brief Valida as restrições do problema para uma dada solução e retorna um relatório detalhado
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucaoValidacao Dados do arquivo de solução
 * @param logFile Referência para o arquivo de log
 * @return bool True se todas as restrições forem atendidas, false caso contrário
 */
bool validarRestricoes(const Deposito& deposito, const Backlog& backlog, const SolucaoValidacao& solucaoValidacao, std::ofstream& logFile) {
    bool validacaoAprovada = true;

    // 1. Validação dos IDs dos pedidos
    logFile << "  1. Validação dos IDs dos pedidos: ";
    bool pedidosValidos = true;
    for (int pedidoId : solucaoValidacao.pedidosWave) {
        if (pedidoId < 0 || pedidoId >= backlog.numPedidos) {
            logFile << "Reprovada\n";
            logFile << "     Erro: ID de pedido inválido: " << pedidoId << " (intervalo válido: 0-" << backlog.numPedidos - 1 << ")\n";
            validacaoAprovada = false;
            pedidosValidos = false;
            break;
        }
    }
    if (pedidosValidos) {
        logFile << "Aprovada\n";
    }

    // 2. Validação dos IDs dos corredores
    logFile << "  2. Validação dos IDs dos corredores: ";
    bool corredoresValidos = true;
    for (int corredorId : solucaoValidacao.corredoresWave) {
        if (corredorId < 0 || corredorId >= deposito.numCorredores) {
            logFile << "Reprovada\n";
            logFile << "     Erro: ID de corredor inválido: " << corredorId << " (intervalo válido: 0-" << deposito.numCorredores - 1 << ")\n";
            validacaoAprovada = false;
            corredoresValidos = false;
            break;
        }
    }
    if (corredoresValidos) {
        logFile << "Aprovada\n";
    }

    // 3. Validação do número total de unidades na wave
    logFile << "  3. Validação do número total de unidades na wave: ";
    int totalUnidades = 0;
    for (int pedidoId : solucaoValidacao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    logFile << "Total de unidades na wave: " << totalUnidades << ", Limites LB e UB: " << backlog.wave.LB << " - " << backlog.wave.UB << ": ";

    bool unidadesValidas = true;
    if (totalUnidades < backlog.wave.LB || totalUnidades > backlog.wave.UB) {
        logFile << "Reprovada\n";
        logFile << "     Erro: Número total de unidades (" << totalUnidades << ") fora dos limites LB e UB (" <<
                   backlog.wave.LB << " - " << backlog.wave.UB << ")\n";
        validacaoAprovada = false;
        unidadesValidas = false;
    }
    if (unidadesValidas) {
        logFile << "Aprovada\n";
    }

    // 4. Validação de estoque suficiente
    logFile << "  4. Validação de estoque suficiente: ";
    std::unordered_map<int, int> estoqueDisponivel;
    for (int corredorId : solucaoValidacao.corredoresWave) {
        for (const auto& [itemId, quantidade] : deposito.corredor[corredorId]) {
            estoqueDisponivel[itemId] += quantidade;
        }
    }

    bool estoqueSuficiente = true;
    for (int pedidoId : solucaoValidacao.pedidosWave) {
        for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
            if (estoqueDisponivel[itemId] < quantidadeSolicitada) {
                logFile << "Reprovada\n";
                logFile << "     Erro: Estoque insuficiente para o item " << itemId << " no pedido " << pedidoId << "\n";
                logFile << "       Quantidade solicitada: " << quantidadeSolicitada << "\n";
                logFile << "       Estoque disponível: " << estoqueDisponivel[itemId] << "\n";
                validacaoAprovada = false;
                estoqueSuficiente = false;
                break;
            }
        }
        if (!estoqueSuficiente) break;
    }

    if (estoqueSuficiente) {
        logFile << "Aprovada\n";
    }

    return validacaoAprovada;
}

void validarResultados(const std::string& diretorioEntrada, const std::string& diretorioSaida, const std::string& arquivoLog) {
    std::ofstream logFile(arquivoLog);
    if (!logFile.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de log: " << arquivoLog << std::endl;
        return;
    }

    logFile << "=== Relatório de Validação dos Resultados ===\n\n";

    // Iterar sobre os arquivos de entrada
    for (const auto& entry : std::filesystem::directory_iterator(diretorioEntrada)) {
        if (entry.is_regular_file()) {
            std::string arquivoEntrada = entry.path().string();
            std::string nomeArquivo = entry.path().filename().string();
            std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
            std::string arquivoSolucao = diretorioSaida + "/" + nomeArquivoSemExtensao + ".sol";

            logFile << "Arquivo de entrada: " << arquivoEntrada << "\n";
            logFile << "Arquivo de solução: " << arquivoSolucao << "\n";

            try {
                // Carregar a instância
                InputParser parser;
                auto [deposito, backlog] = parser.parseFile(arquivoEntrada);

                // Ler o arquivo de solução
                SolucaoValidacao solucaoValidacao = lerArquivoSolucao(arquivoSolucao);

                // Validar as restrições
                bool validacaoAprovada = validarRestricoes(deposito, backlog, solucaoValidacao, logFile);

                if (validacaoAprovada) {
                    logFile << "Validação: Aprovada\n";
                } else {
                    logFile << "Validação: Reprovada\n";
                }
            } catch (const std::exception& e) {
                logFile << "Erro ao validar: " << e.what() << "\n";
            }

            logFile << "----------------------------------------\n";
        }
    }

    logFile.close();
    std::cout << "Relatório de validação salvo em: " << arquivoLog << std::endl;
}