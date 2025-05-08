#include "otimizador_paralelo.h"
#include <algorithm>
#include <chrono>
#include <iostream>

OtimizadorParalelo::OtimizadorParalelo(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador,
    const AnalisadorRelevancia& analisador,
    unsigned int numThreads
) : 
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    analisador_(analisador),
    terminar_(false)
{
    // Determinar número de threads - usar hardware_concurrency ou 4 como fallback
    numThreads_ = numThreads;
    if (numThreads_ == 0) {
        numThreads_ = std::thread::hardware_concurrency();
        if (numThreads_ == 0) {
            numThreads_ = 4; // Fallback se hardware_concurrency falhar
        }
        // Remover esta limitação abaixo para usar todas as threads disponíveis
        // numThreads_ = std::min(numThreads_, 4u); // Limitar a 4 threads por padrão
    }
    
    // Você também pode querer remover ou ajustar esta limitação
    if (backlog.numPedidos < 100) {
        numThreads_ = std::min(numThreads_, 2u); // Limitar a 2 threads para instâncias pequenas
    }
}

Solucao OtimizadorParalelo::otimizar(const Solucao& solucaoInicial) {
    // Inicializar melhor solução global com a solução inicial
    Solucao melhorSolucaoGlobal = solucaoInicial;
    
    // Vetor para armazenar as threads
    std::vector<std::thread> threads;
    
    // Vetor para futures dos resultados
    std::vector<std::future<Solucao>> futures;
    
    // Iniciar threads de otimização
    for (unsigned int i = 0; i < numThreads_; i++) {
        // Criar promessa e future para este thread
        std::promise<Solucao> promessa;
        futures.push_back(promessa.get_future());
        
        // Criar uma cópia da solução inicial com pequena perturbação
        Solucao solucaoThread = solucaoInicial;
        
        // Iniciar thread de otimização
        threads.push_back(std::thread(
            &OtimizadorParalelo::threadOtimizacao, 
            this, 
            i, 
            solucaoThread, 
            std::ref(melhorSolucaoGlobal), 
            std::move(promessa)
        ));
    }
    
    // Aguardar todas as threads terminarem
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    
    // Obter a melhor solução das futures
    for (auto& fut : futures) {
        Solucao solucaoThread = fut.get();
        if (solucaoThread.valorObjetivo > melhorSolucaoGlobal.valorObjetivo) {
            melhorSolucaoGlobal = solucaoThread;
        }
    }
    
    return melhorSolucaoGlobal;
}

void OtimizadorParalelo::threadOtimizacao(
    int threadId,
    Solucao solucaoInicial,
    Solucao& melhorSolucaoGlobal,
    std::promise<Solucao> resultados
) {
    // Configurar gerador de números aleatórios para esta thread
    std::random_device rd;
    std::mt19937 gerador(rd() + threadId); // Seed único por thread
    
    // Solução atual e melhor solução local para esta thread
    Solucao solucaoAtual = solucaoInicial;
    Solucao melhorSolucaoLocal = solucaoInicial;
    
    // Parâmetros para algoritmo de Dinkelbach
    double lambda = solucaoAtual.valorObjetivo;
    int iteracoesSemMelhoria = 0;
    const int MAX_ITERACOES_SEM_MELHORIA = 1000;
    const int MAX_ITERACOES = 10000;
    
    // Tempo de início para controle de tempo
    auto inicioTempo = std::chrono::high_resolution_clock::now();
    
    // Loop principal de otimização
    for (int iter = 0; iter < MAX_ITERACOES; iter++) {
        // Verificar se é hora de terminar
        if (terminar_.load()) break;
        
        // Verificar tempo limite
        auto tempoAtual = std::chrono::high_resolution_clock::now();
        double segundosDecorridos = std::chrono::duration<double>(tempoAtual - inicioTempo).count();
        if (segundosDecorridos > tempoMaximo_) break;
        
        // Perturbar solução atual
        Solucao solucaoPerturbada = perturbarSolucaoLocal(solucaoAtual, gerador);
        
        // Extrair valores para cálculo do Dinkelbach
        double valorF = 0; // Numerador (unidades)
        for (int pedidoId : solucaoPerturbada.pedidosWave) {
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                valorF += quantidade;
            }
        }
        
        double valorG = solucaoPerturbada.corredoresWave.size(); // Denominador (corredores)
        
        // Calcular novo lambda e valor objetivo
        double novoLambda = valorG > 0 ? valorF / valorG : 0;
        solucaoPerturbada.valorObjetivo = novoLambda;
        
        // Atualizar lambda se melhorou
        if (novoLambda > lambda) {
            lambda = novoLambda;
            solucaoAtual = solucaoPerturbada;
            iteracoesSemMelhoria = 0;
            
            // Atualizar melhor solução local
            if (novoLambda > melhorSolucaoLocal.valorObjetivo) {
                melhorSolucaoLocal = solucaoPerturbada;
                
                // Verificar se devemos atualizar melhor global
                {
                    std::lock_guard<std::mutex> lock(melhorSolucaoMutex_);
                    if (melhorSolucaoLocal.valorObjetivo > melhorSolucaoGlobal.valorObjetivo) {
                        melhorSolucaoGlobal = melhorSolucaoLocal;
                    }
                }
            }
        } else {
            iteracoesSemMelhoria++;
        }
        
        // Comunicação entre threads a cada N iterações
        if (iter % iteracoesComunicacao_ == 0) {
            std::lock_guard<std::mutex> lock(melhorSolucaoMutex_);
            
            // Verificar melhor global
            if (melhorSolucaoGlobal.valorObjetivo > melhorSolucaoLocal.valorObjetivo) {
                // Com certa probabilidade, adotar a melhor global
                if (gerador() % 4 == 0) { // 25% de chance
                    solucaoAtual = melhorSolucaoGlobal;
                    lambda = solucaoAtual.valorObjetivo;
                    iteracoesSemMelhoria = 0;
                }
            }
        }
        
        // Reinicialização se estagnado
        if (iteracoesSemMelhoria >= MAX_ITERACOES_SEM_MELHORIA) {
            solucaoAtual = gerarSolucaoAleatoriaLocal(gerador);
            lambda = solucaoAtual.valorObjetivo;
            iteracoesSemMelhoria = 0;
        }
    }
    
    // Atualizar a melhor solução global uma última vez
    {
        std::lock_guard<std::mutex> lock(melhorSolucaoMutex_);
        if (melhorSolucaoLocal.valorObjetivo > melhorSolucaoGlobal.valorObjetivo) {
            melhorSolucaoGlobal = melhorSolucaoLocal;
        }
    }
    
    // Devolver o resultado via promessa
    resultados.set_value(melhorSolucaoLocal);
}

Solucao OtimizadorParalelo::perturbarSolucaoLocal(const Solucao& solucao, std::mt19937& gerador) {
    Solucao novasolucao = solucao;
    
    // Remover alguns pedidos aleatoriamente (30% a 50% dos pedidos)
    if (!novasolucao.pedidosWave.empty()) {
        std::uniform_int_distribution<> distRemover(
            std::max(1, (int)(novasolucao.pedidosWave.size() * 0.3)), 
            std::max(1, (int)(novasolucao.pedidosWave.size() * 0.5))
        );
        
        int numRemover = distRemover(gerador);
        std::shuffle(novasolucao.pedidosWave.begin(), novasolucao.pedidosWave.end(), gerador);
        if (numRemover < novasolucao.pedidosWave.size()) {
            novasolucao.pedidosWave.resize(novasolucao.pedidosWave.size() - numRemover);
        }
    }
    
    // Adicionar novos pedidos de forma gulosa
    std::vector<std::pair<int, double>> candidatos;
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        // Verificar se pedido já está incluído
        if (std::find(novasolucao.pedidosWave.begin(), novasolucao.pedidosWave.end(), pedidoId) 
            != novasolucao.pedidosWave.end()) {
            continue;
        }
        
        // Verificar disponibilidade
        if (!verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
            continue;
        }
        
        // Calcular eficiência
        int unidades = 0;
        std::unordered_set<int> novosCorredores;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            unidades += quantidade;
            
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                if (std::find(novasolucao.corredoresWave.begin(), novasolucao.corredoresWave.end(), corredorId)
                    == novasolucao.corredoresWave.end()) {
                    novosCorredores.insert(corredorId);
                }
            }
        }
        
        double eficiencia = novosCorredores.empty() ? unidades : static_cast<double>(unidades) / novosCorredores.size();
        candidatos.emplace_back(pedidoId, eficiencia);
    }
    
    // Ordenar candidatos por eficiência
    std::sort(candidatos.begin(), candidatos.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Selecionar aleatoriamente entre os melhores candidatos (top 30%)
    int numConsiderar = std::max(1, (int)(candidatos.size() * 0.3));
    std::uniform_int_distribution<> distSeletor(0, numConsiderar - 1);
    
    // Adicionar até 5 pedidos aleatoriamente dos melhores
    int numAdicionar = std::min(5, (int)candidatos.size());
    for (int i = 0; i < numAdicionar; i++) {
        int idx = distSeletor(gerador);
        idx = std::min(idx, (int)candidatos.size() - 1);
        
        int pedidoId = candidatos[idx].first;
        novasolucao.pedidosWave.push_back(pedidoId);
        
        // Remover o candidato usado
        if (!candidatos.empty()) {
            candidatos.erase(candidatos.begin() + idx);
        }
    }
    
    // Recalcular corredores
    std::unordered_set<int> corredoresNecessarios;
    
    for (int pedidoId : novasolucao.pedidosWave) {
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresNecessarios.insert(corredorId);
            }
        }
    }
    
    novasolucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
    
    // Calcular valor objetivo
    double valorF = 0;
    for (int pedidoId : novasolucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            valorF += quantidade;
        }
    }
    
    double valorG = novasolucao.corredoresWave.size();
    if (valorG > 0) {
        novasolucao.valorObjetivo = valorF / valorG;
    } else {
        novasolucao.valorObjetivo = 0;
    }
    
    return novasolucao;
}

Solucao OtimizadorParalelo::gerarSolucaoAleatoriaLocal(std::mt19937& gerador) {
    Solucao novasolucao;
    
    // Número de pedidos a selecionar
    std::uniform_int_distribution<> distNumPedidos(1, std::min(20, backlog_.numPedidos));
    int numPedidosAlvo = distNumPedidos(gerador);
    
    // Selecionar pedidos aleatoriamente
    std::uniform_int_distribution<> distPedidos(0, backlog_.numPedidos - 1);
    std::unordered_set<int> pedidosSelecionados;
    
    while (pedidosSelecionados.size() < numPedidosAlvo) {
        int pedidoId = distPedidos(gerador);
        
        // Verificar disponibilidade
        if (verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
            pedidosSelecionados.insert(pedidoId);
        }
    }
    
    // Converter para vetor
    novasolucao.pedidosWave.assign(pedidosSelecionados.begin(), pedidosSelecionados.end());
    
    // Determinar corredores necessários
    std::unordered_set<int> corredoresNecessarios;
    for (int pedidoId : novasolucao.pedidosWave) {
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresNecessarios.insert(corredorId);
            }
        }
    }
    
    novasolucao.corredoresWave.assign(corredoresNecessarios.begin(), corredoresNecessarios.end());
    
    // Calcular valor objetivo
    double valorF = 0; // Numerador (unidades)
    for (int pedidoId : novasolucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            valorF += quantidade;
        }
    }
    
    double valorG = novasolucao.corredoresWave.size(); // Denominador (corredores)
    if (valorG > 0) {
        novasolucao.valorObjetivo = valorF / valorG;
    } else {
        novasolucao.valorObjetivo = 0;
    }
    
    return novasolucao;
}

void OtimizadorParalelo::setTempoMaximo(double segundos) {
    tempoMaximo_ = segundos;
}

void OtimizadorParalelo::setFrequenciaComunicacao(int iteracoes) {
    iteracoesComunicacao_ = iteracoes;
}