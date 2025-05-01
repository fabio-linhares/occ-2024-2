#include "solucionar_desafio.h"
#include "parser.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "analisador_relevancia.h"
#include "gestor_waves.h" 
#include "seletor_waves.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <thread>
#include <mutex>
#include <vector>

// Função auxiliar para gerar um número aleatório dentro de um intervalo
int gerarNumeroAleatorio(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::mutex mutex;
    
    // Verificar e corrigir caso o intervalo seja inválido
    if (min > max) {
        std::swap(min, max);
    }
    
    std::uniform_int_distribution<> distrib(min, max);
    
    // Proteger o acesso ao gerador com mutex
    std::lock_guard<std::mutex> lock(mutex);
    return distrib(gen);
}

// Função para processar um único arquivo
void processarArquivo(const std::filesystem::path& arquivoPath, 
                     const std::string& diretorioSaida,
                     std::mutex& cout_mutex) {
    std::string arquivoEntrada = arquivoPath.string();
    std::string nomeArquivo = arquivoPath.filename().string();
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Processando: " << nomeArquivo << std::endl;
    }

    try {
        // Carregar a instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(arquivoEntrada);

        // Inicializar as estruturas auxiliares
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);

        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);

        AnalisadorRelevancia analisador(backlog.numPedidos);
        analisador.construir(backlog, localizador);

        // Gerar solução inicial usando as estruturas auxiliares
        Solucao solucaoInicial = gerarSolucaoInicial(deposito, backlog, localizador, verificador, analisador);

        // Otimizar a solução usando as estruturas auxiliares
        Solucao solucaoOtima = otimizarSolucao(deposito, backlog, solucaoInicial, localizador, verificador, analisador);

        // Ajustar a solução final para garantir viabilidade
        Solucao solucaoFinal = ajustarSolucao(deposito, backlog, solucaoOtima, localizador, verificador);

        // Salvar a solução
        salvarSolucao(diretorioSaida, nomeArquivo, solucaoFinal);

    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Erro ao processar arquivo " << nomeArquivo << ": " << e.what() << std::endl;
    }
}

void solucionarDesafio(const std::string& diretorioEntrada, const std::string& diretorioSaida) {
    // 1. Criar diretório de saída se não existir
    if (!std::filesystem::exists(diretorioSaida)) {
        std::filesystem::create_directory(diretorioSaida);
    }

    // 2. Coletar todos os arquivos primeiro
    std::vector<std::filesystem::path> arquivos;
    for (const auto& entry : std::filesystem::directory_iterator(diretorioEntrada)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            arquivos.push_back(entry.path());
        }
    }
    
    // 3. Determinar o número de threads a utilizar
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Fallback se hardware_concurrency() retornar 0
    
    // Limitar número de threads ao número de arquivos
    numThreads = std::min(numThreads, static_cast<unsigned int>(arquivos.size()));
    
    // 4. Criar mutex para proteção da saída de console
    std::mutex cout_mutex;
    
    // 5. Criar e iniciar threads
    std::vector<std::thread> threads;
    
    for (unsigned int t = 0; t < numThreads; t++) {
        threads.emplace_back([t, numThreads, &arquivos, &diretorioSaida, &cout_mutex]() {
            // Cada thread processa uma fração dos arquivos
            for (size_t i = t; i < arquivos.size(); i += numThreads) {
                processarArquivo(arquivos[i], diretorioSaida, cout_mutex);
            }
        });
    }
    
    // 6. Aguardar término de todas as threads
    for (auto& thread : threads) {
        thread.join();
    }
}

Solucao gerarSolucaoInicial(const Deposito& deposito, const Backlog& backlog, 
                           const LocalizadorItens& localizador, 
                           const VerificadorDisponibilidade& verificador,
                           const AnalisadorRelevancia& analisador) {
    Solucao solucao;
    solucao.valorObjetivo = 0.0;

    // Usar o AnalisadorRelevancia para obter pedidos ordenados por relevância
    std::vector<int> pedidosOrdenados = analisador.getPedidosOrdenadosPorRelevancia();
    
    // Adicionar pedidos até atingir o limite inferior da wave
    int unidadesNaWave = 0;
    std::unordered_set<int> corredoresNecessarios;
    
    for (int pedidoId : pedidosOrdenados) {
        // Verificar se o pedido pode ser atendido com o estoque atual
        if (!verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
            continue;
        }
        
        int unidadesPedido = analisador.infoPedidos[pedidoId].numUnidades;

        if (unidadesNaWave + unidadesPedido <= backlog.wave.UB) {
            solucao.pedidosWave.push_back(pedidoId);
            unidadesNaWave += unidadesPedido;
            
            // Adicionar corredores necessários para este pedido
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                // Usar o LocalizadorItens para encontrar os corredores com este item
                const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
                
                int quantidadeRestante = quantidade;
                
                // Ordenar corredores por quantidade disponível
                std::vector<std::pair<int, int>> corredoresOrdenados(
                    corredoresComItem.begin(), corredoresComItem.end());
                std::sort(corredoresOrdenados.begin(), corredoresOrdenados.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                
                for (const auto& [corredorId, quantidadeDisponivel] : corredoresOrdenados) {
                    if (quantidadeRestante <= 0) break;
                    
                    corredoresNecessarios.insert(corredorId);
                    quantidadeRestante -= std::min(quantidadeRestante, quantidadeDisponivel);
                }
            }
        }

        if (unidadesNaWave >= backlog.wave.LB) {
            break;
        }
    }
    
    solucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
    
    // Calcular o valor objetivo da solução inicial
    solucao.valorObjetivo = calcularValorObjetivo(deposito, backlog, solucao);

    return solucao;
}

Solucao perturbarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoAtual,
                         const LocalizadorItens& localizador, 
                         const VerificadorDisponibilidade& verificador,
                         const AnalisadorRelevancia& analisador) {
    Solucao solucaoPerturbada = solucaoAtual;

    // Perturbar a solução: remover alguns pedidos aleatoriamente
    if (!solucaoPerturbada.pedidosWave.empty()) {
        // Garantir que não removemos mais itens do que existem
        int numPedidosRemover = std::min(
            gerarNumeroAleatorio(1, std::max(1, (int)solucaoPerturbada.pedidosWave.size() / 2)),
            (int)solucaoPerturbada.pedidosWave.size());
        
        for (int i = 0; i < numPedidosRemover && !solucaoPerturbada.pedidosWave.empty(); i++) {
            int indexRemover = gerarNumeroAleatorio(0, solucaoPerturbada.pedidosWave.size() - 1);
            solucaoPerturbada.pedidosWave.erase(solucaoPerturbada.pedidosWave.begin() + indexRemover);
        }
    }

    // Recalcular os corredores necessários usando o LocalizadorItens
    std::unordered_set<int> corredoresNecessarios;
    for (int pedidoId : solucaoPerturbada.pedidosWave) {
        for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
            const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
            
            int quantidadeRestante = quantidadeSolicitada;
            
            // Ordenar corredores por quantidade disponível
            std::vector<std::pair<int, int>> corredoresOrdenados(
                corredoresComItem.begin(), corredoresComItem.end());
            std::sort(corredoresOrdenados.begin(), corredoresOrdenados.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (const auto& [corredorId, quantidadeDisponivel] : corredoresOrdenados) {
                if (quantidadeRestante <= 0) break;
                
                corredoresNecessarios.insert(corredorId);
                quantidadeRestante -= std::min(quantidadeRestante, quantidadeDisponivel);
            }
        }
    }
    solucaoPerturbada.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());

    // Obter pedidos ordenados por relevância
    std::vector<int> pedidosOrdenados = analisador.getPedidosOrdenadosPorRelevancia();
    
    // Adicionar novos pedidos relevantes até atingir o limite superior da wave
    int unidadesNaWave = 0;
    for (int pedidoId : solucaoPerturbada.pedidosWave) {
        unidadesNaWave += analisador.infoPedidos[pedidoId].numUnidades;
    }

    for (int pedidoId : pedidosOrdenados) {
        // Pular pedidos que já estão na wave
        if (std::find(solucaoPerturbada.pedidosWave.begin(), solucaoPerturbada.pedidosWave.end(), pedidoId) 
            != solucaoPerturbada.pedidosWave.end()) {
            continue;
        }
        
        int unidadesPedido = analisador.infoPedidos[pedidoId].numUnidades;

        if (unidadesNaWave + unidadesPedido <= backlog.wave.UB) {
            // Verificar se há estoque disponível usando o VerificadorDisponibilidade
            if (verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
                solucaoPerturbada.pedidosWave.push_back(pedidoId);
                unidadesNaWave += unidadesPedido;
                
                // Atualizar corredores necessários
                for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                    const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
                    
                    int quantidadeRestante = quantidadeSolicitada;
                    
                    // Ordenar corredores por quantidade disponível
                    std::vector<std::pair<int, int>> corredoresOrdenados(
                        corredoresComItem.begin(), corredoresComItem.end());
                    std::sort(corredoresOrdenados.begin(), corredoresOrdenados.end(),
                        [](const auto& a, const auto& b) { return a.second > b.second; });
                    
                    for (const auto& [corredorId, quantidadeDisponivel] : corredoresOrdenados) {
                        if (quantidadeRestante <= 0) break;
                        
                        corredoresNecessarios.insert(corredorId);
                        quantidadeRestante -= std::min(quantidadeRestante, quantidadeDisponivel);
                    }
                }
            }
        }

        if (unidadesNaWave >= backlog.wave.LB) {
            break;
        }
    }

    // Atualizar corredores necessários
    solucaoPerturbada.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());

    // Recalcular o valor objetivo
    solucaoPerturbada.valorObjetivo = calcularValorObjetivo(deposito, backlog, solucaoPerturbada);

    return solucaoPerturbada;
}

Solucao otimizarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoInicial,
                        const LocalizadorItens& localizador, 
                        const VerificadorDisponibilidade& verificador,
                        const AnalisadorRelevancia& analisador) {
    const int MAX_ITERACOES = 100;
    
    // Determinar número de threads
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    numThreads = std::min(numThreads, 8u); // Limitar threads para evitar sobrecarga
    
    Solucao melhorSolucao = solucaoInicial;
    double lambda = 0.0;
    std::mutex melhorSolucaoMutex;
    
    for (int iteracao = 0; iteracao < MAX_ITERACOES; iteracao += numThreads) {
        // Preparar estruturas para trabalho paralelo
        std::vector<Solucao> solucoesPerturbadas(numThreads);
        std::vector<double> numeradores(numThreads, -1.0);
        std::vector<std::thread> threads;
        
        // Lançar threads para gerar e avaliar perturbações em paralelo
        for (unsigned int t = 0; t < numThreads && (iteracao + t) < MAX_ITERACOES; t++) {
            threads.emplace_back([t, &deposito, &backlog, &melhorSolucao, &solucoesPerturbadas,
                                 &numeradores, &localizador, &verificador, &analisador, lambda]() {
                // Criar uma cópia da solução atual para perturbar
                Solucao solucaoAtual = melhorSolucao;
                
                // Perturbar e avaliar a solução
                solucoesPerturbadas[t] = perturbarSolucao(deposito, backlog, solucaoAtual,
                                                         localizador, verificador, analisador);
                
                if (!solucoesPerturbadas[t].corredoresWave.empty()) {
                    double totalUnidades = 0.0;
                    for (int pedidoId : solucoesPerturbadas[t].pedidosWave) {
                        totalUnidades += analisador.infoPedidos[pedidoId].numUnidades;
                    }
                    
                    numeradores[t] = totalUnidades - lambda * solucoesPerturbadas[t].corredoresWave.size();
                }
            });
        }
        
        // Aguardar threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Encontrar a melhor perturbação entre as geradas
        double melhorNumerador = -1.0;
        int melhorIndice = -1;
        
        for (unsigned int t = 0; t < numThreads && (iteracao + t) < MAX_ITERACOES; t++) {
            if (numeradores[t] > melhorNumerador) {
                melhorNumerador = numeradores[t];
                melhorIndice = t;
            }
        }
        
        // Atualizar melhor solução
        if (melhorIndice >= 0 && melhorNumerador > 0) {
            std::lock_guard<std::mutex> lock(melhorSolucaoMutex);
            melhorSolucao = solucoesPerturbadas[melhorIndice];
        } else {
            std::lock_guard<std::mutex> lock(melhorSolucaoMutex);
            lambda = calcularValorObjetivo(deposito, backlog, melhorSolucao);
        }
    }
    
    return melhorSolucao;
}

double calcularValorObjetivo(const Deposito& deposito, const Backlog& backlog, const Solucao& solucao) {
    if (solucao.corredoresWave.empty()) {
        return 0.0;
    }

    // Usar AnalisadorRelevancia para obter o número de unidades de cada pedido
    double totalUnidades = 0.0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }

    return totalUnidades / solucao.corredoresWave.size();
}

void salvarSolucao(const std::string& diretorioSaida, const std::string& nomeArquivo, const Solucao& solucao) {
    std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
    std::string arquivoSaida = diretorioSaida + "/" + nomeArquivoSemExtensao + ".sol";
    std::ofstream arquivo(arquivoSaida);

    if (arquivo.is_open()) {
        // Salvar número de pedidos na wave
        arquivo << solucao.pedidosWave.size() << std::endl;

        // Salvar IDs dos pedidos na wave
        for (int pedidoId : solucao.pedidosWave) {
            arquivo << pedidoId << std::endl;
        }

        // Salvar número de corredores visitados
        arquivo << solucao.corredoresWave.size() << std::endl;

        // Salvar IDs dos corredores visitados
        for (int corredorId : solucao.corredoresWave) {
            arquivo << corredorId << std::endl;
        }

        arquivo.close();
        std::cout << "Solução salva em: " << arquivoSaida << std::endl;
    } else {
        std::cerr << "Erro ao salvar o arquivo: " << arquivoSaida << std::endl;
    }
}

Solucao ajustarSolucao(const Deposito& deposito, const Backlog& backlog, Solucao solucao,
                      const LocalizadorItens& localizador, 
                      const VerificadorDisponibilidade& verificador) {
    // Inicializar o estoque disponível baseado nos corredores selecionados
    std::unordered_map<int, int> estoqueDisponivel;
    for (int corredorId : solucao.corredoresWave) {
        for (const auto& [itemId, quantidade] : deposito.corredor[corredorId]) {
            estoqueDisponivel[itemId] += quantidade;
        }
    }

    // Identificar pedidos com estoque insuficiente
    std::vector<int> pedidosParaRemover;
    for (int pedidoId : solucao.pedidosWave) {
        bool estoqueInsuficiente = false;
        for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
            if (estoqueDisponivel[itemId] < quantidadeSolicitada) {
                estoqueInsuficiente = true;
                break;
            }
        }
        
        if (estoqueInsuficiente) {
            pedidosParaRemover.push_back(pedidoId);
        } else {
            // Deduzir o estoque para os pedidos que serão mantidos
            for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                estoqueDisponivel[itemId] -= quantidadeSolicitada;
            }
        }
    }

    // Remover os pedidos com estoque insuficiente
    for (int pedidoId : pedidosParaRemover) {
        solucao.pedidosWave.erase(
            std::remove(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId),
            solucao.pedidosWave.end());
    }

    // Recalcular o número total de unidades na wave
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }

    // Se o total de unidades for inferior a LB, adicionar mais pedidos até atingir LB
    if (totalUnidades < backlog.wave.LB) {
        // Usar o AnalisadorRelevancia para obter pedidos ordenados por relevância
        AnalisadorRelevancia analisador(backlog.numPedidos);
        analisador.construir(backlog, localizador);
        std::vector<int> pedidosOrdenados = analisador.getPedidosOrdenadosPorRelevancia();
        
        for (int pedidoId : pedidosOrdenados) {
            // Pular pedidos que já estão na wave
            if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) 
                != solucao.pedidosWave.end()) {
                continue;
            }
            
            // Verificar se o pedido pode ser atendido com o estoque disponível
            bool estoqueInsuficiente = false;
            for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                if (estoqueDisponivel[itemId] < quantidadeSolicitada) {
                    estoqueInsuficiente = true;
                    break;
                }
            }
            
            if (!estoqueInsuficiente) {
                int unidadesPedido = 0;
                for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                    unidadesPedido += quantidade;
                    estoqueDisponivel[itemId] -= quantidade;
                }
                
                solucao.pedidosWave.push_back(pedidoId);
                totalUnidades += unidadesPedido;
                
                if (totalUnidades >= backlog.wave.LB) {
                    break;
                }
            }
        }
    }
    // Se o total de unidades for superior a UB, remover pedidos até atingir UB
    else if (totalUnidades > backlog.wave.UB) {
        // Ordenar pedidos pelo inverso da pontuação de relevância (menos relevantes primeiro)
        AnalisadorRelevancia analisador(backlog.numPedidos);
        analisador.construir(backlog, localizador);
        
        std::vector<int> pedidosNaWave = solucao.pedidosWave;
        std::sort(pedidosNaWave.begin(), pedidosNaWave.end(),
            [&analisador](int a, int b) {
                return analisador.infoPedidos[a].pontuacaoRelevancia < 
                       analisador.infoPedidos[b].pontuacaoRelevancia;
            });
        
        // Remover pedidos menos relevantes até atingir UB
        for (int pedidoId : pedidosNaWave) {
            int unidadesPedido = 0;
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }
            
            if (totalUnidades - unidadesPedido >= backlog.wave.LB) {
                solucao.pedidosWave.erase(
                    std::remove(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId),
                    solucao.pedidosWave.end());
                totalUnidades -= unidadesPedido;
                
                if (totalUnidades <= backlog.wave.UB) {
                    break;
                }
            }
        }
    }

    // Recalcular os corredores necessários usando o LocalizadorItens
    std::unordered_set<int> corredoresNecessarios;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
            
            int quantidadeRestante = quantidade;
            
            // Ordenar corredores por quantidade disponível
            std::vector<std::pair<int, int>> corredoresOrdenados(
                corredoresComItem.begin(), corredoresComItem.end());
            std::sort(corredoresOrdenados.begin(), corredoresOrdenados.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (const auto& [corredorId, quantidadeDisponivel] : corredoresOrdenados) {
                if (quantidadeRestante <= 0) break;
                
                corredoresNecessarios.insert(corredorId);
                quantidadeRestante -= std::min(quantidadeRestante, quantidadeDisponivel);
            }
        }
    }
    solucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());

    // Recalcular o valor objetivo
    solucao.valorObjetivo = calcularValorObjetivo(deposito, backlog, solucao);

    return solucao;
}