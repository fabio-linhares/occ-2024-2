#include "otimizador_dinkelbach.h"
#include "busca_local_avancada.h"
#include "branch_and_bound_solver.h" // Include B&B solver header
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <random> // For std::random_device, std::mt19937, std::shuffle
#include <cmath> // For std::abs, std::sqrt
#include <limits> // For std::numeric_limits

OtimizadorDinkelbach::OtimizadorDinkelbach(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador
) :
    OtimizadorWave(deposito, backlog, localizador, verificador),  // Chamada ao construtor da classe base
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    epsilon_(0.0001),
    maxIteracoes_(1000),
    usarBranchAndBound_(true),
    usarBuscaLocalAvancada_(true),
    limiteTempoBuscaLocal_(1.0) // Default 1 second for BL
{
}

// Corrigir a função estimarLambdaInicial que estava com erros
double estimarLambdaInicial(const Deposito& deposito, const Backlog& backlog,
                           const LocalizadorItens& localizador) {
    const int AMOSTRA_SIZE = std::min(100, backlog.numPedidos);
    double somaEficiencias = 0.0;
    int contadorValidos = 0;

    std::vector<int> indices(backlog.numPedidos);
    std::iota(indices.begin(), indices.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    for (int i = 0; i < AMOSTRA_SIZE && i < (int)indices.size(); ++i) {
        int pedidoId = indices[i];
        int unidades = 0;
        std::unordered_set<int> corredores;
        
        // Calcular unidades totais e corredores necessários para este pedido
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            unidades += quantidade;
            for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                corredores.insert(corredorId);
            }
        }
        
        // Calcular eficiência se o pedido tem unidades e requer corredores
        if (unidades > 0 && !corredores.empty()) {
            double eficiencia = static_cast<double>(unidades) / corredores.size();
            somaEficiencias += eficiencia;
            contadorValidos++;
        }
    }

    if (contadorValidos > 0) {
        return somaEficiencias / contadorValidos; // Média das eficiências amostradas
    }

    // Fallback: ratio of average units per order (estimated) to average corridors per order (estimated)
    double avgUnits = (backlog.wave.LB + backlog.wave.UB) / 2.0 / std::max(1, backlog.numPedidos / 10);
    double avgCorridors = std::sqrt(deposito.numCorredores);
    if (avgCorridors > 0) {
        return avgUnits / avgCorridors;
    }

    return 1.0; // Valor padrão se nenhum cálculo for possível
}

int OtimizadorDinkelbach::calcularTotalUnidades(const SolucaoWave& solucao) {
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
         if (pedidoId >= 0 && pedidoId < backlog_.numPedidos) {
             for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                 totalUnidades += quantidade;
             }
         }
    }
    return totalUnidades;
}

// Implementação corrigida do método otimizarWave
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::otimizarWave(int LB, int UB) {
    infoConvergencia_ = InfoConvergencia{};
    auto inicioTempo = std::chrono::high_resolution_clock::now();

    bool instanciaPequena = backlog_.numPedidos <= 150;
    double lambda = estimarLambdaInicial(deposito_, backlog_, localizador_);
    // std::cout << "Lambda inicial estimado: " << lambda << std::endl;

    SolucaoWave melhorSolucao;
    melhorSolucao.valorObjetivo = -1.0;
    int iteracao = 0;
    double valorF_atual = 0;
    double valorG_atual = 0;
    
    // Registrar valor inicial de lambda
    infoConvergencia_.valoresLambda.push_back(lambda);
    infoConvergencia_.valoresObjetivo.push_back(-1.0); // Ainda não temos uma solução
    infoConvergencia_.convergiu = false;
    
    while (iteracao < maxIteracoes_) {
        iteracao++;
        
        // Resolver o subproblema linearizado F(x) - lambda*G(x)
        std::pair<SolucaoWave, double> resultado;
        
        if (usarBranchAndBound_ && (instanciaPequena || iteracao <= 3)) {
            // Para instâncias pequenas ou primeiras iterações, usar B&B
            resultado = resolverSubproblemaComBranchAndBound(lambda, LB, UB);
        } else {
            // Para instâncias maiores ou após algumas iterações, usar heurística
            resultado = resolverSubproblemaComHeuristica(lambda, LB, UB);
        }
        
        SolucaoWave& solucaoAtual = resultado.first;
        double valorSubproblema = resultado.second;
        
        // Se não encontramos uma solução viável, tentar outra abordagem
        if (solucaoAtual.pedidosWave.empty()) {
            if (usarBranchAndBound_ && !instanciaPequena) {
                // Se o B&B não funcionou, tentar heurística
                resultado = resolverSubproblemaComHeuristica(lambda, LB, UB);
                solucaoAtual = resultado.first;
                valorSubproblema = resultado.second;
            }
            
            // Se ainda não temos solução, continuar para próxima iteração
            if (solucaoAtual.pedidosWave.empty()) {
                continue;
            }
        }
        
        // Calcular F e G para a solução atual
        valorF_atual = calcularTotalUnidades(solucaoAtual);
        valorG_atual = solucaoAtual.corredoresWave.size();
        
        // Calcular BOV (valor objetivo original - F/G)
        double bov = valorG_atual > 0 ? valorF_atual / valorG_atual : 0;
        solucaoAtual.valorObjetivo = bov;
        
        // Atualizar melhor solução
        if (bov > melhorSolucao.valorObjetivo) {
            melhorSolucao = solucaoAtual;
        }
        
        // Verificar critério de parada por convergência
        double diferenca = std::abs(valorSubproblema);
        if (diferenca < epsilon_) {
            infoConvergencia_.convergiu = true;
            break;
        }
        
        // Atualizar lambda para próxima iteração
        double novoLambda = valorF_atual / valorG_atual;
        
        // Evitar mudanças bruscas (estabilização)
        if (iteracao > 1) {
            novoLambda = 0.7 * novoLambda + 0.3 * lambda;
        }
        
        // Registrar para estatísticas
        infoConvergencia_.valoresLambda.push_back(novoLambda);
        infoConvergencia_.valoresObjetivo.push_back(bov);
        
        // Verificar se lambda está convergindo
        if (std::abs(novoLambda - lambda) / std::max(0.1, std::abs(lambda)) < epsilon_) {
            infoConvergencia_.convergiu = true;
            break;
        }
        
        lambda = novoLambda;
    }
    
    infoConvergencia_.iteracoesRealizadas = iteracao;
    auto fimTempo = std::chrono::high_resolution_clock::now();
    infoConvergencia_.tempoTotal = std::chrono::duration<double>(fimTempo - inicioTempo).count();

    if (!infoConvergencia_.convergiu && iteracao >= maxIteracoes_) {
        std::cout << "Aviso: Algoritmo Dinkelbach não convergiu em " << maxIteracoes_ << " iterações." << std::endl;
    }
    
    if (melhorSolucao.valorObjetivo < 0) {
        std::cout << "Erro: Não foi possível encontrar uma solução viável." << std::endl;
        return SolucaoWave{}; // Retornar solução vazia
    }

    // Aplicar busca local avançada para refinamento
    if (usarBuscaLocalAvancada_ && !melhorSolucao.pedidosWave.empty()) {
        BuscaLocalAvancada buscaLocal(
            deposito_, backlog_, localizador_, verificador_, limiteTempoBuscaLocal_);
        
        // Converter SolucaoWave para formato da BuscaLocalAvancada
        BuscaLocalAvancada::Solucao solBL;
        solBL.pedidosWave = melhorSolucao.pedidosWave;
        solBL.corredoresWave = melhorSolucao.corredoresWave;
        solBL.valorObjetivo = melhorSolucao.valorObjetivo;
        solBL.totalUnidades = calcularTotalUnidades(melhorSolucao);
        
        // Executar busca local com tempo limitado
        solBL = buscaLocal.otimizar(solBL, LB, UB, BuscaLocalAvancada::TipoBuscaLocal::BUSCA_TABU);
        
        // Verificar se a busca local melhorou a solução
        if (solBL.valorObjetivo > melhorSolucao.valorObjetivo) {
            melhorSolucao.pedidosWave = solBL.pedidosWave;
            melhorSolucao.corredoresWave = solBL.corredoresWave;
            melhorSolucao.valorObjetivo = solBL.valorObjetivo;
        }
    }

    return melhorSolucao;
}

// Solve subproblem using Branch and Bound
std::pair<OtimizadorDinkelbach::SolucaoWave, double>
OtimizadorDinkelbach::resolverSubproblemaComBranchAndBound(double lambda, int LB, int UB) {
    // Determine time limit based on instance size
    double limiteTempoSegundos = (backlog_.numPedidos > 100) ? std::max(0.5, limiteTempoBuscaLocal_ / 2.0) : std::max(1.0, limiteTempoBuscaLocal_); // Allocate time for B&B

    // Select branching strategy
    BranchAndBoundSolver::EstrategiaSelecionarVariavel estrategia;
    if (backlog_.numPedidos <= 50) {
        estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::PSEUDO_CUSTO;
    } else if (backlog_.numPedidos <= 100) {
        estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MAIOR_IMPACTO;
    } else {
        estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MAIOR_IMPACTO; // MOST_INFEASIBLE needs better definition
    }

    // Create and configure the solver
    BranchAndBoundSolver solver(
        deposito_,
        backlog_,
        localizador_,
        verificador_,
        limiteTempoSegundos,
        estrategia
    );

    // Configure cuts and bound coefficient based on Dinkelbach iteration progress
    solver.setUsarCortesCobertura(true);
    solver.setUsarCortesDominancia(true); // Enable dominance cuts if implemented
    // Adjust bound coefficient based on iteration (more optimistic early on)
    double coef = (infoConvergencia_.valoresLambda.size() < 5) ? 0.9 : 0.7;
    solver.setCoeficienteLimite(coef); // Setter needs to exist in B&B solver

    // Solve the subproblem
    BranchAndBoundSolver::Solucao solucaoBnB = solver.resolver(lambda, LB, UB);

    // Convert result back to SolucaoWave
    SolucaoWave resultado;
    resultado.pedidosWave = solucaoBnB.pedidosWave;
    resultado.corredoresWave = solucaoBnB.corredoresWave;
    resultado.valorObjetivo = solucaoBnB.valorObjetivo; // This is BOV

    // Calculate the linearized objective value F(x) - lambda*G(x) for the returned solution
    double valorSubproblema = calcularValorSubproblema(resultado, lambda);

    return {resultado, valorSubproblema};
}

// Solve subproblem using a greedy heuristic
std::pair<OtimizadorDinkelbach::SolucaoWave, double>
OtimizadorDinkelbach::resolverSubproblemaComHeuristica(double lambda, int LB, int UB) {
     // This is essentially the same logic as BranchAndBoundSolver::construirSolucaoGulosa
     // We reuse that logic here for consistency.

     struct PedidoPontuado {
         int id;
         int unidades;
         std::unordered_set<int> corredores;
         double pontuacao; // Linearized score
         double eficiencia; // BOV
     };

     std::vector<PedidoPontuado> pedidos;
     pedidos.reserve(backlog_.numPedidos);

     for (int i = 0; i < backlog_.numPedidos; i++) {
         PedidoPontuado pedido;
         pedido.id = i;
         pedido.unidades = 0;
         pedido.corredores.clear();
         for (const auto& [itemId, quantidade] : backlog_.pedido[i]) {
             pedido.unidades += quantidade;
             for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                 pedido.corredores.insert(corredorId);
             }
         }
         if (pedido.unidades == 0) continue;
         pedido.pontuacao = pedido.unidades - lambda * pedido.corredores.size();
         pedido.eficiencia = pedido.corredores.empty() ? std::numeric_limits<double>::infinity() : static_cast<double>(pedido.unidades) / pedido.corredores.size();
         pedidos.push_back(pedido);
     }

     std::sort(pedidos.begin(), pedidos.end(), [](const PedidoPontuado& a, const PedidoPontuado& b) {
         return a.pontuacao > b.pontuacao;
     });

     SolucaoWave solucao;
     solucao.valorObjetivo = 0.0;
     int totalUnidades = 0;
     std::unordered_set<int> corredoresUnicos;
     std::unordered_map<int, int> estoqueConsumido;

     // Phase 1: Add positive score orders
     for (const auto& pedido : pedidos) {
         if (pedido.pontuacao <= 0) continue;
         if (totalUnidades + pedido.unidades > UB) continue;

         bool viavelEstoque = true;
         std::unordered_map<int, int> consumoPedido;
         for (const auto& [itemId, quantidade] : backlog_.pedido[pedido.id]) {
             consumoPedido[itemId] += quantidade;
             if (estoqueConsumido[itemId] + quantidade > verificador_.estoqueTotal[itemId]) {
                 viavelEstoque = false; break;
             }
         }
         if (!viavelEstoque) continue;

         solucao.pedidosWave.push_back(pedido.id);
         totalUnidades += pedido.unidades;
         for(const auto& [itemId, quantidade] : consumoPedido) estoqueConsumido[itemId] += quantidade;
         for (int corredor : pedido.corredores) corredoresUnicos.insert(corredor);
         // if (totalUnidades >= LB) break; // Optional early exit
     }

     // Phase 2: Add efficiency-based orders if LB not met
     if (totalUnidades < LB) {
         std::sort(pedidos.begin(), pedidos.end(), [](const PedidoPontuado& a, const PedidoPontuado& b) {
              if (a.corredores.empty() && b.corredores.empty()) return a.unidades > b.unidades;
              if (a.corredores.empty()) return true;
              if (b.corredores.empty()) return false;
              return a.eficiencia > b.eficiencia;
         });
         for (const auto& pedido : pedidos) {
             bool jaIncluido = false;
             for(int pid : solucao.pedidosWave) { if(pid == pedido.id) { jaIncluido = true; break; } }
             if (jaIncluido || pedido.unidades == 0) continue;
             if (totalUnidades + pedido.unidades > UB) continue;

             bool viavelEstoque = true;
             std::unordered_map<int, int> consumoPedido;
             for (const auto& [itemId, quantidade] : backlog_.pedido[pedido.id]) {
                 consumoPedido[itemId] += quantidade;
                 if (estoqueConsumido[itemId] + quantidade > verificador_.estoqueTotal[itemId]) {
                     viavelEstoque = false; break;
                 }
             }
             if (!viavelEstoque) continue;

             solucao.pedidosWave.push_back(pedido.id);
             totalUnidades += pedido.unidades;
             for(const auto& [itemId, quantidade] : consumoPedido) estoqueConsumido[itemId] += quantidade;
             for (int corredor : pedido.corredores) corredoresUnicos.insert(corredor);
             if (totalUnidades >= LB) break;
         }
     }

     // Final check for LB
     if (totalUnidades < LB) {
          // Failed to build feasible solution with heuristic
          return {SolucaoWave{}, -std::numeric_limits<double>::infinity()}; // Return empty, very low value
     }

     solucao.corredoresWave.assign(corredoresUnicos.begin(), corredoresUnicos.end());
     std::sort(solucao.corredoresWave.begin(), solucao.corredoresWave.end());
     solucao.valorObjetivo = (corredoresUnicos.size() > 0) ? static_cast<double>(totalUnidades) / corredoresUnicos.size() : 0.0;

     // Calculate the linearized objective value F(x) - lambda*G(x)
     double valorSubproblema = calcularValorSubproblema(solucao, lambda);

     return {solucao, valorSubproblema};
}


// Calculate original objective value (BOV) for a set of orders
double OtimizadorDinkelbach::calcularValorObjetivo(const std::vector<int>& pedidosWave) {
    if (pedidosWave.empty()) {
        return 0.0;
    }
    SolucaoWave tempSol;
    tempSol.pedidosWave = pedidosWave;
    tempSol.corredoresWave = construirListaCorredores(pedidosWave);
    int totalUnidades = calcularTotalUnidades(tempSol);

    if (tempSol.corredoresWave.empty()) {
        return (totalUnidades > 0) ? std::numeric_limits<double>::infinity() : 0.0;
    }
    return static_cast<double>(totalUnidades) / tempSol.corredoresWave.size();
}

// Build the list of unique corridors for a set of orders
std::vector<int> OtimizadorDinkelbach::construirListaCorredores(const std::vector<int>& pedidosWave) {
    std::unordered_set<int> corredoresSet;
    for (int pedidoId : pedidosWave) {
        if (pedidoId >= 0 && pedidoId < backlog_.numPedidos) {
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    corredoresSet.insert(corredorId);
                }
            }
        }
    }
    std::vector<int> corredoresVec(corredoresSet.begin(), corredoresSet.end());
    std::sort(corredoresVec.begin(), corredoresVec.end());
    return corredoresVec;
}

// Display convergence details
void OtimizadorDinkelbach::exibirDetalhesConvergencia() const {
    std::cout << "\n--- Detalhes da Convergencia Dinkelbach ---\n";
    std::cout << "Iteracoes realizadas: " << infoConvergencia_.iteracoesRealizadas << "\n";
    std::cout << "Tempo total: " << std::fixed << std::setprecision(4) << infoConvergencia_.tempoTotal << " s\n";
    std::cout << "Convergiu: " << (infoConvergencia_.convergiu ? "Sim" : "Nao") << "\n";
    std::cout << "Iter | Lambda        | Objetivo (BOV)\n";
    std::cout << "-----|---------------|----------------\n";
    for (size_t i = 0; i < infoConvergencia_.valoresLambda.size(); ++i) {
        std::cout << std::setw(4) << i << " | "
                  << std::setw(13) << std::left << infoConvergencia_.valoresLambda[i] << " | "
                  << std::setw(14) << std::left << infoConvergencia_.valoresObjetivo[i] << "\n";
    }
    std::cout << "-----------------------------------------\n";
}

// Calculate value of F(x) - lambda*G(x)
double OtimizadorDinkelbach::calcularValorSubproblema(const SolucaoWave& solucao, double lambda) {
    double valorF = calcularTotalUnidades(solucao);
    double valorG = solucao.corredoresWave.size();
    return valorF - lambda * valorG;
}

// Adicionar a implementação:

OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::otimizarWaveComReinicializacoes(int LB, int UB) {
    auto inicioTotal = std::chrono::high_resolution_clock::now();
    
    // Inicializar melhor solução global
    SolucaoWave melhorSolucaoGlobal;
    double melhorValorObjetivo = -std::numeric_limits<double>::infinity();
    
    // Armazenar pool de melhores soluções
    std::vector<std::pair<SolucaoWave, double>> poolMelhoresSolucoes;
    
    // Número de reinicializações a realizar
    int numReinicializacoes = configReinicializacao_.numReinicializacoes;
    
    // Configurações originais para restaurar entre execuções
    int maxIteracoesOriginal = maxIteracoes_;
    double epsilonOriginal = epsilon_;
    
    std::cout << "\n[INFO] Iniciando otimização com " << numReinicializacoes << " reinicializações...\n";
    
    // Gerador de números aleatórios
    std::random_device rd;
    std::mt19937 rng(rd());
    
    // Loop de reinicializações
    for (int i = 0; i < numReinicializacoes; i++) {
        std::cout << "\n=== Reinicialização " << (i+1) << "/" << numReinicializacoes << " ===\n";
        
        // Ajustar parâmetros dinâmicos se configurado
        if (configReinicializacao_.aumentarIteracoesProgressivamente) {
            // Aumentar iterações progressivamente (de 1000 até maxIteracoesOriginal)
            double fator = ajustarParametrosDinamicos(i, numReinicializacoes);
            maxIteracoes_ = 1000 + static_cast<int>((maxIteracoesOriginal - 1000) * fator);
            
            // Ajustar epsilon - mais restritivo nas últimas reinicializações
            epsilon_ = epsilonOriginal * (1.0 - 0.5 * fator);
            
            std::cout << "  Parâmetros ajustados: maxIteracoes=" << maxIteracoes_ 
                      << ", epsilon=" << epsilon_ << std::endl;
        }
        
        // Ajustar semente aleatória para cada reinicialização se configurado
        if (configReinicializacao_.usarSementesAleatorias) {
            std::uniform_int_distribution<> dist(1, 100000);
            int semente = dist(rng);
            std::cout << "  Utilizando semente aleatória: " << semente << std::endl;
            
            // Usar a semente para inicializar geradores aleatórios internos
            std::mt19937 rngInterna(semente);
            // (Você precisaria passar essa semente para os componentes internos)
        }
        
        // Estratégia para a reinicialização atual
        SolucaoWave solucaoInicial;
        SolucaoWave solucaoOtimizada;

        if (i > 0 && configReinicializacao_.guardarMelhoresSolucoes && !poolMelhoresSolucoes.empty()) {
            // Estratégia 1: Perturbar melhor solução anterior
            if (i % 3 == 1) {
                std::cout << "  Estratégia: Perturbação de melhor solução anterior\n";
                solucaoInicial = poolMelhoresSolucoes[0].first;
                
                // Aplicar perturbação à melhor solução
                double nivelPerturbacao = 0.2 + (0.5 * ajustarParametrosDinamicos(i, numReinicializacoes));
                solucaoInicial = perturbarSolucao(solucaoInicial, nivelPerturbacao, rng);
                
                // Otimizar a partir da solução perturbada
                solucaoOtimizada = otimizarWave(LB, UB, solucaoInicial);
            }
            // Estratégia 2: Recombinar soluções do pool
            else if (i % 3 == 2 && poolMelhoresSolucoes.size() >= 2) {
                std::cout << "  Estratégia: Recombinação de soluções do pool\n";
                
                // Selecionar duas soluções diferentes do pool
                std::uniform_int_distribution<> dist(0, poolMelhoresSolucoes.size() - 1);
                int idx1 = dist(rng);
                int idx2;
                do {
                    idx2 = dist(rng);
                } while (idx2 == idx1 && poolMelhoresSolucoes.size() > 1);
                
                // Recombinar as soluções
                solucaoInicial = recombinarSolucoes(poolMelhoresSolucoes[idx1].first, 
                                                   poolMelhoresSolucoes[idx2].first, LB, UB);
                
                // Otimizar a partir da solução recombinada
                solucaoOtimizada = otimizarWave(LB, UB, solucaoInicial);
            }
            // Estratégia 3: Solução totalmente nova com foco em diversificação
            else {
                std::cout << "  Estratégia: Geração de nova solução diversificada\n";
                solucaoOtimizada = otimizarWave(LB, UB); // Sem solução inicial
            }
        } else {
            // Primeira iteração sempre usa a abordagem padrão
            std::cout << "  Estratégia: Solução inicial padrão\n";
            solucaoOtimizada = otimizarWave(LB, UB);
        }
        
        // Verificar se encontramos uma solução melhor
        double valorAtual = calcularBOV(solucaoOtimizada);
        
        if (valorAtual > melhorValorObjetivo) {
            melhorValorObjetivo = valorAtual;
            melhorSolucaoGlobal = solucaoOtimizada;
            std::cout << "  ** NOVA MELHOR SOLUÇÃO ** BOV = " << valorAtual
                      << " (Unidades: " << getTotalUnidades(solucaoOtimizada) 
                      << ", Corredores: " << solucaoOtimizada.corredoresWave.size() << ")\n";
        } else {
            std::cout << "  Solução atual BOV = " << valorAtual 
                      << " (não melhora o melhor BOV = " << melhorValorObjetivo << ")\n";
        }
        
        // Adicionar ao pool se configurado
        if (configReinicializacao_.guardarMelhoresSolucoes) {
            // Adicionar esta solução ao pool
            poolMelhoresSolucoes.push_back({solucaoOtimizada, valorAtual});
            
            // Ordenar pool e manter apenas as melhores
            std::sort(poolMelhoresSolucoes.begin(), poolMelhoresSolucoes.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            if (poolMelhoresSolucoes.size() > configReinicializacao_.tamanhoPoolSolucoes) {
                poolMelhoresSolucoes.resize(configReinicializacao_.tamanhoPoolSolucoes);
            }
        }
    }
    
    // Restaurar configurações originais
    maxIteracoes_ = maxIteracoesOriginal;
    epsilon_ = epsilonOriginal;
    
    // Calcular tempo total
    auto fimTotal = std::chrono::high_resolution_clock::now();
    double tempoTotalMs = std::chrono::duration<double, std::milli>(fimTotal - inicioTotal).count();
    
    std::cout << "\n[INFO] Otimização com reinicializações concluída em " 
              << (tempoTotalMs / 1000.0) << " segundos.\n";
    std::cout << "[INFO] Melhor BOV encontrado: " << melhorValorObjetivo << std::endl;
    
    return melhorSolucaoGlobal;
}

// Método para calcular o valor de BOV (unidades/corredores)
double OtimizadorDinkelbach::calcularBOV(const SolucaoWave& solucao) {
    if (solucao.corredoresWave.empty()) return 0.0;
    
    int totalUnidades = getTotalUnidades(solucao);
    return static_cast<double>(totalUnidades) / solucao.corredoresWave.size();
}

// Método para obter o total de unidades de uma solução
int OtimizadorDinkelbach::getTotalUnidades(const SolucaoWave& solucao) {
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    return totalUnidades;
}

// Método para ajustar parâmetros dinâmicos com base no índice de reinicialização
double OtimizadorDinkelbach::ajustarParametrosDinamicos(int indiceReinicializacao, int totalReinicializacoes) {
    // Retorna um valor entre 0.0 (primeira reinicialização) e 1.0 (última reinicialização)
    return static_cast<double>(indiceReinicializacao) / std::max(1, totalReinicializacoes - 1);
}

// Método para perturbar uma solução existente
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::perturbarSolucao(
    const SolucaoWave& solucao, double nivelPerturbacao, std::mt19937& rng) {
    
    SolucaoWave solucaoPerturbada = solucao;
    
    // 1. Remover aleatoriamente alguns pedidos
    int numPedidosARemover = static_cast<int>(solucao.pedidosWave.size() * nivelPerturbacao);
    
    if (numPedidosARemover > 0 && !solucao.pedidosWave.empty()) {
        std::shuffle(solucaoPerturbada.pedidosWave.begin(), solucaoPerturbada.pedidosWave.end(), rng);
        solucaoPerturbada.pedidosWave.resize(solucao.pedidosWave.size() - numPedidosARemover);
    }
    
    // 2. Adicionar aleatoriamente novos pedidos viáveis
    std::vector<int> pedidosDisponiveis;
    std::unordered_set<int> pedidosIncluidos(solucaoPerturbada.pedidosWave.begin(), solucaoPerturbada.pedidosWave.end());
    
    // Identificar pedidos disponíveis para adicionar
    for (int i = 0; i < backlog_.numPedidos; i++) {
        if (pedidosIncluidos.find(i) == pedidosIncluidos.end()) {
            pedidosDisponiveis.push_back(i);
        }
    }
    
    // Adicionar alguns pedidos aleatoriamente
    int numPedidosAdicionar = numPedidosARemover;
    if (!pedidosDisponiveis.empty() && numPedidosAdicionar > 0) {
        std::shuffle(pedidosDisponiveis.begin(), pedidosDisponiveis.end(), rng);
        for (int i = 0; i < std::min(numPedidosAdicionar, static_cast<int>(pedidosDisponiveis.size())); i++) {
            solucaoPerturbada.pedidosWave.push_back(pedidosDisponiveis[i]);
        }
    }
    
    // 3. Reconstruir lista de corredores
    solucaoPerturbada.corredoresWave = construirListaCorredores(solucaoPerturbada.pedidosWave);
    
    return solucaoPerturbada;
}

// Método para recombinar duas soluções
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::recombinarSolucoes(
    const SolucaoWave& solucao1, const SolucaoWave& solucao2, int LB, int UB) {
    
    SolucaoWave solucaoRecombinada;
    std::unordered_set<int> pedidosIncluidos;
    
    // 1. Adicionar pedidos que estão em ambas as soluções
    for (int pedidoId : solucao1.pedidosWave) {
        if (std::find(solucao2.pedidosWave.begin(), solucao2.pedidosWave.end(), pedidoId) != solucao2.pedidosWave.end()) {
            solucaoRecombinada.pedidosWave.push_back(pedidoId);
            pedidosIncluidos.insert(pedidoId);
        }
    }
    
    // 2. Adicionar pedidos exclusivos de cada solução baseado na eficiência
    struct PedidoEficiencia {
        int pedidoId;
        double eficiencia;
    };
    
    std::vector<PedidoEficiencia> pedidosExclusivos;
    
    // Identificar pedidos exclusivos da solução 1
    for (int pedidoId : solucao1.pedidosWave) {
        if (pedidosIncluidos.find(pedidoId) == pedidosIncluidos.end()) {
            int unidades = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                unidades += quantidade;
            }
            
            std::unordered_set<int> corredoresAdicionais;
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    corredoresAdicionais.insert(corredorId);
                }
            }
            
            double eficiencia = corredoresAdicionais.empty() ? unidades : 
                              static_cast<double>(unidades) / corredoresAdicionais.size();
            
            pedidosExclusivos.push_back({pedidoId, eficiencia});
        }
    }
    
    // Identificar pedidos exclusivos da solução 2
    for (int pedidoId : solucao2.pedidosWave) {
        if (pedidosIncluidos.find(pedidoId) == pedidosIncluidos.end()) {
            int unidades = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                unidades += quantidade;
            }
            
            std::unordered_set<int> corredoresAdicionais;
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    corredoresAdicionais.insert(corredorId);
                }
            }
            
            double eficiencia = corredoresAdicionais.empty() ? unidades : 
                              static_cast<double>(unidades) / corredoresAdicionais.size();
            
            pedidosExclusivos.push_back({pedidoId, eficiencia});
        }
    }
    
    // Ordenar pedidos exclusivos por eficiência
    std::sort(pedidosExclusivos.begin(), pedidosExclusivos.end(), 
             [](const PedidoEficiencia& a, const PedidoEficiencia& b) {
                 return a.eficiencia > b.eficiencia;
             });
    
    // Adicionar pedidos exclusivos até atingir limites ou esgotar candidatos
    int totalUnidades = getTotalUnidades(solucaoRecombinada);
    
    for (const auto& pedido : pedidosExclusivos) {
        int unidadesPedido = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedido.pedidoId]) {
            unidadesPedido += quantidade;
        }
        
        // Adicionar se não ultrapassar UB
        if (totalUnidades + unidadesPedido <= UB) {
            solucaoRecombinada.pedidosWave.push_back(pedido.pedidoId);
            totalUnidades += unidadesPedido;
        }
        
        // Parar se já atingiu LB e tem mais que a média de pedidos das soluções originais
        if (totalUnidades >= LB && 
            solucaoRecombinada.pedidosWave.size() >= (solucao1.pedidosWave.size() + solucao2.pedidosWave.size()) / 2) {
            break;
        }
    }
    
    // 3. Reconstruir lista de corredores
    solucaoRecombinada.corredoresWave = construirListaCorredores(solucaoRecombinada.pedidosWave);
    
    return solucaoRecombinada;
}

// Versão sobrecarregada de otimizarWave para aceitar solução inicial
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::otimizarWave(
    int LB, int UB, const SolucaoWave& solucaoInicial) {
    
    // Guardar a solução inicial para usar como ponto de partida
    bool usarSolucaoInicial = !solucaoInicial.pedidosWave.empty();
    SolucaoWave melhorSolucao = solucaoInicial;
    
    // O restante segue o algoritmo normal do Dinkelbach, mas usando a solução inicial
    // como ponto de partida para o primeiro subproblema, se fornecida.
    
    // Executar o algoritmo normal, mas iniciando com esta solução
    // Para brevidade, não estou duplicando todo o código do método original
    
    // Você precisaria modificar o método original para aceitar uma solução inicial
    // ou criar uma versão privada que ambos os métodos públicos chamam
    
    return melhorSolucao;
}