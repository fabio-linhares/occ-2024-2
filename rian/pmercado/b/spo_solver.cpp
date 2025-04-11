#include "spo_solver.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <numeric>
#include <limits>

// Estrutura otimizada de dados
struct OptimizedDataStructures {
    std::unordered_map<int, std::vector<int>> itemToAisles;          // Mapa de item -> corredores
    std::vector<std::unordered_map<int, int>> aisleToItems;          // Vetor de corredor -> (item, quantidade)
    std::vector<std::unordered_map<int, int>> orderToItems;          // Vetor de pedido -> (item, quantidade)
    std::unordered_map<int, int> totalItemsPerOrder;                 // Mapa de pedido -> total de itens
    std::unordered_map<int, std::set<int>> requiredAislesPerOrder;   // Mapa de pedido -> corredores necessários
};

OptimizedDataStructures optimizedDS;

// Função para limpar as estruturas de dados entre instâncias
void cleanupDataStructures() {
    optimizedDS.itemToAisles.clear();
    optimizedDS.aisleToItems.clear();
    optimizedDS.orderToItems.clear();
    optimizedDS.totalItemsPerOrder.clear();
    optimizedDS.requiredAislesPerOrder.clear();
}

// Inicialização das estruturas de dados otimizadas
void initializeOptimizedDataStructures(const Instance& instance, const Config& config) {
    std::cout << "Inicializando otimizações de estruturas de dados..." << std::endl;
    
    // Marcar o parâmetro como usado para evitar o aviso
    (void)config;
    
    // Limpar estruturas existentes para evitar vazamentos
    cleanupDataStructures();
    
    // Redimensionar vetores para o número correto de elementos
    optimizedDS.aisleToItems.resize(instance.aisles.size());
    optimizedDS.orderToItems.resize(instance.orders.size());
    
    // Preencher o mapeamento item -> corredor de forma segura
    for (size_t aisleIdx = 0; aisleIdx < instance.aisles.size(); ++aisleIdx) {
        const Aisle& aisle = instance.aisles[aisleIdx];
        for (const auto& [itemId, quantity] : aisle.itemQuantities) {
            // Usar mapa em vez de acesso direto por índice
            optimizedDS.itemToAisles[itemId].push_back(aisle.id);
            optimizedDS.aisleToItems[aisleIdx][itemId] = quantity;
        }
    }
    
    // Preencher o mapeamento pedido -> item de forma segura
    for (size_t orderIdx = 0; orderIdx < instance.orders.size(); ++orderIdx) {
        const Order& order = instance.orders[orderIdx];
        int totalItems = 0;
        std::set<int> requiredAisles;
        
        for (const Item& item : order.items) {
            optimizedDS.orderToItems[orderIdx][item.id] += item.quantity;
            totalItems += item.quantity;
            
            // Identificar corredores necessários para este item
            for (int aisleId : optimizedDS.itemToAisles[item.id]) {
                requiredAisles.insert(aisleId);
            }
        }
        
        optimizedDS.totalItemsPerOrder[order.id] = totalItems;
        optimizedDS.requiredAislesPerOrder[order.id] = requiredAisles;
    }
    
    std::cout << "Estruturas de dados otimizadas inicializadas com sucesso." << std::endl;
}

// Modificar a função generateInitialSolution para adicionar uma estratégia de último recurso
Solution generateInitialSolution(const Instance& instance, const Config& config) {
    // Marcar o parâmetro para evitar warning
    (void)config;
    
    std::cout << "Gerando solução inicial robusta..." << std::endl;
    
    Solution solution;
    
    // Estrutura para acompanhar disponibilidade restante de itens
    std::unordered_map<int, int> itemAvailability;
    for (const auto& aisle : instance.aisles) {
        for (const auto& [itemId, quantity] : aisle.itemQuantities) {
            itemAvailability[itemId] += quantity;
        }
    }
    
    // Calcular eficiência de cada pedido (itens/corredores)
    struct OrderEfficiency {
        int id;              // ID real do pedido
        int index;           // Índice no vetor de pedidos
        double efficiency;   // Razão itens/corredores
        int totalItems;      // Total de itens
        std::set<int> requiredAisles; // Corredores necessários
    };
    
    std::vector<OrderEfficiency> ordersByEfficiency;
    
    for (size_t i = 0; i < instance.orders.size(); ++i) {
        const Order& order = instance.orders[i];
        
        // Verificar se todos os itens do pedido estão disponíveis em quantidade suficiente
        bool canFulfill = true;
        std::unordered_map<int, int> itemsNeeded;
        
        for (const Item& item : order.items) {
            itemsNeeded[item.id] += item.quantity;
            if (itemsNeeded[item.id] > itemAvailability[item.id]) {
                canFulfill = false;
                break;
            }
        }
        
        if (canFulfill && !optimizedDS.requiredAislesPerOrder[i].empty()) {
            OrderEfficiency orderInfo;
            orderInfo.id = order.id;
            orderInfo.index = i;
            orderInfo.totalItems = optimizedDS.totalItemsPerOrder[i];
            orderInfo.requiredAisles = optimizedDS.requiredAislesPerOrder[i];
            orderInfo.efficiency = static_cast<double>(orderInfo.totalItems) / orderInfo.requiredAisles.size();
            
            ordersByEfficiency.push_back(orderInfo);
        }
    }
    
    // Ordenar pedidos por eficiência (maior para menor)
    std::sort(ordersByEfficiency.begin(), ordersByEfficiency.end(),
              [](const OrderEfficiency& a, const OrderEfficiency& b) {
                  return a.efficiency > b.efficiency;
              });
    
    // Construir solução gulosa
    int totalItems = 0;
    std::set<int> selectedAisles;
    std::unordered_map<int, int> remainingAvailability = itemAvailability;
    
    for (const auto& order : ordersByEfficiency) {
        // Verificar se já atingimos o LB
        if (totalItems >= instance.LB) {
            break;
        }
        
        // Verificar se adicionar este pedido não ultrapassará o UB
        if (totalItems + order.totalItems > instance.UB) {
            continue;
        }
        
        // Verificar se ainda há disponibilidade para os itens deste pedido
        bool canAdd = true;
        std::unordered_map<int, int> itemsNeeded;
        
        for (const auto& [itemId, quantity] : optimizedDS.orderToItems[order.index]) {
            itemsNeeded[itemId] += quantity;
            if (itemsNeeded[itemId] > remainingAvailability[itemId]) {
                canAdd = false;
                break;
            }
        }
        
        if (canAdd) {
            // Adicionar pedido à solução
            solution.selectedOrders.push_back(order.id);
            totalItems += order.totalItems;
            
            // Atualizar disponibilidade restante
            for (const auto& [itemId, quantity] : itemsNeeded) {
                remainingAvailability[itemId] -= quantity;
            }
            
            // Adicionar corredores necessários
            for (int aisleId : order.requiredAisles) {
                selectedAisles.insert(aisleId);
            }
        }
    }
    
    // Se não conseguimos atingir o LB, tentar adicionar mais pedidos
    if (totalItems < instance.LB) {
        std::cout << "AVISO: Não foi possível atingir o limite inferior com pedidos eficientes." << std::endl;
        std::cout << "Tentando estratégia alternativa..." << std::endl;
        
        // Ordenar por número total de itens (maior para menor)
        std::sort(ordersByEfficiency.begin(), ordersByEfficiency.end(),
                  [](const OrderEfficiency& a, const OrderEfficiency& b) {
                      return a.totalItems > b.totalItems;
                  });
        
        // Reiniciar a solução
        solution.selectedOrders.clear();
        selectedAisles.clear();
        totalItems = 0;
        remainingAvailability = itemAvailability;
        
        // Tentar novamente com foco em itens totais
        for (const auto& order : ordersByEfficiency) {
            if (totalItems >= instance.LB) {
                break;
            }
            
            if (totalItems + order.totalItems > instance.UB) {
                continue;
            }
            
            bool canAdd = true;
            std::unordered_map<int, int> itemsNeeded;
            
            for (const auto& [itemId, quantity] : optimizedDS.orderToItems[order.index]) {
                itemsNeeded[itemId] += quantity;
                if (itemsNeeded[itemId] > remainingAvailability[itemId]) {
                    canAdd = false;
                    break;
                }
            }
            
            if (canAdd) {
                solution.selectedOrders.push_back(order.id);
                totalItems += order.totalItems;
                
                for (const auto& [itemId, quantity] : itemsNeeded) {
                    remainingAvailability[itemId] -= quantity;
                }
                
                for (int aisleId : order.requiredAisles) {
                    selectedAisles.insert(aisleId);
                }
            }
        }
    }
    
    // Se ainda não conseguimos atingir o LB, tentar estratégia de último recurso
    if (totalItems < instance.LB || solution.visitedAisles.empty()) {
        std::cout << "AVISO: Estratégias anteriores falharam. Tentando estratégia de último recurso..." << std::endl;
        
        // Selecionar pelo menos um pedido e garantir que os corredores sejam adicionados
        solution.selectedOrders.clear();
        selectedAisles.clear();
        totalItems = 0;
        remainingAvailability = itemAvailability;
        
        // Selecionar pedidos um a um até atingir o LB ou não haver mais pedidos viáveis
        for (size_t i = 0; i < instance.orders.size(); ++i) {
            const Order& order = instance.orders[i];
            std::unordered_map<int, int> itemsNeeded;
            bool canAdd = true;
            
            // Verificar disponibilidade de itens
            for (const Item& item : order.items) {
                itemsNeeded[item.id] += item.quantity;
                if (itemsNeeded[item.id] > remainingAvailability[item.id]) {
                    canAdd = false;
                    break;
                }
            }
            
            if (canAdd) {
                // Adicionar pedido à solução
                solution.selectedOrders.push_back(order.id);
                
                // Atualizar total de itens
                for (const Item& item : order.items) {
                    totalItems += item.quantity;
                    remainingAvailability[item.id] -= item.quantity;
                }
                
                // Adicionar todos os corredores necessários para este pedido
                for (const Item& item : order.items) {
                    for (int aisleId : optimizedDS.itemToAisles[item.id]) {
                        selectedAisles.insert(aisleId);
                    }
                }
                
                // Se atingimos o LB, podemos parar
                if (totalItems >= instance.LB) {
                    break;
                }
            }
        }
    }
    
    // Atualizar corredores visitados
    solution.visitedAisles.assign(selectedAisles.begin(), selectedAisles.end());
    
    std::cout << "Solução inicial: " << solution.selectedOrders.size() 
              << " pedidos, " << totalItems << " itens, " 
              << solution.visitedAisles.size() << " corredores." << std::endl;
    
    return solution;
}

// Melhorar a função updateVisitedAisles para garantir disponibilidade completa
void updateVisitedAisles(Solution& solution, const Instance& instance) {
    // Calcular a demanda total por item
    std::unordered_map<int, int> totalDemand;
    std::unordered_map<int, bool> itemNeeded;
    
    for (int orderId : solution.selectedOrders) {
        for (const auto& order : instance.orders) {
            if (order.id == orderId) {
                for (const Item& item : order.items) {
                    totalDemand[item.id] += item.quantity;
                    itemNeeded[item.id] = true;
                }
                break;
            }
        }
    }
    
    // Garantir que todos os itens necessários tenham pelo menos um corredor
    std::set<int> selectedAisles(solution.visitedAisles.begin(), solution.visitedAisles.end());
    
    // Primeiro passo: garantir que cada item necessário tenha pelo menos um corredor
    for (const auto& [itemId, needed] : itemNeeded) {
        if (!needed) continue;
        
        // Verificar se já temos cobertura para este item
        bool covered = false;
        for (int aisleId : selectedAisles) {
            const Aisle* aisle = nullptr;
            for (const auto& a : instance.aisles) {
                if (a.id == aisleId) {
                    aisle = &a;
                    break;
                }
            }
            
            if (aisle && aisle->itemQuantities.count(itemId) > 0) {
                covered = true;
                break;
            }
        }
        
        // Se não estiver coberto, adicionar pelo menos um corredor com este item
        if (!covered) {
            for (const auto& aisle : instance.aisles) {
                if (aisle.itemQuantities.count(itemId) > 0) {
                    selectedAisles.insert(aisle.id);
                    break;
                }
            }
        }
    }
    
    // Estrutura para analisar corredores
    struct AisleCoverage {
        int id;
        int uniqueItems;
        int totalCoverage;
        double score;
        std::map<int, int> coverageMap; // item -> quantidade
    };
    
    std::vector<AisleCoverage> aisles;
    
    // Calcular a contribuição de cada corredor
    for (const auto& aisle : instance.aisles) {
        AisleCoverage coverage;
        coverage.id = aisle.id;
        coverage.uniqueItems = 0;
        coverage.totalCoverage = 0;
        
        for (const auto& [itemId, quantity] : aisle.itemQuantities) {
            if (totalDemand.count(itemId) && totalDemand[itemId] > 0) {
                coverage.uniqueItems++;
                int covered = std::min(quantity, totalDemand[itemId]);
                coverage.totalCoverage += covered;
                coverage.coverageMap[itemId] = covered;
            }
        }
        
        if (coverage.uniqueItems > 0) {
            // Score: prioridade para corredores que cobrem itens exclusivos
            coverage.score = coverage.uniqueItems * 1000.0 + coverage.totalCoverage;
            aisles.push_back(coverage);
        }
    }
    
    // Ordenar por pontuação (maior primeiro)
    std::sort(aisles.begin(), aisles.end(), 
              [](const AisleCoverage& a, const AisleCoverage& b) {
                  return a.score > b.score;
              });
    
    // Selecionar corredores até cobrir toda a demanda
    std::map<int, int> coveredDemand;
    bool allCovered = false;
    
    while (!allCovered && !aisles.empty()) {
        const auto& bestAisle = aisles[0];
        selectedAisles.insert(bestAisle.id);
        
        // Atualizar cobertura
        for (const auto& [itemId, covered] : bestAisle.coverageMap) {
            int remaining = totalDemand[itemId] - coveredDemand[itemId];
            int additional = std::min(covered, remaining);
            coveredDemand[itemId] += additional;
        }
        
        // Verificar se tudo foi coberto
        allCovered = true;
        for (const auto& [itemId, demand] : totalDemand) {
            if (coveredDemand[itemId] < demand) {
                allCovered = false;
                break;
            }
        }
        
        // Remover este corredor e recalcular scores
        aisles.erase(aisles.begin());
        
        // Atualizar scores dos corredores restantes
        for (auto& aisle : aisles) {
            aisle.uniqueItems = 0;
            aisle.totalCoverage = 0;
            
            for (auto& [itemId, covered] : aisle.coverageMap) {
                int remaining = totalDemand[itemId] - coveredDemand[itemId];
                
                if (remaining > 0) {
                    aisle.uniqueItems++;
                    int newCovered = std::min(covered, remaining);
                    aisle.totalCoverage += newCovered;
                    // Atualizar coverageMap não é necessário aqui
                }
            }
            
            // Recalcular score
            aisle.score = aisle.uniqueItems * 1000.0 + aisle.totalCoverage;
        }
        
        // Reordenar
        std::sort(aisles.begin(), aisles.end(), 
                  [](const AisleCoverage& a, const AisleCoverage& b) {
                      return a.score > b.score;
                  });
    }
    
    // Atualizar solução
    solution.visitedAisles.assign(selectedAisles.begin(), selectedAisles.end());
}

// Resolver o problema usando Método de Dinkelbach
Solution solveParametricProblem(const Instance& instance, double lambda, const Config& config) {
    // Marcar parâmetro para evitar warning
    (void)config;
    
    // Calcular valor paramétrico para cada pedido (itens - lambda * corredores)
    struct OrderValue {
        int id;
        int index;
        double paramValue;
        int totalItems;
        std::set<int> requiredAisles;
    };
    
    std::vector<OrderValue> orderValues;
    
    for (size_t i = 0; i < instance.orders.size(); ++i) {
        const Order& order = instance.orders[i];
        OrderValue value;
        value.id = order.id;
        value.index = i;
        value.totalItems = optimizedDS.totalItemsPerOrder[i];
        value.requiredAisles = optimizedDS.requiredAislesPerOrder[i];
        
        // Calcular valor paramétrico (N(x) - λD(x))
        value.paramValue = value.totalItems - lambda * value.requiredAisles.size();
        
        // Considerar apenas pedidos com valor positivo
        if (value.paramValue > 0) {
            orderValues.push_back(value);
        }
    }
    
    // Ordenar por valor paramétrico (maior para menor)
    std::sort(orderValues.begin(), orderValues.end(),
              [](const OrderValue& a, const OrderValue& b) {
                  return a.paramValue > b.paramValue;
              });
    
    // Construir solução gulosa
    Solution solution;
    std::set<int> selectedAisles;
    int totalItems = 0;
    
    // Estrutura para acompanhar disponibilidade restante de itens
    std::unordered_map<int, int> itemAvailability;
    for (const auto& aisle : instance.aisles) {
        for (const auto& [itemId, quantity] : aisle.itemQuantities) {
            itemAvailability[itemId] += quantity;
        }
    }
    
    for (const auto& order : orderValues) {
        // Verificar limite superior
        if (totalItems + order.totalItems > instance.UB) {
            continue;
        }
        
        // Verificar disponibilidade de itens
        bool canAdd = true;
        std::unordered_map<int, int> itemsNeeded;
        
        for (const auto& [itemId, quantity] : optimizedDS.orderToItems[order.index]) {
            itemsNeeded[itemId] += quantity;
            if (itemsNeeded[itemId] > itemAvailability[itemId]) {
                canAdd = false;
                break;
            }
        }
        
        if (canAdd) {
            // Adicionar pedido à solução
            solution.selectedOrders.push_back(order.id);
            totalItems += order.totalItems;
            
            // Atualizar disponibilidade
            for (const auto& [itemId, quantity] : itemsNeeded) {
                itemAvailability[itemId] -= quantity;
            }
            
            // Adicionar corredores necessários
            for (int aisleId : order.requiredAisles) {
                selectedAisles.insert(aisleId);
            }
        }
        
        // Se atingimos o limite inferior, podemos parar
        if (totalItems >= instance.LB) {
            break;
        }
    }
    
    // Se não atingimos o limite inferior, tente uma abordagem alternativa
    if (totalItems < instance.LB) {
        // Ordenar por quantidade total de itens
        std::sort(orderValues.begin(), orderValues.end(),
                  [](const OrderValue& a, const OrderValue& b) {
                      return a.totalItems > b.totalItems;
                  });
        
        // Reiniciar solução
        solution.selectedOrders.clear();
        selectedAisles.clear();
        totalItems = 0;
        
        // Reiniciar disponibilidade
        for (const auto& aisle : instance.aisles) {
            for (const auto& [itemId, quantity] : aisle.itemQuantities) {
                itemAvailability[itemId] += quantity;
            }
        }
        
        // Tentar adicionar pedidos priorizando quantidade de itens
        for (const auto& order : orderValues) {
            if (totalItems >= instance.LB) {
                break;
            }
            
            if (totalItems + order.totalItems > instance.UB) {
                continue;
            }
            
            bool canAdd = true;
            std::unordered_map<int, int> itemsNeeded;
            
            for (const auto& [itemId, quantity] : optimizedDS.orderToItems[order.index]) {
                itemsNeeded[itemId] += quantity;
                if (itemsNeeded[itemId] > itemAvailability[itemId]) {
                    canAdd = false;
                    break;
                }
            }
            
            if (canAdd) {
                solution.selectedOrders.push_back(order.id);
                totalItems += order.totalItems;
                
                for (const auto& [itemId, quantity] : itemsNeeded) {
                    itemAvailability[itemId] -= quantity;
                }
                
                for (int aisleId : order.requiredAisles) {
                    selectedAisles.insert(aisleId);
                }
            }
        }
    }
    
    // Atualizar corredores visitados
    solution.visitedAisles.assign(selectedAisles.begin(), selectedAisles.end());
    
    // Verificar e otimizar corredores visitados
    updateVisitedAisles(solution, instance);
    
    return solution;
}

// Implementação do método de Dinkelbach
Solution solveSPO(const Instance& instance, const Config& config) {
    std::cout << "\n====== Executando Método de Dinkelbach ======" << std::endl;
    
    // Medir tempo de execução
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Inicializar estruturas de dados
    initializeOptimizedDataStructures(instance, config);
    
    // Gerar solução inicial
    Solution bestSolution = generateInitialSolution(instance, config);
    
    // Garantir que todos os corredores necessários estão incluídos
    updateVisitedAisles(bestSolution, instance);
    
    // Calcular ratio inicial
    int totalItems = 0;
    for (int orderId : bestSolution.selectedOrders) {
        for (const auto& order : instance.orders) {
            if (order.id == orderId) {
                for (const Item& item : order.items) {
                    totalItems += item.quantity;
                }
                break;
            }
        }
    }
    
    // Evitar divisão por zero
    if (bestSolution.visitedAisles.empty()) {
        std::cerr << "ERRO: Solução inicial não contém corredores." << std::endl;
        return bestSolution;
    }
    
    double bestRatio = static_cast<double>(totalItems) / bestSolution.visitedAisles.size();
    double lambda = bestRatio;
    
    std::cout << "Solução inicial: " << totalItems << " itens, " 
              << bestSolution.visitedAisles.size() << " corredores, "
              << "razão: " << bestRatio << std::endl;
    
    // Método de Dinkelbach
    int iter = 0;
    const int MAX_ITERATIONS = config.maxIterations;
    double epsilon = config.epsilon;
    bool converged = false;
    
    while (iter < MAX_ITERATIONS && !converged) {
        // Verificar timeout
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - startTime).count();
        
        if (elapsedSeconds > config.maxTime * 0.8) { // 80% do tempo disponível
            std::cout << "Limite de tempo atingido. Parando iterações." << std::endl;
            break;
        }
        
        // Resolver o subproblema paramétrico
        Solution newSolution = solveParametricProblem(instance, lambda, config);
        
        // Calcular nova razão
        totalItems = 0;
        for (int orderId : newSolution.selectedOrders) {
            for (const auto& order : instance.orders) {
                if (order.id == orderId) {
                    for (const Item& item : order.items) {
                        totalItems += item.quantity;
                    }
                    break;
                }
            }
        }
        
        // Evitar divisão por zero
        if (newSolution.visitedAisles.empty()) {
            std::cout << "Iteração " << iter << ": Solução sem corredores. Usando lambda anterior." << std::endl;
            iter++;
            continue;
        }
        
        double newRatio = static_cast<double>(totalItems) / newSolution.visitedAisles.size();
        
        std::cout << "Iteração " << iter << ": " << totalItems << " itens, " 
                  << newSolution.visitedAisles.size() << " corredores, "
                  << "razão: " << newRatio << std::endl;
        
        // Verificar convergência
        if (std::fabs(newRatio - lambda) < epsilon) {
            converged = true;
            std::cout << "Método de Dinkelbach convergiu na iteração " << iter << std::endl;
            
            if (newRatio > bestRatio) {
                bestSolution = newSolution;
                bestRatio = newRatio;
            }
        } else {
            lambda = newRatio;
            
            if (newRatio > bestRatio) {
                bestSolution = newSolution;
                bestRatio = newRatio;
            }
        }
        
        iter++;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        endTime - startTime).count();
    
    std::cout << "Solução final encontrada em " << totalSeconds << " segundos" << std::endl;
    std::cout << "Pedidos selecionados: " << bestSolution.selectedOrders.size() 
              << ", Corredores utilizados: " << bestSolution.visitedAisles.size() 
              << ", Razão final: " << bestRatio << std::endl;
    
    // Garantir uma solução final válida
    updateVisitedAisles(bestSolution, instance);
    
    return bestSolution;
}

// Função de escrita de solução
void writeSolution(const Solution& solution, const std::string& outputPath) {
    std::ofstream outFile(outputPath);
    
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo de saída: " << outputPath << std::endl;
        return;
    }
    
    // Linha 1: Número de corredores
    outFile << solution.visitedAisles.size() << std::endl;
    
    // Linha 2: IDs dos corredores
    for (size_t i = 0; i < solution.visitedAisles.size(); ++i) {
        outFile << solution.visitedAisles[i];
        if (i < solution.visitedAisles.size() - 1) {
            outFile << " ";
        }
    }
    outFile << std::endl;
    
    // Linha 3: Número de pedidos
    outFile << solution.selectedOrders.size() << std::endl;
    
    // Linha 4: IDs dos pedidos
    for (size_t i = 0; i < solution.selectedOrders.size(); ++i) {
        outFile << solution.selectedOrders[i];
        if (i < solution.selectedOrders.size() - 1) {
            outFile << " ";
        }
    }
    outFile << std::endl;
    
    outFile.close();
    
    std::cout << "Solução escrita em: " << outputPath << std::endl;
}

// Versão com três parâmetros que chama a versão com dois
void writeSolution(const Solution& solution, const Instance& instance, const std::string& outputPath) {
    // Marcar o parâmetro como usado para evitar o warning
    (void)instance;
    
    // Chamar a versão com dois parâmetros
    writeSolution(solution, outputPath);
}

// Adicionar a função readInstance
Instance readInstance(const std::string& filename) {
    Instance instance;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << filename << std::endl;
        return instance;
    }
    
    // Ler número de pedidos, itens e corredores
    int numOrders, numItems, numAisles;
    file >> numOrders >> numItems >> numAisles;
    
    // Ler limites LB e UB
    file >> instance.LB >> instance.UB;
    
    // Ler pedidos
    instance.orders.resize(numOrders);
    for (int i = 0; i < numOrders; ++i) {
        Order& order = instance.orders[i];
        int orderId, numOrderItems;
        file >> orderId >> numOrderItems;
        
        order.id = orderId;
        order.items.resize(numOrderItems);
        
        for (int j = 0; j < numOrderItems; ++j) {
            int itemId, quantity;
            file >> itemId >> quantity;
            
            order.items[j].id = itemId;
            order.items[j].quantity = quantity;
        }
    }
    
    // Ler corredores
    instance.aisles.resize(numAisles);
    for (int i = 0; i < numAisles; ++i) {
        Aisle& aisle = instance.aisles[i];
        int aisleId, numAisleItems;
        file >> aisleId >> numAisleItems;
        
        aisle.id = aisleId;
        
        for (int j = 0; j < numAisleItems; ++j) {
            int itemId, quantity;
            file >> itemId >> quantity;
            
            aisle.itemQuantities[itemId] = quantity;
        }
    }
    
    file.close();
    
    std::cout << "Lendo instância com " << numOrders << " pedidos, " 
              << numItems << " itens e " << numAisles << " corredores" << std::endl;
    std::cout << "Limites lidos: LB=" << instance.LB << ", UB=" << instance.UB << std::endl;
    
    return instance;
}

// Implementação da função validateSolution
bool validateSolution(const Instance& instance, const Solution& solution, const Config& config) {
    std::cout << "Validando solução..." << std::endl;
    
    int totalItems = 0;
    std::unordered_map<int, int> requiredItems;
    
    // Verificar pedidos selecionados
    for (int orderId : solution.selectedOrders) {
        bool orderFound = false;
        for (const auto& order : instance.orders) {
            if (order.id == orderId) {
                orderFound = true;
                // Contabilizar itens do pedido
                for (const Item& item : order.items) {
                    totalItems += item.quantity;
                    requiredItems[item.id] += item.quantity;
                }
                break;
            }
        }
        
        if (!orderFound) {
            std::cerr << "Erro: Pedido " << orderId << " não existe na instância" << std::endl;
            return false;
        }
    }
    
    // Verificar limites configurados
    if (totalItems < config.minItems) {
        std::cout << "Aviso: O número total de itens (" << totalItems << ") é menor que o limite inferior configurado (" 
                 << config.minItems << ")" << std::endl;
    } else if (totalItems > config.maxItems) {
        std::cout << "Aviso: O número total de itens (" << totalItems << ") é maior que o limite superior configurado (" 
                 << config.maxItems << ")" << std::endl;
    } else {
        std::cout << "Número total de itens (" << totalItems << ") está dentro dos limites configurados" << std::endl;
    }
    
    // Verificar limites da instância
    if (totalItems < instance.LB) {
        std::cerr << "Erro: O número total de itens (" << totalItems << ") é menor que o limite inferior da instância (" 
                 << instance.LB << ")" << std::endl;
        return false;
    } else if (totalItems > instance.UB) {
        std::cerr << "Erro: O número total de itens (" << totalItems << ") é maior que o limite superior da instância (" 
                 << instance.UB << ")" << std::endl;
        return false;
    } else {
        std::cout << "Número total de itens (" << totalItems << ") está dentro dos limites da instância" << std::endl;
    }
    
    // Verificar disponibilidade nos corredores
    std::unordered_map<int, int> availableItems;
    for (int aisleId : solution.visitedAisles) {
        const Aisle* aisle = nullptr;
        for (const auto& a : instance.aisles) {
            if (a.id == aisleId) {
                aisle = &a;
                break;
            }
        }
        
        if (aisle) {
            for (const auto& [itemId, quantity] : aisle->itemQuantities) {
                availableItems[itemId] += quantity;
            }
        } else {
            std::cerr << "Erro: Corredor " << aisleId << " não existe na instância" << std::endl;
            return false;
        }
    }
    
    // Verificar se há itens suficientes nos corredores
    bool allItemsAvailable = true;
    for (const auto& [itemId, required] : requiredItems) {
        if (availableItems[itemId] < required) {
            std::cerr << "Erro: Item " << itemId << " tem quantidade insuficiente nos corredores visitados" << std::endl;
            std::cerr << "  Demandado: " << required << ", Disponível: " << availableItems[itemId] << std::endl;
            allItemsAvailable = false;
        }
    }
    
    // Verificar IDs dos pedidos
    std::set<int> validOrderIds;
    for (const auto& order : instance.orders) {
        validOrderIds.insert(order.id);
    }
    
    for (int orderId : solution.selectedOrders) {
        if (validOrderIds.find(orderId) == validOrderIds.end()) {
            std::cerr << "Erro: Pedido com ID " << orderId << " não existe na instância" << std::endl;
            return false;
        }
    }
    
    std::cout << "Todos os IDs de pedidos são válidos" << std::endl;
    
    // Calcular e mostrar a razão itens/corredores
    if (!solution.visitedAisles.empty()) {
        double ratio = static_cast<double>(totalItems) / solution.visitedAisles.size();
        std::cout << "Razão final (itens/corredores): " << ratio << std::endl;
    }
    
    return allItemsAvailable;
}

// Adicionar a função validateSolutionFull
bool validateSolutionFull(const Instance& instance, const Solution& solution, const Config& config) {
    // Marcar o parâmetro como usado para evitar o aviso
    (void)config;
    
    // Verificar número de itens (LB/UB)
    int totalItems = 0;
    std::unordered_map<int, int> requiredItems;
    
    for (int orderId : solution.selectedOrders) {
        const Order* order = nullptr;
        for (const auto& o : instance.orders) {
            if (o.id == orderId) {
                order = &o;
                break;
            }
        }
        
        if (!order) {
            std::cerr << "Erro: Pedido " << orderId << " não encontrado na instância" << std::endl;
            return false;
        }
        
        for (const Item& item : order->items) {
            totalItems += item.quantity;
            requiredItems[item.id] += item.quantity;
        }
    }
    
    // Verificar se está dentro dos limites configurados
    if (totalItems < config.minItems || totalItems > config.maxItems) {
        if (totalItems < config.minItems) {
            std::cerr << "Aviso: O número total de itens (" << totalItems 
                     << ") é menor que o limite inferior configurado (" << config.minItems << ")" << std::endl;
        } else {
            std::cerr << "Aviso: O número total de itens (" << totalItems 
                     << ") é maior que o limite superior configurado (" << config.maxItems << ")" << std::endl;
        }
    }
    
    // Verificar se está dentro dos limites da instância
    if (totalItems < instance.LB) {
        std::cerr << "Erro: O número total de itens (" << totalItems 
                 << ") é menor que o limite inferior da instância (" << instance.LB << ")" << std::endl;
        return false;
    }
    
    if (totalItems > instance.UB) {
        std::cerr << "Erro: O número total de itens (" << totalItems 
                 << ") é maior que o limite superior da instância (" << instance.UB << ")" << std::endl;
        return false;
    }
    
    // Verificar disponibilidade de itens nos corredores
    std::unordered_map<int, int> availableItems;
    for (int aisleId : solution.visitedAisles) {
        const Aisle* aisle = nullptr;
        for (const auto& a : instance.aisles) {
            if (a.id == aisleId) {
                aisle = &a;
                break;
            }
        }
        
        if (!aisle) {
            std::cerr << "Erro: Corredor " << aisleId << " não encontrado na instância" << std::endl;
            return false;
        }
        
        for (const auto& [itemId, quantity] : aisle->itemQuantities) {
            availableItems[itemId] += quantity;
        }
    }
    
    bool itemsAvailable = true;
    for (const auto& [itemId, required] : requiredItems) {
        if (availableItems[itemId] < required) {
            std::cerr << "Erro: Item " << itemId << " tem quantidade insuficiente nos corredores visitados" << std::endl;
            std::cerr << "  Demandado: " << required << ", Disponível: " << availableItems[itemId] << std::endl;
            itemsAvailable = false;
        }
    }
    
    // Verificar IDs de pedidos
    bool validOrderIds = true;
    std::set<int> instanceOrderIds;
    for (const auto& order : instance.orders) {
        instanceOrderIds.insert(order.id);
    }
    
    for (int orderId : solution.selectedOrders) {
        if (instanceOrderIds.find(orderId) == instanceOrderIds.end()) {
            std::cerr << "Erro: Pedido com ID " << orderId << " não existe na instância" << std::endl;
            validOrderIds = false;
        }
    }
    
    if (validOrderIds) {
        std::cout << "Todos os IDs de pedidos são válidos" << std::endl;
    }
    
    return itemsAvailable && validOrderIds;
}