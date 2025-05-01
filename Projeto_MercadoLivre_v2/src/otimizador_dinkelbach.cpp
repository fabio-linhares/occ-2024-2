#include "otimizador_dinkelbach.h"
#include <chrono>
#include <iostream>
#include <iomanip>

OtimizadorDinkelbach::OtimizadorDinkelbach(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador
) : 
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    epsilon_(0.0001),
    maxIteracoes_(100),
    usarBranchAndBound_(true) {
}

void OtimizadorDinkelbach::configurarParametros(double epsilon, int maxIteracoes, bool usarBranchAndBound) {
    epsilon_ = epsilon;
    maxIteracoes_ = maxIteracoes;
    usarBranchAndBound_ = usarBranchAndBound;
}

OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::otimizarWave(int LB, int UB) {
    // Inicializar dados para rastrear convergência
    infoConvergencia_.valoresLambda.clear();
    infoConvergencia_.valoresObjetivo.clear();
    infoConvergencia_.iteracoesRealizadas = 0;
    infoConvergencia_.convergiu = false;
    
    auto inicioTempo = std::chrono::high_resolution_clock::now();
    
    // Verificar se a instância é pequena o suficiente para branch-and-bound
    bool instanciaPequena = backlog_.numPedidos <= 200;
    
    // Inicializar lambda com um valor baixo para focar em maximizar unidades
    double lambda = 0.1;
    
    // Melhor solução encontrada até o momento
    SolucaoWave melhorSolucao;
    melhorSolucao.valorObjetivo = 0;
    
    // Iterações do algoritmo de Dinkelbach
    int iteracao = 0;
    double valorF = 0;  // Numerador (unidades)
    double valorG = 0;  // Denominador (corredores)
    
    while (iteracao < maxIteracoes_) {
        // Resolver o subproblema linearizado F(x) - lambda*G(x)
        std::pair<SolucaoWave, double> resultado;
        
        if (usarBranchAndBound_ && instanciaPequena) {
            // Usar branch-and-bound para instâncias pequenas
            resultado = resolverSubproblemaComBranchAndBound(lambda, LB, UB);
        } else {
            // Usar heurística gulosa para instâncias grandes
            resultado = resolverSubproblemaComHeuristica(lambda, LB, UB);
        }
        
        SolucaoWave solucaoAtual = resultado.first;
        double valorQ = resultado.second;
        
        // Calcular F(x) (unidades) e G(x) (corredores)
        valorF = 0;
        for (int pedidoId : solucaoAtual.pedidosWave) {
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                valorF += quantidade;
            }
        }
        
        valorG = solucaoAtual.corredoresWave.size();
        
        // Atualizar lambda
        double novoLambda = valorF / valorG;
        
        // Registrar valores para rastreamento de convergência
        infoConvergencia_.valoresLambda.push_back(lambda);
        infoConvergencia_.valoresObjetivo.push_back(novoLambda);
        
        // Verificar convergência
        if (std::abs(novoLambda - lambda) < epsilon_) {
            // Solução convergiu
            infoConvergencia_.convergiu = true;
            
            // Atualizar solução final
            solucaoAtual.valorObjetivo = novoLambda;
            melhorSolucao = solucaoAtual;
            
            break;
        }
        
        // Verificar se a solução atual é melhor
        if (novoLambda > melhorSolucao.valorObjetivo) {
            solucaoAtual.valorObjetivo = novoLambda;
            melhorSolucao = solucaoAtual;
        }
        
        // Atualizar lambda para próxima iteração
        lambda = novoLambda;
        
        iteracao++;
    }
    
    // Registrar informações finais sobre convergência
    infoConvergencia_.iteracoesRealizadas = iteracao + 1;
    
    auto fimTempo = std::chrono::high_resolution_clock::now();
    infoConvergencia_.tempoTotal = std::chrono::duration<double>(fimTempo - inicioTempo).count();
    
    return melhorSolucao;
}

std::pair<OtimizadorDinkelbach::SolucaoWave, double> 
OtimizadorDinkelbach::resolverSubproblemaComBranchAndBound(double lambda, int LB, int UB) {
    // Limitar o tempo para no máximo 2 segundos para instâncias pequenas
    // e 1 segundo para instâncias maiores (para evitar explosão combinatória)
    double limiteTempoSegundos = backlog_.numPedidos > 100 ? 1.0 : 2.0;
    
    // Selecionar estratégia com base no tamanho da instância
    BranchAndBoundSolver::EstrategiaSelecionarVariavel estrategia;
    
    if (backlog_.numPedidos <= 50) {
        // Para instâncias muito pequenas, podemos usar estratégias mais sofisticadas
        estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::PSEUDO_CUSTO;
    } else if (backlog_.numPedidos <= 100) {
        // Para instâncias médias, usar abordagem de maior impacto
        estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MAIOR_IMPACTO;
    } else {
        // Para instâncias maiores, usar a estratégia most-infeasible que geralmente
        // causa mais podas no início da árvore de busca
        estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MOST_INFEASIBLE;
    }
    
    // Criar o solver com as configurações escolhidas
    BranchAndBoundSolver solver(
        deposito_, 
        backlog_, 
        localizador_, 
        verificador_, 
        limiteTempoSegundos,
        estrategia
    );
    
    // Configurar uso de cortes
    solver.setUsarCortesCobertura(true);
    solver.setUsarCortesDominancia(true);
    
    // Ajustar coeficiente de limite com base na iteração
    // Nas primeiras iterações, ser mais otimista
    if (infoConvergencia_.valoresLambda.size() < 3) {
        solver.setCoeficienteLimite(0.5); // Mais otimista
    } else {
        solver.setCoeficienteLimite(0.7); // Mais conservador
    }
    
    // Resolver usando branch-and-bound
    auto solucao = solver.resolver(lambda, LB, UB);
    
    // Converter para formato do OtimizadorDinkelbach
    SolucaoWave resultado;
    resultado.pedidosWave = solucao.pedidosWave;
    resultado.corredoresWave = solucao.corredoresWave;
    
    return {resultado, solucao.valorObjetivo};
}

std::pair<OtimizadorDinkelbach::SolucaoWave, double>
OtimizadorDinkelbach::resolverSubproblemaComHeuristica(double lambda, int LB, int UB) {
    // Este método implementa uma heurística gulosa para resolver o subproblema
    // linearizado F(x) - lambda*G(x)
    
    // Estrutura para armazenar a pontuação de cada pedido
    struct PedidoPontuado {
        int pedidoId;
        double pontuacao;
        int unidades;
        std::unordered_set<int> corredores;
    };
    
    std::vector<PedidoPontuado> pedidosPontuados;
    
    // Calcular pontuação para cada pedido
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        // Verificar disponibilidade
        if (!verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
            continue;
        }
        
        PedidoPontuado pedido;
        pedido.pedidoId = pedidoId;
        pedido.unidades = 0;
        pedido.corredores.clear();
        
        // Calcular unidades e corredores para o pedido
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            pedido.unidades += quantidade;
            
            // Adicionar corredores necessários
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                pedido.corredores.insert(corredorId);
            }
        }
        
        // Calcular pontuação: unidades - lambda * corredores
        pedido.pontuacao = pedido.unidades - lambda * pedido.corredores.size();
        
        // Adicionar à lista se a pontuação for positiva
        if (pedido.pontuacao > 0) {
            pedidosPontuados.push_back(pedido);
        }
    }
    
    // Ordenar pedidos por pontuação decrescente
    std::sort(pedidosPontuados.begin(), pedidosPontuados.end(),
              [](const PedidoPontuado& a, const PedidoPontuado& b) {
                  return a.pontuacao > b.pontuacao;
              });
    
    // Construir solução selecionando pedidos em ordem de pontuação
    SolucaoWave solucao;
    std::unordered_set<int> corredoresUnicos;
    int totalUnidades = 0;
    double valorObjetivo = 0;
    
    for (const auto& pedido : pedidosPontuados) {
        // Verificar se adicionar este pedido não excede o limite superior
        if (totalUnidades + pedido.unidades > UB) {
            continue;
        }
        
        // Adicionar pedido à solução
        solucao.pedidosWave.push_back(pedido.pedidoId);
        totalUnidades += pedido.unidades;
        
        // Adicionar corredores únicos
        for (int corredorId : pedido.corredores) {
            corredoresUnicos.insert(corredorId);
        }
        
        // Verificar se atingimos o limite inferior
        if (totalUnidades >= LB) {
            break;
        }
    }
    
    // Se não atingiu o limite inferior, tentar adicionar mais pedidos
    if (totalUnidades < LB) {
        for (const auto& pedido : pedidosPontuados) {
            // Verificar se o pedido já está na solução
            if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedido.pedidoId) 
                != solucao.pedidosWave.end()) {
                continue;
            }
            
            // Verificar se adicionar este pedido não excede o limite superior
            if (totalUnidades + pedido.unidades > UB) {
                continue;
            }
            
            // Adicionar pedido à solução
            solucao.pedidosWave.push_back(pedido.pedidoId);
            totalUnidades += pedido.unidades;
            
            // Adicionar corredores únicos
            for (int corredorId : pedido.corredores) {
                corredoresUnicos.insert(corredorId);
            }
            
            // Verificar se atingimos o limite inferior
            if (totalUnidades >= LB) {
                break;
            }
        }
    }
    
    // Se ainda não atingiu o limite inferior, a instância pode ser infactível
    if (totalUnidades < LB) {
        // Neste caso, vamos relaxar a restrição de pontuação positiva
        // e adicionar pedidos mesmo com pontuação negativa
        
        std::vector<PedidoPontuado> todosOsPedidos;
        
        for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
            // Verificar se o pedido já está na solução
            if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) 
                != solucao.pedidosWave.end()) {
                continue;
            }
            
            // Verificar disponibilidade
            if (!verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
                continue;
            }
            
            PedidoPontuado pedido;
            pedido.pedidoId = pedidoId;
            pedido.unidades = 0;
            pedido.corredores.clear();
            
            // Calcular unidades para o pedido
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
                pedido.unidades += quantidade;
                
                // Adicionar corredores necessários, considerando os já incluídos
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    if (corredoresUnicos.find(corredorId) == corredoresUnicos.end()) {
                        pedido.corredores.insert(corredorId);
                    }
                }
            }
            
            // Calcular pontuação: unidades/corredores
            pedido.pontuacao = pedido.corredores.empty() ? 
                               pedido.unidades : 
                               pedido.unidades / static_cast<double>(pedido.corredores.size());
            
            todosOsPedidos.push_back(pedido);
        }
        
        // Ordenar pedidos por densidade (unidades/corredores)
        std::sort(todosOsPedidos.begin(), todosOsPedidos.end(),
                  [](const PedidoPontuado& a, const PedidoPontuado& b) {
                      return a.pontuacao > b.pontuacao;
                  });
        
        // Adicionar pedidos até atingir o limite inferior
        for (const auto& pedido : todosOsPedidos) {
            // Verificar se adicionar este pedido não excede o limite superior
            if (totalUnidades + pedido.unidades > UB) {
                continue;
            }
            
            // Adicionar pedido à solução
            solucao.pedidosWave.push_back(pedido.pedidoId);
            totalUnidades += pedido.unidades;
            
            // Adicionar corredores únicos
            for (int corredorId : pedido.corredores) {
                corredoresUnicos.insert(corredorId);
            }
            
            // Verificar se atingimos o limite inferior
            if (totalUnidades >= LB) {
                break;
            }
        }
    }
    
    // Converter conjunto de corredores para vetor
    solucao.corredoresWave.assign(corredoresUnicos.begin(), corredoresUnicos.end());
    
    // Calcular valor objetivo
    valorObjetivo = totalUnidades - lambda * corredoresUnicos.size();
    
    return {solucao, valorObjetivo};
}

double OtimizadorDinkelbach::calcularValorObjetivo(const std::vector<int>& pedidosWave) {
    // Calcular total de unidades
    int totalUnidades = 0;
    for (int pedidoId : pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Calcular número de corredores necessários
    auto corredoresWave = construirListaCorredores(pedidosWave);
    int numCorredores = corredoresWave.size();
    
    // Evitar divisão por zero
    if (numCorredores == 0) {
        return 0;
    }
    
    // Retornar razão unidades/corredores
    return static_cast<double>(totalUnidades) / numCorredores;
}

std::vector<int> OtimizadorDinkelbach::construirListaCorredores(const std::vector<int>& pedidosWave) {
    std::unordered_set<int> corredoresUnicos;
    
    // Para cada pedido na wave
    for (int pedidoId : pedidosWave) {
        // Para cada item no pedido
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            // Adicionar todos os corredores que contêm o item
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresUnicos.insert(corredorId);
            }
        }
    }
    
    // Converter para vetor e retornar
    return std::vector<int>(corredoresUnicos.begin(), corredoresUnicos.end());
}

const OtimizadorDinkelbach::InfoConvergencia& OtimizadorDinkelbach::getInfoConvergencia() const {
    return infoConvergencia_;
}

void OtimizadorDinkelbach::exibirDetalhesConvergencia() const {
    std::cout << "\n=== Detalhes de Convergência do Algoritmo de Dinkelbach ===\n";
    std::cout << "Iterações realizadas: " << infoConvergencia_.iteracoesRealizadas << "\n";
    std::cout << "Convergiu: " << (infoConvergencia_.convergiu ? "Sim" : "Não") << "\n";
    std::cout << "Tempo total: " << std::fixed << std::setprecision(4) 
              << infoConvergencia_.tempoTotal << " segundos\n\n";
    
    std::cout << "Histórico de Iterações:\n";
    std::cout << std::setw(10) << "Iteração" << std::setw(15) << "Lambda" 
              << std::setw(20) << "Valor Objetivo" << std::setw(15) << "Delta Lambda" << "\n";
    
    for (size_t i = 0; i < infoConvergencia_.valoresLambda.size(); i++) {
        double deltaLambda = (i > 0) ? 
            std::abs(infoConvergencia_.valoresLambda[i] - infoConvergencia_.valoresLambda[i-1]) : 0;
            
        std::cout << std::setw(10) << i 
                  << std::setw(15) << std::fixed << std::setprecision(6) << infoConvergencia_.valoresLambda[i]
                  << std::setw(20) << std::fixed << std::setprecision(6) << infoConvergencia_.valoresObjetivo[i]
                  << std::setw(15) << std::fixed << std::setprecision(6) << deltaLambda
                  << "\n";
    }
    
    std::cout << "\n";
}