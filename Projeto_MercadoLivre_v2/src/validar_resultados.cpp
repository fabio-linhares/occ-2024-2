#include "validar_resultados.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <chrono>

// Definição de cores para formatação do console
const std::string RESET = "\033[0m";
const std::string VERMELHO = "\033[31m";
const std::string VERDE = "\033[32m";
const std::string CIANO = "\033[36m";

// Definição de bordas para formatação
const std::string BORDA_ES = "┌";
const std::string BORDA_SD = "┐";
const std::string BORDA_V = "│";
const std::string BORDA_EJ = "├";
const std::string BORDA_DJ = "┤";
const std::string BORDA_DS = "└";
const std::string BORDA_ID = "┘";

// Funções de formatação para saída no console
std::string colorir(const std::string& texto, const std::string& cor) {
    return cor + texto + RESET;
}

std::string colorirBold(const std::string& texto, const std::string& cor) {
    return "\033[1m" + cor + texto + RESET;
}

std::string cabecalho(const std::string& texto) {
    std::stringstream ss;
    std::string linha(texto.length() + 4, '=');
    ss << "\n" << linha << "\n= " << texto << " =\n" << linha << "\n";
    return ss.str();
}

std::string sucesso(const std::string& texto) {
    return colorir(texto, VERDE);
}

std::string linhaHorizontal(int largura) {
    std::string linha;
    linha.reserve(largura);
    for (int i = 0; i < largura; i++) {
        linha += "─";
    }
    return linha;
}

// Função para gerar nome de arquivo com timestamp atual
std::string gerarNomeArquivoComTimestamp() {
    // Obter timestamp atual
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    // Converter para struct tm para acessar componentes
    std::tm tm_info = *std::localtime(&time);
    
    // Formatar conforme especificação: DDMMAA-HHMM
    std::stringstream ss;
    ss << "validation_log_";
    
    // Dia - com zero à esquerda se necessário
    ss << std::setfill('0') << std::setw(2) << tm_info.tm_mday;
    
    // Mês - com zero à esquerda se necessário (tm_mon é base 0, então +1)
    ss << std::setfill('0') << std::setw(2) << (tm_info.tm_mon + 1);
    
    // Ano - apenas últimos dois dígitos
    ss << std::setfill('0') << std::setw(2) << (tm_info.tm_year % 100);
    
    ss << "-";
    
    // Hora - com zero à esquerda se necessário
    ss << std::setfill('0') << std::setw(2) << tm_info.tm_hour;
    
    // Minutos - com zero à esquerda se necessário
    ss << std::setfill('0') << std::setw(2) << tm_info.tm_min;
    
    ss << ".txt";
    
    return ss.str();
}

// Carregar os tempos de execução
std::unordered_map<std::string, double> carregarTemposExecucao() {
    std::unordered_map<std::string, double> tempos;
    std::ifstream inFile("data/tempos_execucao.csv");
    
    if (inFile.is_open()) {
        std::string linha;
        // Pular a linha de cabeçalho
        std::getline(inFile, linha);
        
        while (std::getline(inFile, linha)) {
            std::istringstream ss(linha);
            std::string instancia;
            double tempo;
            
            if (std::getline(ss, instancia, ',') && (ss >> tempo)) {
                tempos[instancia] = tempo;
            }
        }
        
        inFile.close();
    }
    
    return tempos;
}

/**
 * @brief Estrutura para armazenar os dados de um arquivo de solução
 */
struct SolucaoValidacao {
    std::vector<int> pedidosWave;
    std::vector<int> corredoresWave;
};

/**
 * @brief Calcula o valor objetivo para uma solução
 * @param deposito Dados do depósito
 * @param backlog Dados do backlog
 * @param solucaoValidacao Dados da solução
 * @return Valor objetivo calculado
 */
double calcularValorObjetivo(const Deposito& deposito, const Backlog& backlog, const SolucaoValidacao& solucaoValidacao) {
    // Se não há pedidos ou corredores na solução, retorna 0
    if (solucaoValidacao.pedidosWave.empty() || solucaoValidacao.corredoresWave.empty()) {
        return 0.0;
    }
    
    // Calcular o total de unidades
    int totalUnidades = 0;
    for (int pedidoId : solucaoValidacao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Dividir pelo número de corredores
    return static_cast<double>(totalUnidades) / solucaoValidacao.corredoresWave.size();
}

/**
 * @brief Retorna um mapa com os BOVs oficiais do desafio
 * @return Mapa de instâncias para BOVs
 */
std::unordered_map<std::string, double> getBOVsOficiais() {
    std::unordered_map<std::string, double> bovs;
    bovs["instance_0001"] = 15.00;
    bovs["instance_0002"] = 2.00;
    bovs["instance_0003"] = 12.00;
    bovs["instance_0004"] = 3.50;
    bovs["instance_0005"] = 177.88;
    bovs["instance_0006"] = 691.00;
    bovs["instance_0007"] = 392.25;
    bovs["instance_0008"] = 162.94;
    bovs["instance_0009"] = 4.42;
    bovs["instance_0010"] = 16.79;
    bovs["instance_0011"] = 16.85;
    bovs["instance_0012"] = 11.25;
    bovs["instance_0013"] = 117.38;
    bovs["instance_0014"] = 181.64;
    bovs["instance_0015"] = 149.33;
    bovs["instance_0016"] = 85.00;
    bovs["instance_0017"] = 36.50;
    bovs["instance_0018"] = 117.20;
    bovs["instance_0019"] = 202.00;
    bovs["instance_0020"] = 5.00;
    return bovs;
}

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

    try {
        // Ler número de pedidos na wave
        if (std::getline(file, linha)) {
            std::istringstream ss(linha);
            int numPedidos;
            if (!(ss >> numPedidos)) {
                throw std::runtime_error("Erro ao ler o número de pedidos na wave");
            }
        } else {
            throw std::runtime_error("Arquivo de solução incompleto: número de pedidos ausente");
        }

        // Ler IDs dos pedidos na wave (todos na mesma linha)
        if (std::getline(file, linha)) {
            std::istringstream ss(linha);
            int pedidoId;
            while (ss >> pedidoId) {
                solucao.pedidosWave.push_back(pedidoId);
            }
        } else {
            throw std::runtime_error("Arquivo de solução incompleto: IDs de pedidos ausentes");
        }

        // Ler número de corredores visitados
        if (std::getline(file, linha)) {
            std::istringstream ss(linha);
            int numCorredores;
            if (!(ss >> numCorredores)) {
                throw std::runtime_error("Erro ao ler o número de corredores visitados");
            }
        } else {
            throw std::runtime_error("Arquivo de solução incompleto: número de corredores ausente");
        }

        // Ler IDs dos corredores visitados (todos na mesma linha)
        if (std::getline(file, linha)) {
            std::istringstream ss(linha);
            int corredorId;
            while (ss >> corredorId) {
                solucao.corredoresWave.push_back(corredorId);
            }
        } else {
            throw std::runtime_error("Arquivo de solução incompleto: IDs de corredores ausentes");
        }
    } catch (const std::exception& e) {
        file.close();
        throw;
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
 * @param nomeArquivoSemExtensao Nome do arquivo sem extensão para identificar a instância
 * @return bool True se todas as restrições forem atendidas, false caso contrário
 */
bool validarRestricoes(const Deposito& deposito, const Backlog& backlog, const SolucaoValidacao& solucaoValidacao, 
                       std::ofstream& logFile, const std::string& nomeArquivoSemExtensao) {
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

    // 5. Cálculo e comparação do valor objetivo
    double valorObjetivo = calcularValorObjetivo(deposito, backlog, solucaoValidacao);
    logFile << "  5. Valor objetivo (BOV): " << std::fixed << std::setprecision(2) << valorObjetivo << "\n";
    
    // Comparar com o BOV oficial
    auto bovs = getBOVsOficiais();
    std::string nomeInstancia = nomeArquivoSemExtensao;
    std::transform(nomeInstancia.begin(), nomeInstancia.end(), nomeInstancia.begin(), 
                 [](unsigned char c){ return std::tolower(c); });
    
    if (bovs.find(nomeInstancia) != bovs.end()) {
        double bovOficial = bovs[nomeInstancia];
        double diferenca = valorObjetivo - bovOficial;
        double percentual = (bovOficial > 0) ? (diferenca / bovOficial) * 100.0 : 0.0;
        
        logFile << "     BOV oficial: " << std::fixed << std::setprecision(2) << bovOficial << "\n";
        logFile << "     Diferença: " << std::fixed << std::setprecision(2) << diferenca;
        
        if (diferenca > 0) {
            logFile << " (+" << std::fixed << std::setprecision(2) << percentual << "% acima do BOV oficial)\n";
        } else if (diferenca < 0) {
            logFile << " (" << std::fixed << std::setprecision(2) << percentual << "% abaixo do BOV oficial)\n";
        } else {
            logFile << " (igual ao BOV oficial)\n";
        }
    } else {
        logFile << "     BOV oficial não disponível para esta instância\n";
    }

    return validacaoAprovada;
}

void validarResultados(const std::string& diretorioEntrada, const std::string& diretorioSolucoes) {
    std::cout << cabecalho("VALIDAÇÃO DE RESULTADOS") << std::endl;

    std::stringstream ss;
    const int largura = 58;
    ss << BORDA_ES << linhaHorizontal(largura - 2) << BORDA_SD << "\n";
    ss << BORDA_V << " " << colorirBold("CONFIGURAÇÕES DE VALIDAÇÃO", CIANO) << std::string(29, ' ') << BORDA_V << "\n";
    ss << BORDA_EJ << linhaHorizontal(largura - 2) << BORDA_DJ << "\n";
    ss << BORDA_V << " " << colorir("• Diretório de entrada: ", CIANO) << diretorioEntrada << "\n";
    ss << BORDA_V << " " << colorir("• Diretório de soluções: ", CIANO) << diretorioSolucoes << "\n";
    ss << BORDA_DS << linhaHorizontal(largura - 2) << BORDA_ID;

    std::cout << ss.str() << std::endl << std::endl;
    
    // Verificar diretórios
    if (!std::filesystem::exists(diretorioEntrada)) {
        std::cerr << "Erro: Diretório de entrada não existe.\n";
        return;
    }
    
    if (!std::filesystem::exists(diretorioSolucoes)) {
        std::cerr << "Erro: Diretório de soluções não existe.\n";
        return;
    }
    
    // Coletar arquivos de entrada e solução
    std::vector<std::pair<std::string, std::string>> arquivos;
    for (const auto& entry : std::filesystem::directory_iterator(diretorioEntrada)) {
        if (entry.is_regular_file()) {
            std::string arquivoEntrada = entry.path().string();
            std::string nomeArquivo = entry.path().filename().string();
            std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
            std::string arquivoSolucao = diretorioSolucoes + "/" + nomeArquivoSemExtensao + ".sol";
            
            if (std::filesystem::exists(arquivoSolucao)) {
                arquivos.push_back({arquivoEntrada, arquivoSolucao});
            }
        }
    }
    
    if (arquivos.empty()) {
        std::cout << "Nenhum par de arquivos de entrada/solução encontrado para validação.\n";
        return;
    }
    
    // Gerar nome do arquivo de log com timestamp
    std::string arquivoLog = "data/" + gerarNomeArquivoComTimestamp();
    
    std::ofstream logFile(arquivoLog);
    if (!logFile.is_open()) {
        std::cerr << "Erro ao criar arquivo de log: " << arquivoLog << "\n";
        return;
    }
    
    // Carregar os tempos de execução
    auto temposExecucao = carregarTemposExecucao();
    double tempoTotal = temposExecucao["TOTAL"];
    
    logFile << "=== Relatório de Validação dos Resultados ===\n\n";
    
    double tempoTotalValidado = 0.0;
    int instanciasProcessadas = 0;

    // Iterar sobre os arquivos de entrada e solução
    for (const auto& [arquivoEntrada, arquivoSolucao] : arquivos) {
        std::string nomeArquivo = std::filesystem::path(arquivoEntrada).filename().string();
        std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
        
        logFile << "Arquivo de entrada: " << arquivoEntrada << "\n";
        logFile << "Arquivo de solução: " << arquivoSolucao << "\n";
        
        try {
            // Carregar dados da instância
            InputParser parser;
            auto [deposito, backlog] = parser.parseFile(arquivoEntrada);
            
            // Carregar dados da solução
            SolucaoValidacao solucao = lerArquivoSolucao(arquivoSolucao);
            
            // Validar a solução
            bool validada = validarRestricoes(deposito, backlog, solucao, logFile, nomeArquivoSemExtensao);
            
            // Adicionar informação de tempo de execução, se disponível
            double tempoExecucao = 0.0;
            if (temposExecucao.find(nomeArquivoSemExtensao) != temposExecucao.end()) {
                tempoExecucao = temposExecucao[nomeArquivoSemExtensao];
                tempoTotalValidado += tempoExecucao;
                instanciasProcessadas++;
                
                logFile << "  6. Tempo de processamento: " << std::fixed << std::setprecision(3) 
                        << tempoExecucao << " segundos\n";
            }
            
            logFile << "Validação: " << (validada ? "Aprovada" : "Reprovada") << "\n";
            
        } catch (const std::exception& e) {
            logFile << "Erro ao validar: " << e.what() << "\n";
            logFile << "Validação: Reprovada (erro)\n";
        }
        
        logFile << "----------------------------------------\n";
    }
    
    // Adicionar resumo dos tempos
    logFile << "\n=== Resumo dos Tempos de Execução ===\n";
    logFile << "Tempo total de execução: " << std::fixed << std::setprecision(3) << tempoTotal << " segundos\n";
    if (instanciasProcessadas > 0) {
        logFile << "Tempo médio por instância: " << std::fixed << std::setprecision(3) 
                << (tempoTotalValidado / instanciasProcessadas) << " segundos\n";
    }
    
    logFile.close();

    std::cout << std::endl;
    std::cout << sucesso("Validação concluída. Resultados salvos em:") << std::endl;
    std::cout << colorirBold("  " + arquivoLog, VERDE) << std::endl << std::endl;
}