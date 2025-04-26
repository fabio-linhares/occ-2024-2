#include "modules/solucao_inicial.h"
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <iostream>
#include <thread>
#include <vector>
#include <execution>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

// Implementação de ThreadPool para reutilização de threads
class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

bool gerarSolucaoInicial(const Warehouse& warehouse, Solution& solution) {
    std::cout << "    Construindo solução inicial com paralelismo otimizado..." << std::endl;
    
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    int totalItemsAtual = 0;
    
    // Configurar ThreadPool para o número de núcleos disponíveis
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Fallback
    std::cout << "Utilizando " << numThreads << " threads no pool" << std::endl;
    
    ThreadPool pool(numThreads);
    
    // 1. BUSCA PARALELA DE MÁXIMOS COM WORK STEALING
    std::vector<std::atomic<double>> maxEfficiencyPerThread(numThreads);
    std::vector<std::atomic<double>> maxDensityPerThread(numThreads);
    
    for (unsigned int i = 0; i < numThreads; i++) {
        maxEfficiencyPerThread[i] = 0.0;
        maxDensityPerThread[i] = 0.0;
    }
    
    // Estratégia de work stealing para melhor balanceamento
    int chunkSize = std::max<int>(1, warehouse.numOrders / (numThreads * 4)); // Chunks menores para balanceamento
    std::atomic<int> nextChunkStart(0);

    std::vector<std::future<void>> maxFutures;
    
    for (unsigned int t = 0; t < numThreads; t++) {
        maxFutures.push_back(pool.enqueue([&, t]() {
            while (true) {
                // Work stealing: pegar próximo chunk disponível
                int start = nextChunkStart.fetch_add(chunkSize);
                if (start >= warehouse.numOrders) break;
                
                int end = std::min(start + chunkSize, warehouse.numOrders);
                
                double threadMaxEff = maxEfficiencyPerThread[t];
                double threadMaxDen = maxDensityPerThread[t];
                
                for (int i = start; i < end; i++) {
                    threadMaxEff = std::max(threadMaxEff, aux.weights.orderEfficiencyRatio[i]);
                    threadMaxDen = std::max(threadMaxDen, aux.weights.orderUnitDensity[i]);
                }
                
                maxEfficiencyPerThread[t] = threadMaxEff;
                maxDensityPerThread[t] = threadMaxDen;
            }
        }));
    }
    
    // Aguardar conclusão
    for (auto& future : maxFutures) {
        future.get();
    }
    
    // Combinar resultados
    double maxEfficiency = 0.0;
    double maxDensity = 0.0;
    
    for (unsigned int i = 0; i < numThreads; i++) {
        maxEfficiency = std::max(maxEfficiency, static_cast<double>(maxEfficiencyPerThread[i]));
        maxDensity = std::max(maxDensity, static_cast<double>(maxDensityPerThread[i]));
    }
    
    // 2. CÁLCULO PARALELO DE SCORES COM BALANCEAMENTO DINÂMICO
    std::vector<std::pair<int, double>> combinedScores(warehouse.numOrders);
    std::atomic<int> nextScoreChunk(0);
    
    std::vector<std::future<void>> scoreFutures;
    
    for (unsigned int t = 0; t < numThreads; t++) {
        scoreFutures.push_back(pool.enqueue([&]() {
            while (true) {
                int start = nextScoreChunk.fetch_add(chunkSize);
                if (start >= warehouse.numOrders) break;
                
                int end = std::min(start + chunkSize, warehouse.numOrders);
                
                for (int i = start; i < end; i++) {
                    double normEfficiency = maxEfficiency > 0 ? 
                        aux.weights.orderEfficiencyRatio[i] / maxEfficiency : 0;
                    double normDensity = maxDensity > 0 ? 
                        aux.weights.orderUnitDensity[i] / maxDensity : 0;
                    
                    // Balanceamento entre eficiência e quantidade (ajustável)
                    double score = 0.7 * normEfficiency + 0.3 * normDensity;
                    combinedScores[i] = {i, score};
                }
            }
        }));
    }
    
    for (auto& future : scoreFutures) {
        future.get();
    }
    
    // Ordenação paralela
    std::sort(std::execution::par_unseq, combinedScores.begin(), combinedScores.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 3. PRIMEIRA FASE: PEDIDOS EFICIENTES (OTIMIZADA COM PARALELISMO)
    std::cout << "Fase 1: Adicionando pedidos eficientes com simulação paralela..." << std::endl;

    // 3.1 Simulação paralela para identificar candidatos viáveis
    std::vector<std::pair<int, int>> candidateOrders;
    std::mutex candidatesMutex;
    std::atomic<int> totalSimulatedItems(0);
    const int batchSize = 100; // Ajustar conforme tamanho do problema

    // Dividir os pedidos ordenados em chunks para processamento paralelo
    int numBatches = (combinedScores.size() + batchSize - 1) / batchSize;
    std::vector<std::future<void>> simulationFutures;

    for (int b = 0; b < numBatches && totalSimulatedItems < warehouse.LB; b++) {
        simulationFutures.push_back(pool.enqueue([&, b]() {
            // Copiar estado local para simulação
            int localTotalItems = totalItemsAtual;
            std::vector<std::pair<int, int>> localCandidates; // orderIdx, addedItems
            
            // Determinar intervalo do lote
            int start = b * batchSize;
            int end = std::min(start + batchSize, static_cast<int>(combinedScores.size()));
            
            // Simular adição de pedidos neste lote
            for (int i = start; i < end; i++) {
                int orderIdx = combinedScores[i].first;
                double score = combinedScores[i].second;
                
                if (score <= 0) continue;
                
                // Se este pedido ainda cabe no limite superior
                int orderItems = aux.totalItemsPerOrder[orderIdx];
                if (localTotalItems + orderItems <= warehouse.UB) {
                    localCandidates.push_back({orderIdx, orderItems});
                    localTotalItems += orderItems;
                }
                
                // Se simulação local já atingiu LB, parar
                if (localTotalItems >= warehouse.LB) break;
            }
            
            // Adicionar candidatos locais ao pool global
            if (!localCandidates.empty()) {
                std::lock_guard<std::mutex> lock(candidatesMutex);
                candidateOrders.insert(candidateOrders.end(), localCandidates.begin(), localCandidates.end());
                // Atualizar contador atômico do simulado total
                totalSimulatedItems.fetch_add(localTotalItems - totalItemsAtual);
            }
        }));
    }

    // Aguardar simulações terminarem
    for (auto& future : simulationFutures) {
        future.get();
    }

    // 3.2 Ordenar candidatos por score (manter ordem original dos scores)
    std::sort(candidateOrders.begin(), candidateOrders.end(), 
        [&combinedScores](const auto& a, const auto& b) {
            // Encontrar posições nos scores originais
            auto posA = std::find_if(combinedScores.begin(), combinedScores.end(), 
                [&a](const auto& p) { return p.first == a.first; });
            auto posB = std::find_if(combinedScores.begin(), combinedScores.end(), 
                [&b](const auto& p) { return p.first == b.first; });
            return std::distance(combinedScores.begin(), posA) < std::distance(combinedScores.begin(), posB);
        });

    // 3.3 Aplicar candidatos até atingir limite
    for (const auto& [orderIdx, orderItems] : candidateOrders) {
        if (totalItemsAtual + orderItems <= warehouse.UB) {
            solution.addOrder(orderIdx, warehouse);
            totalItemsAtual = solution.getTotalItems();
            
            //std::cout << "  Adicionado pedido #" << orderIdx 
            //          << " (Items: " << orderItems << ", Total: " << totalItemsAtual << ")" << std::endl;
        }
        
        if (totalItemsAtual >= warehouse.LB)
            break;
    }
    
    // 4. SEGUNDA FASE: DIVERSIDADE DE ITENS COM PARALELISMO
    if (totalItemsAtual < warehouse.LB) {
        std::cout << "Fase 2: Adicionando pedidos para aumentar diversidade..." << std::endl;
        
        // Coletar itens já cobertos
        std::unordered_set<int> coveredItems;
        std::mutex coveredMutex;
        
        for (int orderIdx : solution.getSelectedOrders()) {
            for (int itemId : aux.itemsInOrder[orderIdx]) {
                coveredItems.insert(itemId);
            }
        }
        
        // Calcular score de diversidade para todos os pedidos em paralelo
        std::vector<std::pair<int, int>> diversityScores;
        std::mutex scoresMutex;
        std::atomic<int> nextDiversityChunk(0);
        
        std::vector<std::future<void>> diversityFutures;
        
        for (unsigned int t = 0; t < numThreads; t++) {
            diversityFutures.push_back(pool.enqueue([&]() {
                std::vector<std::pair<int, int>> localScores;
                
                while (true) {
                    int start = nextDiversityChunk.fetch_add(chunkSize);
                    if (start >= warehouse.numOrders) break;
                    
                    int end = std::min(start + chunkSize, warehouse.numOrders);
                    
                    for (int orderIdx = start; orderIdx < end; orderIdx++) {
                        // Pular pedidos já incluídos
                        if (std::find(solution.getSelectedOrders().begin(), 
                                    solution.getSelectedOrders().end(), orderIdx) != solution.getSelectedOrders().end())
                            continue;
                        
                        // Contar novos itens
                        int novosItens = 0;
                        for (int itemId : aux.itemsInOrder[orderIdx]) {
                            std::lock_guard<std::mutex> lock(coveredMutex);
                            if (coveredItems.count(itemId) == 0) novosItens++;
                        }
                        
                        if (novosItens > 0) {
                            localScores.push_back({orderIdx, novosItens});
                        }
                    }
                }
                
                // Mesclar resultados locais com o vetor global
                if (!localScores.empty()) {
                    std::lock_guard<std::mutex> lock(scoresMutex);
                    diversityScores.insert(diversityScores.end(), localScores.begin(), localScores.end());
                }
            }));
        }
        
        for (auto& future : diversityFutures) {
            future.get();
        }
        
        // Ordenar por novidade (mais itens novos primeiro)
        std::sort(std::execution::par, diversityScores.begin(), diversityScores.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Adicionar pedidos por diversidade
        for (const auto& [orderIdx, novosItens] : diversityScores) {
            if (totalItemsAtual + aux.totalItemsPerOrder[orderIdx] <= warehouse.UB) {
                solution.addOrder(orderIdx, warehouse);
                totalItemsAtual = solution.getTotalItems();
                
                // Atualizar itens cobertos (não precisa de mutex aqui, já estamos sequenciais)
                for (int itemId : aux.itemsInOrder[orderIdx]) {
                    coveredItems.insert(itemId);
                }
                
                std::cout << "  Adicionado pedido #" << orderIdx 
                          << " (Novos itens: " << novosItens << ", Total: " << totalItemsAtual << ")" << std::endl;
            }
            
            if (totalItemsAtual >= warehouse.LB)
                break;
        }
    }
    
    // Verificar factibilidade
    bool factivel = (totalItemsAtual >= warehouse.LB && totalItemsAtual <= warehouse.UB);
    solution.setFeasible(factivel);
    
    std::cout << "Solução inicial construída:" << std::endl;
    std::cout << "  Pedidos selecionados: " << solution.getSelectedOrders().size() << std::endl;
    std::cout << "  Corredores visitados: " << solution.getVisitedCorridors().size() << std::endl;
    std::cout << "  Total de itens: " << totalItemsAtual << " (LB: " << warehouse.LB << ", UB: " << warehouse.UB << ")" << std::endl;
    std::cout << "  Função objetivo: " << solution.getObjectiveValue() << std::endl;
    std::cout << "  Factibilidade: " << (factivel ? "SIM" : "NÃO") << std::endl;
    
    return factivel;
}