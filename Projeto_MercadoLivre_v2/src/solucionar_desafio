#include "solucionar_desafio.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <cmath>

// Função auxiliar para gerar um número aleatório dentro de um intervalo
int gerarNumeroAleatorio(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Verificar e corrigir caso o intervalo seja inválido
    if (min > max) {
        std::swap(min, max);
    }
    
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

void solucionarDesafio(const std::string& diretorioEntrada, const std::string& diretorioSaida) {
    // 1. Criar diretório de saída se não existir
    if (!std::filesystem::exists(diretorioSaida)) {
        std::filesystem::create_directory(diretorioSaida);
    }

    // 2. Iterar sobre os arquivos no diretório de entrada
    for (const auto& entry : std::filesystem::directory_iterator(diretorioEntrada)) {
        if (entry.is_regular_file()) {
            std::string arquivoEntrada = entry.path().string();
            std::string nomeArquivo = entry.path().filename().string();
            std::cout << "Processando arquivo: " << arquivoEntrada << std::endl;

            try {
                // 3. Carregar a instância
                InputParser parser;
                auto [deposito, backlog] = parser.parseFile(arquivoEntrada);

                // 4. Gerar solução inicial (guloso)
                Solucao solucaoInicial = gerarSolucaoInicial(deposito, backlog);
                std::cout << "Solução inicial gerada." << std::endl;

                // 5. Otimizar solução (Dinkelbach)
                Solucao solucaoOtimizada = otimizarSolucao(deposito, backlog, solucaoInicial);
                std::cout << "Solução otimizada." << std::endl;

                // 6. Ajustar a solução para garantir estoque suficiente e limites LB/UB
                Solucao solucaoAjustada = ajustarSolucao(deposito, backlog, solucaoOtimizada);
                std::cout << "Solução ajustada." << std::endl;

                // 7. Salvar a solução
                salvarSolucao(diretorioSaida, nomeArquivo, solucaoAjustada);
                std::cout << "=============================================" << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "Erro ao processar arquivo " << nomeArquivo << ": " << e.what() << std::endl;
            }
        }
    }
}

Solucao gerarSolucaoInicial(const Deposito& deposito, const Backlog& backlog) {
    Solucao solucao;
    solucao.valorObjetivo = 0.0;

    // Algoritmo guloso simples: adicionar pedidos até atingir o limite inferior da wave
    int unidadesNaWave = 0;
    for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
        int unidadesPedido = 0;
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }

        if (unidadesNaWave + unidadesPedido <= backlog.wave.UB) {
            solucao.pedidosWave.push_back(pedidoId);
            unidadesNaWave += unidadesPedido;
        }

        if (unidadesNaWave >= backlog.wave.LB) {
            break;
        }
    }

    // Encontrar os corredores necessários para atender aos pedidos na wave
    std::unordered_set<int> corredoresNecessarios;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            // Encontrar um corredor que tenha o item
            for (int corredorId = 0; corredorId < deposito.numCorredores; corredorId++) {
                if (deposito.corredor[corredorId].count(itemId)) {
                    corredoresNecessarios.insert(corredorId);
                    break;
                }
            }
        }
    }
    solucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());

    // Calcular o valor objetivo da solução inicial
    solucao.valorObjetivo = calcularValorObjetivo(deposito, backlog, solucao);

    return solucao;
}

Solucao perturbarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoAtual) {
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

    // Recalcular os corredores necessários após a remoção
    std::unordered_map<int, int> estoqueDisponivel;
    std::unordered_set<int> corredoresNecessarios;
    for (int pedidoId : solucaoPerturbada.pedidosWave) {
        for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
            bool corredorEncontrado = false;
            for (int corredorId = 0; corredorId < deposito.numCorredores; corredorId++) {
                if (deposito.corredor[corredorId].count(itemId) &&
                    estoqueDisponivel[itemId] < quantidadeSolicitada) {
                    corredoresNecessarios.insert(corredorId);
                    estoqueDisponivel[itemId] += deposito.corredor[corredorId].at(itemId);
                    corredorEncontrado = true;
                    break;
                }
            }
            if (!corredorEncontrado) {
                // Se não encontrou corredor, remover o pedido da wave
                solucaoPerturbada.pedidosWave.erase(
                    std::remove(solucaoPerturbada.pedidosWave.begin(), solucaoPerturbada.pedidosWave.end(), pedidoId),
                    solucaoPerturbada.pedidosWave.end());
                break;
            }
        }
    }
    solucaoPerturbada.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());

    // Adicionar novos pedidos aleatoriamente até atingir o limite superior da wave
    std::vector<int> pedidosCandidatos;
    for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
        if (std::find(solucaoPerturbada.pedidosWave.begin(), solucaoPerturbada.pedidosWave.end(), pedidoId) == solucaoPerturbada.pedidosWave.end()) {
            pedidosCandidatos.push_back(pedidoId);
        }
    }

    std::shuffle(pedidosCandidatos.begin(), pedidosCandidatos.end(),
                std::mt19937(std::random_device()()));

    int unidadesNaWave = 0;
    for (int pedidoId : solucaoPerturbada.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            unidadesNaWave += quantidade;
        }
    }

    for (int pedidoId : pedidosCandidatos) {
        int unidadesPedido = 0;
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }

        if (unidadesNaWave + unidadesPedido <= backlog.wave.UB) {
            // Verificar se há estoque disponível para todos os itens do pedido
            bool estoqueSuficiente = true;
            for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                int estoqueNecessario = quantidadeSolicitada;
                bool itemAtendido = false;
                for (int corredorId : solucaoPerturbada.corredoresWave) {
                    if (deposito.corredor[corredorId].count(itemId)) {
                        estoqueNecessario -= deposito.corredor[corredorId].at(itemId);
                        if (estoqueNecessario <= 0) {
                            itemAtendido = true;
                            break;
                        }
                    }
                }
                if (!itemAtendido) {
                    estoqueSuficiente = false;
                    break;
                }
            }

            if (estoqueSuficiente) {
                solucaoPerturbada.pedidosWave.push_back(pedidoId);
                unidadesNaWave += unidadesPedido;
            }
        }

        if (unidadesNaWave >= backlog.wave.LB) {
            break;
        }
    }

    // Recalcular os corredores necessários
    corredoresNecessarios.clear();
    for (int pedidoId : solucaoPerturbada.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            for (int corredorId = 0; corredorId < deposito.numCorredores; corredorId++) {
                if (deposito.corredor[corredorId].count(itemId)) {
                    corredoresNecessarios.insert(corredorId);
                    break;
                }
            }
        }
    }
    solucaoPerturbada.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());

    // Recalcular o valor objetivo
    solucaoPerturbada.valorObjetivo = calcularValorObjetivo(deposito, backlog, solucaoPerturbada);

    return solucaoPerturbada;
}

Solucao otimizarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoInicial) {
    Solucao melhorSolucao = solucaoInicial;
    double lambda = 0.0; // Inicializar lambda

    for (int iteracao = 0; iteracao < 100; iteracao++) {
        // Criar uma cópia da solução atual
        Solucao solucaoAtual = melhorSolucao;

        // Perturbar a solução atual
        Solucao solucaoPerturbada = perturbarSolucao(deposito, backlog, solucaoAtual);

        // Calcular o valor objetivo modificado
        double numerador = 0.0;
        double denominador = 0.0;

        if (!solucaoPerturbada.corredoresWave.empty()) {
            // Calcular o numerador e o denominador
            double totalUnidades = 0.0;
            for (int pedidoId : solucaoPerturbada.pedidosWave) {
                for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                    totalUnidades += quantidade;
                }
            }
            numerador = totalUnidades - lambda * solucaoPerturbada.corredoresWave.size();
            denominador = solucaoPerturbada.corredoresWave.size();
        }

        // Se o numerador for positivo, atualizar a melhor solução
        if (numerador > 0) {
            melhorSolucao = solucaoPerturbada;
        } else {
            // Ajustar lambda
            lambda = calcularValorObjetivo(deposito, backlog, melhorSolucao);
        }
    }

    return melhorSolucao;
}

double calcularValorObjetivo(const Deposito& deposito, const Backlog& backlog, const Solucao& solucao) {
    if (solucao.corredoresWave.empty()) {
        return 0.0;
    }

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
Solucao ajustarSolucao(const Deposito& deposito, const Backlog& backlog, Solucao solucao) {
    // Inicializar o estoque disponível com base nos corredores selecionados
    std::unordered_map<int, int> estoqueDisponivel;
    for (int corredorId : solucao.corredoresWave) {
        for (const auto& [itemId, quantidade] : deposito.corredor[corredorId]) {
            estoqueDisponivel[itemId] += quantidade;
        }
    }

    // Identificar pedidos com estoque insuficiente
    std::vector<int> pedidosParaRemover;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
            if (estoqueDisponivel[itemId] < quantidadeSolicitada) {
                pedidosParaRemover.push_back(pedidoId);
                break; // Se um item estiver insuficiente, remover o pedido
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
        std::vector<int> pedidosCandidatos;
        for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
            if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) == solucao.pedidosWave.end()) {
                pedidosCandidatos.push_back(pedidoId);
            }
        }

        std::shuffle(pedidosCandidatos.begin(), pedidosCandidatos.end(),
                    std::mt19937(std::random_device()()));

        for (int pedidoId : pedidosCandidatos) {
            int unidadesPedido = 0;
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }

            // Verificar se adicionar este pedido não viola a restrição de estoque
            bool estoqueSuficiente = true;
            for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                if (estoqueDisponivel[itemId] < quantidadeSolicitada) {
                    estoqueSuficiente = false;
                    break;
                }
            }

            if (estoqueSuficiente && totalUnidades + unidadesPedido <= backlog.wave.UB) {
                solucao.pedidosWave.push_back(pedidoId);
                totalUnidades += unidadesPedido;

                // Atualizar o estoque disponível
                for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                    estoqueDisponivel[itemId] -= quantidadeSolicitada;
                }
            }

            if (totalUnidades >= backlog.wave.LB) {
                break;
            }
        }
    } else if (totalUnidades > backlog.wave.UB) { // Se o total de unidades for superior a UB, remover pedidos até atingir UB
        std::vector<int> pedidosParaRemover;
        for (int pedidoId : solucao.pedidosWave) {
            pedidosParaRemover.push_back(pedidoId);
        }

        std::shuffle(pedidosParaRemover.begin(), pedidosParaRemover.end(),
                    std::mt19937(std::random_device()()));

        for (int pedidoId : pedidosParaRemover) {
            int unidadesPedido = 0;
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }

            if (totalUnidades - unidadesPedido >= backlog.wave.LB) {
                solucao.pedidosWave.erase(
                    std::remove(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId),
                    solucao.pedidosWave.end());
                totalUnidades -= unidadesPedido;

                // Atualizar o estoque disponível
                for (const auto& [itemId, quantidadeSolicitada] : backlog.pedido[pedidoId]) {
                    estoqueDisponivel[itemId] += quantidadeSolicitada;
                }
            }

            if (totalUnidades <= backlog.wave.UB) {
                break;
            }
        }
    }

    // Recalcular os corredores necessários
    std::unordered_set<int> corredoresNecessarios;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            for (int corredorId = 0; corredorId < deposito.numCorredores; corredorId++) {
                if (deposito.corredor[corredorId].count(itemId)) {
                    corredoresNecessarios.insert(corredorId);
                    break;
                }
            }
        }
    }
    solucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());

    // Recalcular o valor objetivo
    solucao.valorObjetivo = calcularValorObjetivo(deposito, backlog, solucao);

    return solucao;
}