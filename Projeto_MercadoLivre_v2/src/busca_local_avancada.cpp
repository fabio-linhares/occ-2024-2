#include "busca_local_avancada.h"
#include "verificador_disponibilidade.h"
#include "localizador_itens.h"
#include "armazem.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <set>
#include <numeric> // For std::iota



/**
 * @brief Key struct for Tabu List used in the Tabu Search algorithm.
 * 
 * This struct represents a unique key for a movement in the Tabu list.
 * It combines movement type and affected pedidos (orders) to identify movements.
 */
struct MovimentoTabuKey {
    BuscaLocalAvancada::TipoMovimento tipo;
    std::vector<int> pedidosAdd; // Must be sorted before using as key
    std::vector<int> pedidosRem; // Must be sorted before using as key

    /**
     * @brief Equality operator for comparing tabu keys
     * Assumes vectors are sorted before comparison
     */
    bool operator==(const MovimentoTabuKey& other) const {
        return tipo == other.tipo && 
               pedidosAdd == other.pedidosAdd && 
               pedidosRem == other.pedidosRem;
    }
};

// Hash function for MovimentoTabuKey
namespace std {
    template <>
    struct hash<MovimentoTabuKey> {
        size_t operator()(const MovimentoTabuKey& k) const {
            size_t h = std::hash<int>()(static_cast<int>(k.tipo));
            for (int p : k.pedidosAdd) {
                h ^= std::hash<int>()(p) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            for (int p : k.pedidosRem) {
                h ^= std::hash<int>()(p) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
    };
}

BuscaLocalAvancada::BuscaLocalAvancada(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador,
    double limiteTempo
) :
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    limiteTempo_(limiteTempo),
    rng_(std::random_device{}())
{
    // Default configurations
    configTabu_ = ConfigTabu{};
    configVNS_ = ConfigVNS{};
    configILS_ = ConfigILS{};

    // Initialize statistics
    iniciarEstatisticas(Solucao{}); // Initialize with empty solution stats

    inicializarMemoriaLongoPrazo(backlog_.numPedidos);
}

void BuscaLocalAvancada::iniciarEstatisticas(const Solucao& solucaoInicial) {
    estatisticas_ = Estatisticas{};
    estatisticas_.valorObjetivoInicial = solucaoInicial.valorObjetivo;
    estatisticas_.melhorValorObjetivo = solucaoInicial.valorObjetivo;
    estatisticas_.iteracoesTotais = 0;
    estatisticas_.melhorias = 0;
    estatisticas_.movimentosGerados = 0;
    estatisticas_.movimentosAplicados = 0;
    estatisticas_.movimentosAceitos = 0;
    estatisticas_.movimentosRejeitados = 0;
    estatisticas_.movimentosTabu = 0;
    estatisticas_.aspiracoesSucedidas = 0;
    estatisticas_.iteracoesIntensificacao = 0;
    estatisticas_.iteracoesDiversificacao = 0;
    estatisticas_.mudancasVizinhanca = 0;
    estatisticas_.shakesSucedidos = 0;
    estatisticas_.perturbacoes = 0;
    estatisticas_.buscasLocais = 0;
    tempoInicio_ = std::chrono::high_resolution_clock::now();
}

BuscaLocalAvancada::Solucao BuscaLocalAvancada::otimizar(
    const Solucao& solucaoInicial,
    int LB, int UB,
    TipoBuscaLocal tipoBusca
) {
    iniciarEstatisticas(solucaoInicial);
    Solucao resultado;
    
    switch (tipoBusca) {
        case TipoBuscaLocal::BUSCA_TABU:
            estatisticas_.algoritmoUsado = "Busca Tabu";
            resultado = buscaTabu(solucaoInicial, LB, UB);
            break;
        case TipoBuscaLocal::VNS:
            estatisticas_.algoritmoUsado = "VNS";
            resultado = vns(solucaoInicial, LB, UB);
            break;
        case TipoBuscaLocal::ILS:
            estatisticas_.algoritmoUsado = "ILS";
            resultado = ils(solucaoInicial, LB, UB);
            break;
        default:
            estatisticas_.algoritmoUsado = "Busca Tabu (padrão)";
            resultado = buscaTabu(solucaoInicial, LB, UB);
            break;
    }
    
    // Atualizar estatísticas de tempo
    auto fimTempo = std::chrono::high_resolution_clock::now();
    estatisticas_.tempoTotalMs = std::chrono::duration<double, std::milli>(fimTempo - tempoInicio_).count();
    estatisticas_.tempoExecucaoMs = estatisticas_.tempoTotalMs; // Manter compatibilidade
    
    // Calcular melhoria percentual
    if (estatisticas_.valorObjetivoInicial > 0) {
        estatisticas_.melhoria = ((estatisticas_.melhorValorObjetivo - estatisticas_.valorObjetivoInicial) / 
                                estatisticas_.valorObjetivoInicial) * 100.0;
    }
    
    return resultado;
}

std::string BuscaLocalAvancada::obterEstatisticas() const {
    std::stringstream ss;
    ss << "=== Estatísticas da Busca Local ===\n";
    ss << "Algoritmo: " << estatisticas_.algoritmoUsado << "\n";
    ss << "Iterações totais: " << estatisticas_.iteracoesTotais << "\n";
    ss << "Melhorias encontradas: " << estatisticas_.melhorias << "\n";
    ss << "Valor objetivo inicial: " << std::fixed << std::setprecision(4) 
       << estatisticas_.valorObjetivoInicial << "\n";
    ss << "Melhor valor objetivo: " << std::fixed << std::setprecision(4) 
       << estatisticas_.melhorValorObjetivo << "\n";
    ss << "Melhoria: " << std::fixed << std::setprecision(2) 
       << estatisticas_.melhoria << "%\n";
    ss << "Tempo de execução: " << estatisticas_.tempoExecucaoMs << " ms\n";
    
    // Estatísticas específicas por algoritmo
    if (estatisticas_.algoritmoUsado == "Busca Tabu") {
        ss << "Movimentos gerados: " << estatisticas_.movimentosGerados << "\n";
        ss << "Movimentos aplicados: " << estatisticas_.movimentosAplicados << "\n";
        ss << "Movimentos aceitos: " << estatisticas_.movimentosAceitos << "\n";
        ss << "Movimentos rejeitados (Tabu): " << estatisticas_.movimentosRejeitados << "\n";
        ss << "Movimentos Tabu (considerados): " << estatisticas_.movimentosTabu << "\n";
        ss << "Aspirações Sucedidas: " << estatisticas_.aspiracoesSucedidas << "\n";
        ss << "Iterações Intensificação: " << estatisticas_.iteracoesIntensificacao << "\n";
        ss << "Iterações Diversificação: " << estatisticas_.iteracoesDiversificacao << "\n";
    } else if (estatisticas_.algoritmoUsado == "VNS") {
        ss << "Mudanças de Vizinhança: " << estatisticas_.mudancasVizinhanca << "\n";
        ss << "'Shakes' Sucedidos: " << estatisticas_.shakesSucedidos << "\n";
    } else if (estatisticas_.algoritmoUsado == "ILS") {
        ss << "Perturbações realizadas: " << estatisticas_.perturbacoes << "\n";
        ss << "Buscas Locais realizadas: " << estatisticas_.buscasLocais << "\n";
    }
    
    return ss.str();
}

bool BuscaLocalAvancada::tempoExcedido() const {
    auto agora = std::chrono::high_resolution_clock::now();
    double tempoDecorrido = std::chrono::duration<double>(agora - tempoInicio_).count();
    
    // Dar uma margem de segurança (90% do tempo total)
    return tempoDecorrido > (limiteTempo_ * 0.9);
}

// --- Tabu Search Implementation ---
BuscaLocalAvancada::Solucao BuscaLocalAvancada::buscaTabu(const Solucao& solucaoInicial, int LB, int UB) {
    Solucao solucaoAtual = solucaoInicial;
    Solucao melhorSolucao = solucaoInicial;

    std::unordered_map<MovimentoTabuKey, int> listaTabu; // Key -> iteration when tabu expires
    int iteracao = 0;
    int iteracoesSemMelhoria = 0;
    bool modoIntensificacao = false;
    bool modoDiversificacao = false;
    while (iteracao < configTabu_.maxIteracoes && !tempoExcedido()) {
        estatisticas_.iteracoesTotais++;
        iteracao++;

        std::vector<Movimento> vizinhanca;
        if (modoIntensificacao) {
             vizinhanca = gerarMovimentosIntensificacao(solucaoAtual, LB, UB);
             estatisticas_.iteracoesIntensificacao++;
        } else if (modoDiversificacao) {
             vizinhanca = gerarMovimentosDiversificacao(solucaoAtual, LB, UB);
             estatisticas_.iteracoesDiversificacao++;
        } else {
             vizinhanca = gerarVizinhanca(solucaoAtual, LB, UB, 0); // Standard neighborhood
        }
        
        estatisticas_.movimentosGerados += vizinhanca.size();


        Movimento melhorMovimento;
        double melhorDelta = -std::numeric_limits<double>::infinity();
        bool encontrouNaoTabu = false;

        // Pre-sort frequently accessed collections to avoid repeated sorting
        std::vector<std::pair<double, Movimento>> candidatosPromissores;
        candidatosPromissores.reserve(std::min(50, static_cast<int>(vizinhanca.size())));

        for (const auto& movimento : vizinhanca) {
            // Quick feasibility check before more expensive operations
            double deltaEstimado = movimento.deltaValorObjetivo;
            if (deltaEstimado <= -std::numeric_limits<double>::infinity()) continue;

            // Create tabu key only for potentially good moves
            if (deltaEstimado > melhorDelta || deltaEstimado > 0) {
            // Check if move is tabu
            MovimentoTabuKey key;
            key.tipo = movimento.tipo;
            key.pedidosAdd = movimento.pedidosAdicionar;
            key.pedidosRem = movimento.pedidosRemover;
            std::sort(key.pedidosAdd.begin(), key.pedidosAdd.end());
            std::sort(key.pedidosRem.begin(), key.pedidosRem.end());

            bool isTabu = listaTabu.count(key) && listaTabu[key] > iteracao;
            if (isTabu) estatisticas_.movimentosTabu++;

            // Check aspiration criterion - if better than best solution so far
            double valorEstimado = solucaoAtual.valorObjetivo + deltaEstimado;
            bool aspiracao = valorEstimado > melhorSolucao.valorObjetivo;
            if (aspiracao) estatisticas_.aspiracoesSucedidas++;

            // Accept if not tabu or aspiration criteria met
            if (!isTabu || aspiracao) {
                // Store promising candidates for full evaluation
                candidatosPromissores.emplace_back(deltaEstimado, movimento);
            }
            }
        }

        // Sort candidates by descending delta value
        std::sort(candidatosPromissores.begin(), candidatosPromissores.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });

        // Evaluate top candidates with full recalculation (limit to top 10 for efficiency)
        int candidatosAvaliados = 0;
        for (const auto& [delta, movimento] : candidatosPromissores) {
            if (candidatosAvaliados++ >= 10) break;

            // Full evaluation with viability check
            Solucao solucaoVizinha = aplicarMovimento(solucaoAtual, movimento);
            if (!solucaoViavel(solucaoVizinha, LB, UB)) continue;
            
            recalcularSolucao(solucaoVizinha);
            double deltaAtual = solucaoVizinha.valorObjetivo - solucaoAtual.valorObjetivo;

            if (deltaAtual > melhorDelta) {
            melhorDelta = deltaAtual;
            melhorMovimento = movimento;
            encontrouNaoTabu = true;
            }
        }

        if (melhorDelta > -std::numeric_limits<double>::infinity()) { // Found a valid move
            estatisticas_.movimentosAceitos++;
            solucaoAtual = aplicarMovimento(solucaoAtual, melhorMovimento);
            recalcularSolucao(solucaoAtual); // Update units, corridors, objective

            // Update Tabu List
            MovimentoTabuKey key;
            key.tipo = melhorMovimento.tipo;
            key.pedidosAdd = melhorMovimento.pedidosAdicionar;
            key.pedidosRem = melhorMovimento.pedidosRemover;
            std::sort(key.pedidosAdd.begin(), key.pedidosAdd.end());
            std::sort(key.pedidosRem.begin(), key.pedidosRem.end());
            int duracaoTabu = configTabu_.duracaoTabuBase + (rng_() % 5); // Add randomness
            listaTabu[key] = iteracao + duracaoTabu;

            // Update best solution
            if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                melhorSolucao = solucaoAtual;
                iteracoesSemMelhoria = 0;
                modoIntensificacao = false; // Exit special modes on improvement
                modoDiversificacao = false;
                estatisticas_.melhorias++;
                // std::cout << "Nova melhor solucao (Tabu) it " << iteracao << ": BOV = " << melhorSolucao.valorObjetivo << std::endl;
            } else {
                iteracoesSemMelhoria++;
            }

             // Update long-term memory (frequency/recency) - simplified
             for(int p : melhorMovimento.pedidosAdicionar) frequenciaPedidos_[p]++;
             for(int p : melhorMovimento.pedidosRemover) frequenciaPedidos_[p]++; // Count participation


        } else {
            // No improving or non-tabu move found
            iteracoesSemMelhoria++;
            estatisticas_.movimentosRejeitados++; // Count as rejected if no move made
        }

        // Clean up old entries in Tabu List (optional, prevents unbounded growth)
        if (iteracao % 50 == 0) {
             for (auto it = listaTabu.begin(); it != listaTabu.end(); ) {
                 if (it->second <= iteracao) {
                     it = listaTabu.erase(it);
                 } else {
                     ++it;
                 }
             }
        }


        // Intensification / Diversification logic
        if (!modoIntensificacao && !modoDiversificacao) {
            if (iteracoesSemMelhoria >= configTabu_.maxIteracoesSemMelhoria) {
                 // Trigger diversification
                 modoDiversificacao = true;
                 solucaoAtual = aplicarPerturbacaoForte(melhorSolucao, LB, UB); // Perturb from best known
                 recalcularSolucao(solucaoAtual);
                 iteracoesSemMelhoria = 0; // Reset counter after diversification
                 // std::cout << "Diversificando..." << std::endl;
            }
        } else if (modoDiversificacao) {
             if (iteracoesSemMelhoria >= configTabu_.ciclosDiversificacao) { // Stay in diversification for a few cycles
                 modoDiversificacao = false;
                 modoIntensificacao = true; // Switch to intensification
                 solucaoAtual = melhorSolucao; // Restart intensification from best known
                 iteracoesSemMelhoria = 0;
                 // std::cout << "Intensificando..." << std::endl;
             }
        } else if (modoIntensificacao) {
             if (iteracoesSemMelhoria >= configTabu_.ciclosIntensificacao) { // Stay in intensification for a few cycles
                 modoIntensificacao = false; // Exit intensification
                 iteracoesSemMelhoria = 0; // Reset counter
             }
        }

    } // End main loop

    return melhorSolucao;
}

// --- VNS Implementation ---
BuscaLocalAvancada::Solucao BuscaLocalAvancada::vns(const Solucao& solucaoInicial, int LB, int UB) {
     Solucao solucaoAtual = solucaoInicial;
     Solucao melhorSolucao = solucaoInicial;
     int k = 0; // Start with first neighborhood structure
     int iter = 0;

     while (iter < configVNS_.maxIteracoes && !tempoExcedido()) {
         estatisticas_.iteracoesTotais++;
         iter++;

         // 1. Shaking: Generate a random solution s' from the k-th neighborhood Nk(s)
         Solucao solucaoAposShake = perturbarSolucao(solucaoAtual, configVNS_.intensidadeShakeBase * (k + 1), LB, UB); // Intensity increases with k
         estatisticas_.perturbacoes++; // Count shakes as perturbations

         // 2. Local Search: Apply local search to s' to find a local optimum s''
         Solucao solucaoAposLS = buscaLocalBasica(solucaoAposShake, 0, LB, UB); // Use basic neighborhood (e.g., add/remove) for LS
         estatisticas_.buscasLocais++;

         // 3. Move or not: If s'' is better than s, move there and reset k; otherwise, increment k
         if (solucaoAposLS.valorObjetivo > solucaoAtual.valorObjetivo) {
             solucaoAtual = solucaoAposLS;
             k = 0; // Reset neighborhood index
             estatisticas_.shakesSucedidos++;
             if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                 melhorSolucao = solucaoAtual;
                 estatisticas_.melhorias++;
                 // std::cout << "Nova melhor solucao (VNS) it " << iter << ": BOV = " << melhorSolucao.valorObjetivo << std::endl;
             }
         } else {
             k++;
             if (k >= configVNS_.numVizinhancas) {
                 k = 0; // Cycle back if max neighborhood reached
             }
             estatisticas_.mudancasVizinhanca++;
         }
     }
     return melhorSolucao;
}


// --- ILS Implementation ---
BuscaLocalAvancada::Solucao BuscaLocalAvancada::ils(const Solucao& solucaoInicial, int LB, int UB) {
    Solucao melhorSolucao = solucaoInicial;
    Solucao solucaoAtual = solucaoInicial;
    
    // Inicializar estatísticas
    int iterSemMelhoria = 0;
    
    // Executar busca local na solução inicial
    solucaoAtual = buscaLocalBasica(solucaoAtual, 0, LB, UB);
    estatisticas_.buscasLocais++; // Incrementar contador de buscas locais
    
    if (avaliarMovimento(melhorSolucao, {TipoMovimento::SWAP, {}, {}}) < 
        avaliarMovimento(solucaoAtual, {TipoMovimento::SWAP, {}, {}})) {
        melhorSolucao = solucaoAtual;
        estatisticas_.melhorias++;
    }
    
    // Loop principal do ILS
    for (int iter = 0; iter < configILS_.maxIteracoes; iter++) {
        // Incrementar contador de iterações
        estatisticas_.iteracoesTotais++;
        
        if (tempoExcedido()) break;
        
        // Perturbar solução atual
        double intensidadePert = configILS_.intensidadePerturbacaoInicial + 
                               (double)iterSemMelhoria * 0.01;
        Solucao solucaoPerturbada = perturbarSolucao(solucaoAtual, intensidadePert, LB, UB);
        estatisticas_.perturbacoes++; // Incrementar contador de perturbações
        
        // Aplicar busca local na solução perturbada
        Solucao candidata = buscaLocalBasica(solucaoPerturbada, 0, LB, UB);
        estatisticas_.buscasLocais++; // Incrementar contador de buscas locais
        
        // Verificar se encontramos uma solução melhor
        if (candidata.valorObjetivo > melhorSolucao.valorObjetivo) {
            melhorSolucao = candidata;
            solucaoAtual = candidata;
            iterSemMelhoria = 0;
            estatisticas_.melhorias++;
        } else {
            iterSemMelhoria++;
        }
        
        // Reinicialização periódica se estagnado
        if (iterSemMelhoria > configILS_.maxIteracoesSemMelhoria && 
            configILS_.usarReinicioPeriodico) {
            solucaoAtual = aplicarPerturbacaoForte(melhorSolucao, LB, UB);
            estatisticas_.perturbacoes++;
            iterSemMelhoria = 0;
        }
    }
    
    return melhorSolucao;
}


// --- State-of-the-Art Neighborhood Generation ---
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarVizinhanca(
    const Solucao& solucao, int LB, int UB, int tipoVizinhanca)
{
    std::vector<Movimento> vizinhanca;
    // Track currently included and excluded orders
    std::unordered_set<int> pedidosDentro(solucao.pedidosWave.begin(), solucao.pedidosWave.end());
    std::vector<int> pedidosFora;
    
    // Fast construction of candidate list
    pedidosFora.reserve(backlog_.numPedidos - pedidosDentro.size());
    for(int i = 0; i < backlog_.numPedidos; ++i) {
        if(pedidosDentro.find(i) == pedidosDentro.end()) {
            pedidosFora.push_back(i);
        }
    }
    
    // Adaptive neighborhood selection based on search stage
    switch(tipoVizinhanca) {
        case 0: {  // Basic ADD/REMOVE operations
            // Sophisticated sampling for large instances
            const int MAX_CANDIDATES = 200;  // Limit to control computational effort
            bool useSampling = pedidosFora.size() > MAX_CANDIDATES || solucao.pedidosWave.size() > MAX_CANDIDATES;
            
            // Prioritize promising candidates first using frequency/recency memory
            std::vector<std::pair<double, int>> scoredCandidates;
            
            // Score "ADD" candidates
            for (int pedidoAdd : pedidosFora) {
                double score = 1.0;
                // Use tabu memory to prioritize rarely used orders
                if (!frequenciaPedidos_.empty()) {
                    score += (1.0 / (frequenciaPedidos_[pedidoAdd] + 1.0)) * 10.0;
                }
                scoredCandidates.emplace_back(score, pedidoAdd);
            }
            
            // Sort by score and select top candidates if sampling is enabled
            std::sort(scoredCandidates.begin(), scoredCandidates.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
            
            if (useSampling) {
                // Keep some deterministic high-scoring candidates
                int keep = MAX_CANDIDATES * 0.5;
                // Add some random candidates for diversity
                std::vector<std::pair<double, int>> topCandidates(
                    scoredCandidates.begin(),
                    scoredCandidates.begin() + std::min(keep, static_cast<int>(scoredCandidates.size()))
                );
                
                // Randomly sample some additional candidates
                if (scoredCandidates.size() > keep) {
                    std::vector<std::pair<double, int>> remainingCandidates(
                        scoredCandidates.begin() + keep, scoredCandidates.end()
                    );
                    std::shuffle(remainingCandidates.begin(), remainingCandidates.end(), rng_);
                    
                    int additionalSamples = std::min(MAX_CANDIDATES - keep, 
                                                     static_cast<int>(remainingCandidates.size()));
                    topCandidates.insert(topCandidates.end(), 
                                         remainingCandidates.begin(), 
                                         remainingCandidates.begin() + additionalSamples);
                }
                
                scoredCandidates = std::move(topCandidates);
            }
            
            // Generate ADD moves for selected candidates
            for (const auto& [score, pedidoAdd] : scoredCandidates) {
                // Fast feasibility check before creating move
                int unidadesAdicionais = 0;
                for (const auto& [_, quantidade] : backlog_.pedido[pedidoAdd]) {
                    unidadesAdicionais += quantidade;
                }
                
                // Skip if would exceed UB by more than 10%
                if (solucao.totalUnidades + unidadesAdicionais > UB * 1.1) {
                    continue;
                }
                
                // Quick stock availability check
                if (!verificador_.verificarDisponibilidade(backlog_.pedido[pedidoAdd])) {
                    continue;
                }
                
                Movimento m;
                m.tipo = TipoMovimento::ADICIONAR;
                m.pedidosAdicionar = {pedidoAdd};
                m.pedidosRemover = {};
                m.deltaValorObjetivo = avaliarMovimento(solucao, m);
                
                // Prioritize promising moves
                if (m.deltaValorObjetivo > -std::numeric_limits<double>::infinity()) {
                    vizinhanca.push_back(m);
                }
            }
            
            // Similar approach for REMOVE moves
            scoredCandidates.clear();
            for (int pedidoRem : solucao.pedidosWave) {
                double score = 1.0;
                if (!frequenciaPedidos_.empty()) {
                    score += (frequenciaPedidos_[pedidoRem] + 1.0) * 5.0;  // Prefer removing frequently used
                }
                scoredCandidates.emplace_back(score, pedidoRem);
            }
            
            std::sort(scoredCandidates.begin(), scoredCandidates.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
            
            if (useSampling && scoredCandidates.size() > MAX_CANDIDATES) {
                scoredCandidates.resize(MAX_CANDIDATES);
            }
            
            for (const auto& [score, pedidoRem] : scoredCandidates) {
                // Skip if removing would violate LB by more than 10%
                int unidadesRemovidas = 0;
                for (const auto& [_, quantidade] : backlog_.pedido[pedidoRem]) {
                    unidadesRemovidas += quantidade;
                }
                
                if (solucao.totalUnidades - unidadesRemovidas < LB * 0.9) {
                    continue;
                }
                
                Movimento m;
                m.tipo = TipoMovimento::REMOVER;
                m.pedidosAdicionar = {};
                m.pedidosRemover = {pedidoRem};
                m.deltaValorObjetivo = avaliarMovimento(solucao, m);
                
                if (m.deltaValorObjetivo > -std::numeric_limits<double>::infinity()) {
                    vizinhanca.push_back(m);
                }
            }
            break;
        }
        
        case 1: {  // SWAP moves
            auto swapMoves = gerarMovimentosSwap(solucao, LB, UB);
            vizinhanca.insert(vizinhanca.end(), swapMoves.begin(), swapMoves.end());
            break;
        }
        
        case 2: {  // CHAIN_EXCHANGE moves
            auto chainMoves = gerarMovimentosChainExchange(solucao, LB, UB);
            vizinhanca.insert(vizinhanca.end(), chainMoves.begin(), chainMoves.end());
            break;
        }
        
        case 3: {  // Mixed neighborhood with probability-based selection
            // Determine weights based on search history
            double wAdd = 0.3, wRem = 0.2, wSwap = 0.3, wChain = 0.2;
            
            // Adjust weights based on recent success rates (if tracked)
            if (estatisticas_.iteracoesTotais > 50) {
                // Example: If we've had more success with swaps recently, increase their weight
                wSwap *= 1.5;
                double total = wAdd + wRem + wSwap + wChain;
                wAdd /= total;
                wRem /= total;
                wSwap /= total;
                wChain /= total;
            }
            
            // Generate moves based on probability
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double rnd = dist(rng_);
            
            if (rnd < wAdd + wRem) {
                // Generate a smaller basic neighborhood (ADD/REMOVE)
                auto basicMoves = BuscaLocalAvancada::gerarVizinhanca(solucao, LB, UB, 0);
                // Sample subset for efficiency
                const int MAX_BASIC = 100;
                if (basicMoves.size() > MAX_BASIC) {
                    std::shuffle(basicMoves.begin(), basicMoves.end(), rng_);
                    basicMoves.resize(MAX_BASIC);
                }
                vizinhanca.insert(vizinhanca.end(), basicMoves.begin(), basicMoves.end());
            }
            else if (rnd < wAdd + wRem + wSwap) {
                auto swapMoves = gerarMovimentosSwap(solucao, LB, UB);
                vizinhanca.insert(vizinhanca.end(), swapMoves.begin(), swapMoves.end());
            }
            else {
                auto chainMoves = gerarMovimentosChainExchange(solucao, LB, UB);
                vizinhanca.insert(vizinhanca.end(), chainMoves.begin(), chainMoves.end());
            }
            break;
        }
        
        default: {  // Fallback to comprehensive neighborhood
            // Generate all move types with controlled sampling
            auto basicMoves = BuscaLocalAvancada::gerarVizinhanca(solucao, LB, UB, 0);
            auto swapMoves = gerarMovimentosSwap(solucao, LB, UB);
            auto chainMoves = gerarMovimentosChainExchange(solucao, LB, UB);
            
            // Combine and sample if too large
            vizinhanca.reserve(basicMoves.size() + swapMoves.size() + chainMoves.size());
            vizinhanca.insert(vizinhanca.end(), basicMoves.begin(), basicMoves.end());
            vizinhanca.insert(vizinhanca.end(), swapMoves.begin(), swapMoves.end());
            vizinhanca.insert(vizinhanca.end(), chainMoves.begin(), chainMoves.end());
            
            const int MAX_TOTAL = 300;
            if (vizinhanca.size() > MAX_TOTAL) {
                std::shuffle(vizinhanca.begin(), vizinhanca.end(), rng_);
                vizinhanca.resize(MAX_TOTAL);
            }
        }
    }
    
    // Final sorting - prioritize promising moves for evaluation
    std::sort(vizinhanca.begin(), vizinhanca.end(), 
              [](const Movimento& a, const Movimento& b) {
                  return a.deltaValorObjetivo > b.deltaValorObjetivo;
              });
    
    return vizinhanca;
}

// --- Swap Moves ---
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosSwap(
    const Solucao& solucao, int LB, int UB)
{
    std::vector<Movimento> vizinhanca;
    std::vector<int> pedidosFora;
    std::unordered_set<int> pedidosDentro(solucao.pedidosWave.begin(), solucao.pedidosWave.end());

    for(int i=0; i<backlog_.numPedidos; ++i) {
        if(pedidosDentro.find(i) == pedidosDentro.end()) {
            pedidosFora.push_back(i);
        }
    }

    // Try swapping each inside order with each outside order
    for (int pedidoRem : solucao.pedidosWave) {
        for (int pedidoAdd : pedidosFora) {
            Movimento m;
            m.tipo = TipoMovimento::SWAP;
            m.pedidosAdicionar = {pedidoAdd};
            m.pedidosRemover = {pedidoRem};
            m.deltaValorObjetivo = avaliarMovimento(solucao, m); // Needs implementation
            vizinhanca.push_back(m);
        }
    }
    return vizinhanca;
}

// --- Chain Exchange Moves ---
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosChainExchange(
    const Solucao& solucao, int LB, int UB)
{
    std::vector<Movimento> movimentos;
    
    // Se a solução atual não tem pedidos suficientes para chain exchange
    if (solucao.pedidosWave.size() < 2) {
        return movimentos;
    }
    
    // Guardar unidades totais da solução atual
    int totalUnidades = solucao.totalUnidades;
    
    // Considerar até 10 pedidos aleatórios para remoção para limitar o número de movimentos
    int maxPedidosConsiderar = std::min(10, static_cast<int>(solucao.pedidosWave.size()));
    std::vector<int> indices(solucao.pedidosWave.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng_);
    
    // Para cada par de pedidos, gerar movimento de chain exchange (remover 2, adicionar 1-2)
    for (int i = 0; i < maxPedidosConsiderar; i++) {
        for (int j = i + 1; j < maxPedidosConsiderar; j++) {
            int pedido1 = solucao.pedidosWave[indices[i]];
            int pedido2 = solucao.pedidosWave[indices[j]];
            
            // Calcular unidades que serão removidas
            int unidadesRemovidas = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedido1]) {
                unidadesRemovidas += quantidade;
            }
            for (const auto& [_, quantidade] : backlog_.pedido[pedido2]) {
                unidadesRemovidas += quantidade;
            }
            
            // Verificar faixa alvo após remover os pedidos
            int novoTotalAlvo = totalUnidades - unidadesRemovidas;
            
            // Se remover esses pedidos já deixa abaixo do LB, não considere
            if (novoTotalAlvo < LB) {
                continue;
            }
            
            // Considerar adicionar pedidos não incluídos para compensar
            std::vector<int> pedidosCandidatos;
            for (int k = 0; k < backlog_.numPedidos; k++) {
                // Pular pedidos que já estão na wave
                if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), k) != solucao.pedidosWave.end()) {
                    continue;
                }
                
                // Verificar se o pedido é viável considerando estoque
                if (!verificador_.verificarDisponibilidade(backlog_.pedido[k])) {
                    continue;
                }
                
                pedidosCandidatos.push_back(k);
            }
            
            // Limitar número de candidatos para controlar complexidade
            if (pedidosCandidatos.size() > 20) {
                std::shuffle(pedidosCandidatos.begin(), pedidosCandidatos.end(), rng_);
                pedidosCandidatos.resize(20);
            }
            
            // Tentar adicionar até 2 pedidos que satisfaçam as restrições
            for (int k = 0; k < static_cast<int>(pedidosCandidatos.size()); k++) {
                int pedidoAdd1 = pedidosCandidatos[k];
                int unidadesAdd1 = 0;
                
                for (const auto& [_, quantidade] : backlog_.pedido[pedidoAdd1]) {
                    unidadesAdd1 += quantidade;
                }
                
                // Verificar se adicionar um pedido respeita limites
                int novoTotal1 = novoTotalAlvo + unidadesAdd1;
                
                if (novoTotal1 >= LB && novoTotal1 <= UB) {
                    // Criar movimento de chain exchange (2-por-1)
                    Movimento movimento;
                    movimento.tipo = TipoMovimento::CHAIN_EXCHANGE;
                    movimento.pedidosRemover = {pedido1, pedido2};
                    movimento.pedidosAdicionar = {pedidoAdd1};
                    
                    // Calcular delta valor objetivo (estimativa simplificada)
                    double deltaValor = avaliarMovimento(solucao, movimento);
                    movimento.deltaValorObjetivo = deltaValor;
                    
                    if (deltaValor > 0) {
                        movimentos.push_back(movimento);
                    }
                }
                
                // Tentar adicionar um segundo pedido se ainda não atingiu o limite
                for (int l = k + 1; l < static_cast<int>(pedidosCandidatos.size()); l++) {
                    int pedidoAdd2 = pedidosCandidatos[l];
                    int unidadesAdd2 = 0;
                    
                    for (const auto& [_, quantidade] : backlog_.pedido[pedidoAdd2]) {
                        unidadesAdd2 += quantidade;
                    }
                    
                    // Verificar se adicionar dois pedidos respeita limites
                    int novoTotal2 = novoTotalAlvo + unidadesAdd1 + unidadesAdd2;
                    
                    if (novoTotal2 >= LB && novoTotal2 <= UB) {
                        // Criar movimento de chain exchange (2-por-2)
                        Movimento movimento;
                        movimento.tipo = TipoMovimento::CHAIN_EXCHANGE;
                        movimento.pedidosRemover = {pedido1, pedido2};
                        movimento.pedidosAdicionar = {pedidoAdd1, pedidoAdd2};
                        
                        // Calcular delta valor objetivo
                        double deltaValor = avaliarMovimento(solucao, movimento);
                        movimento.deltaValorObjetivo = deltaValor;
                        
                        if (deltaValor > 0) {
                            movimentos.push_back(movimento);
                        }
                    }
                }
            }
        }
    }
    
    return movimentos;
}

// --- Apply Move ---
BuscaLocalAvancada::Solucao BuscaLocalAvancada::aplicarMovimento(
    const Solucao& solucao, const Movimento& movimento)
{
    Solucao novaSolucao = solucao; // Copy base solution
    std::unordered_set<int> pedidosSet(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end());

    // Remove pedidos
    for (int pRem : movimento.pedidosRemover) {
        pedidosSet.erase(pRem);
    }
    // Add pedidos
    for (int pAdd : movimento.pedidosAdicionar) {
        pedidosSet.insert(pAdd);
    }

    // Update vector
    novaSolucao.pedidosWave.assign(pedidosSet.begin(), pedidosSet.end());
    // Corridors and objective will be updated by recalcularSolucao

    return novaSolucao;
}

// --- Evaluate Move (Efficient Delta Calculation - Simplified Example) ---
// A proper implementation requires calculating change in units and change in corridors efficiently
double BuscaLocalAvancada::avaliarMovimento(
    const Solucao& solucao, const Movimento& movimento)
{
    // --- THIS IS A PLACEHOLDER - NEEDS EFFICIENT DELTA CALCULATION ---
    // Calculate the objective of the potential new solution and find the difference.
    // This is inefficient but correct for now.
    Solucao vizinha = aplicarMovimento(solucao, movimento);
    // Check basic viability (LB/UB might be violated, stock check is harder here)
    int tempUnidades = 0;
    for(int p : vizinha.pedidosWave) {
        for(const auto& item : backlog_.pedido[p]) tempUnidades += item.second;
    }
    // if (tempUnidades < LB || tempUnidades > UB) return -std::numeric_limits<double>::infinity(); // Infeasible

    recalcularSolucao(vizinha); // Calculate objective fully
    if (solucao.valorObjetivo == -std::numeric_limits<double>::infinity()) return vizinha.valorObjetivo; // Handle initial case
    return vizinha.valorObjetivo - solucao.valorObjetivo;
    // --- END PLACEHOLDER ---
}


// --- Recalculate Solution Properties ---
void BuscaLocalAvancada::recalcularSolucao(Solucao& solucao) {
    // Limpar corredores atuais
    solucao.corredoresWave.clear();
    
    // Recalcular unidades totais e corredores necessários
    solucao.totalUnidades = 0;
    std::unordered_set<int> corredoresSet;
    
    for (int pedidoId : solucao.pedidosWave) {
        if (pedidoId < 0 || pedidoId >= backlog_.numPedidos) continue;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            solucao.totalUnidades += quantidade;
            
            // Adicionar corredores necessários para este item
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresSet.insert(corredorId);
            }
        }
    }
    
    // Atualizar vetor de corredores
    solucao.corredoresWave.assign(corredoresSet.begin(), corredoresSet.end());
    
    // Recalcular valor objetivo
    if (!solucao.corredoresWave.empty()) {
        solucao.valorObjetivo = static_cast<double>(solucao.totalUnidades) / solucao.corredoresWave.size();
    } else {
        solucao.valorObjetivo = 0.0;
    }
}

// --- Check Solution Viability ---
bool BuscaLocalAvancada::solucaoViavel(const Solucao& solucao, int LB, int UB) const {
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
         if (pedidoId < 0 || pedidoId >= backlog_.numPedidos) return false; // Invalid ID
         for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
             totalUnidades += quantidade;
         }
    }

    if (totalUnidades < LB || totalUnidades > UB) {
        return false;
    }

    // Check stock
    if (!verificador_.verificarDisponibilidadeConjunto(solucao.pedidosWave, backlog_)) {
        return false;
    }

    return true;
}

// --- Perturb Solution (for ILS/VNS) ---
BuscaLocalAvancada::Solucao BuscaLocalAvancada::perturbarSolucao(
    const Solucao& solucao, double intensidade, int LB, int UB)
{
    Solucao novaSolucao = solucao;
    if (novaSolucao.pedidosWave.empty()) return novaSolucao; // Cannot perturb empty solution easily

    int numPedidos = novaSolucao.pedidosWave.size();
    int numRemover = std::min(numPedidos, std::max(1, static_cast<int>(numPedidos * intensidade * 0.1))); // Remove 0-10% based on intensity
    int numAdicionar = std::max(1, static_cast<int>(backlog_.numPedidos * intensidade * 0.05)); // Add 0-5% based on intensity

    std::shuffle(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end(), rng_);

    // Remove random orders
    std::vector<int> pedidosRemovidos;
    for(int i=0; i<numRemover; ++i) {
        pedidosRemovidos.push_back(novaSolucao.pedidosWave.back());
        novaSolucao.pedidosWave.pop_back();
    }

    // Add random orders (that are not already in)
    std::vector<int> todosPedidos(backlog_.numPedidos);
    std::iota(todosPedidos.begin(), todosPedidos.end(), 0);
    std::shuffle(todosPedidos.begin(), todosPedidos.end(), rng_);
    std::unordered_set<int> pedidosAtuais(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end());

    int adicionados = 0;
    for(int p : todosPedidos) {
        if(adicionados >= numAdicionar) break;
        if(pedidosAtuais.find(p) == pedidosAtuais.end()) {
             // Basic check if adding might violate UB drastically
             int unidadesPedido = 0;
             for(const auto& item : backlog_.pedido[p]) unidadesPedido += item.second;
             if (novaSolucao.totalUnidades + unidadesPedido <= UB * 1.1) { // Allow slight overshoot
                 novaSolucao.pedidosWave.push_back(p);
                 adicionados++;
             }
        }
    }

    recalcularSolucao(novaSolucao); // Update properties
    // Ensure viability (simple repair: if outside LB/UB, might revert or do more complex repair)
    if (novaSolucao.totalUnidades < LB || novaSolucao.totalUnidades > UB) {
         // Simple fallback: return original solution if perturbation failed viability
         // A better approach would be a repair heuristic.
         return solucao;
    }


    return novaSolucao;
}

// --- Apply Strong Perturbation ---
BuscaLocalAvancada::Solucao BuscaLocalAvancada::aplicarPerturbacaoForte(const Solucao& solucao, int LB, int UB) {
    // Cria uma cópia da solução original
    Solucao novaSolucao = solucao;
    
    // Define o número de pedidos a remover (30-50% dos pedidos atuais)
    std::uniform_int_distribution<int> dist(
        std::max(1, static_cast<int>(solucao.pedidosWave.size() * 0.3)),
        std::max(2, static_cast<int>(solucao.pedidosWave.size() * 0.5))
    );
    
    int numRemover = dist(rng_);
    numRemover = std::min(numRemover, static_cast<int>(novaSolucao.pedidosWave.size()));
    
    // Remover pedidos aleatoriamente
    for (int i = 0; i < numRemover; i++) {
        if (novaSolucao.pedidosWave.empty()) break;
        
        std::uniform_int_distribution<int> idxDist(0, novaSolucao.pedidosWave.size() - 1);
        int idx = idxDist(rng_);
        
        novaSolucao.pedidosWave.erase(novaSolucao.pedidosWave.begin() + idx);
    }
    
    // Adicionar novos pedidos
    std::vector<int> pedidosCandidatos;
    for (int i = 0; i < backlog_.numPedidos; i++) {
        // Verificar se o pedido já está na solução
        if (std::find(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end(), i) != novaSolucao.pedidosWave.end()) {
            continue;
        }
        
        // Verificar disponibilidade
        if (verificador_.verificarDisponibilidade(backlog_.pedido[i])) {
            pedidosCandidatos.push_back(i);
        }
    }
    
    // Embaralhar candidatos
    std::shuffle(pedidosCandidatos.begin(), pedidosCandidatos.end(), rng_);
    
    // Adicionar pedidos até atingir LB ou até não haver mais candidatos
    recalcularSolucao(novaSolucao);
    
    while (!pedidosCandidatos.empty() && novaSolucao.totalUnidades < LB) {
        int pedidoId = pedidosCandidatos.back();
        pedidosCandidatos.pop_back();
        
        // Calcular unidades adicionais
        int unidadesAdicionais = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesAdicionais += quantidade;
        }
        
        // Verificar se adicionar este pedido não ultrapassa UB
        if (novaSolucao.totalUnidades + unidadesAdicionais <= UB) {
            novaSolucao.pedidosWave.push_back(pedidoId);
            recalcularSolucao(novaSolucao);
        }
    }
    
    return novaSolucao;
}

// --- Basic Local Search (e.g., Best Improvement Add/Remove) ---
BuscaLocalAvancada::Solucao BuscaLocalAvancada::buscaLocalBasica(
    const Solucao& solucao, int tipoVizinhanca, int LB, int UB)
{
    Solucao solucaoAtual = solucao;
    bool melhoriaEncontrada = true;

    while (melhoriaEncontrada && !tempoExcedido()) {
        melhoriaEncontrada = false;
        std::vector<Movimento> vizinhanca = gerarVizinhanca(solucaoAtual, LB, UB, tipoVizinhanca);
        double melhorDelta = 0; // Only accept improvements
        Movimento melhorMovimento;

        for (const auto& movimento : vizinhanca) {
            // Evaluate move - use efficient delta if available, otherwise recalculate
            Solucao solucaoVizinha = aplicarMovimento(solucaoAtual, movimento);
            if (!solucaoViavel(solucaoVizinha, LB, UB)) continue;
            recalcularSolucao(solucaoVizinha);
            double delta = solucaoVizinha.valorObjetivo - solucaoAtual.valorObjetivo;

            if (delta > melhorDelta) {
                melhorDelta = delta;
                melhorMovimento = movimento;
                melhoriaEncontrada = true;
            }
        }

        if (melhoriaEncontrada) {
            solucaoAtual = aplicarMovimento(solucaoAtual, melhorMovimento);
            recalcularSolucao(solucaoAtual); // Update state fully
            estatisticas_.melhorias++; // Count improvements within basic LS
        }
    }
    return solucaoAtual;
}


// --- Initialize Long-Term Memory ---
void BuscaLocalAvancada::inicializarMemoriaLongoPrazo(int numPedidos) {
    frequenciaPedidos_.assign(numPedidos, 0);
    recenciaPedidos_.assign(numPedidos, 0);
    qualidadePedidos_.assign(numPedidos, 0.0);
}

// --- Placeholder Implementations for Missing Functions ---
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosPathRelinking(const Solucao& solucao, const Solucao& solucaoGuia, int LB, int UB) { return {}; }
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosIntensificacao(const Solucao& solucao, int LB, int UB) {
     // Example: Focus on swaps involving high-frequency orders
     return gerarMovimentosSwap(solucao, LB, UB); // Placeholder
}
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosDiversificacao(const Solucao& solucao, int LB, int UB) {
     // Example: Focus on adding low-frequency orders
     return gerarVizinhanca(solucao, LB, UB, 0); // Placeholder
}

// Adicionar após os outros métodos de configuração
void BuscaLocalAvancada::configurarILS(const ConfigILS& config) {
    configILS_ = config;
}

// Implementar vizinhança focada em minimizar corredores
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosReducaoCorredores(
    const Solucao& solucao, int LB, int UB) {
    
    std::vector<Movimento> movimentos;
    
    // Mapear quais corredores são usados por quais pedidos
    std::unordered_map<int, std::vector<int>> pedidosPorCorredor;
    
    // E quais corredores cada pedido usa
    std::unordered_map<int, std::unordered_set<int>> corredoresPorPedido;
    
    for (int pedidoId : solucao.pedidosWave) {
        std::unordered_set<int> corredores;
        
        for (const auto& [itemId, qtd] : backlog_.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredores.insert(corredorId);
                pedidosPorCorredor[corredorId].push_back(pedidoId);
            }
        }
        
        corredoresPorPedido[pedidoId] = corredores;
    }
    
    // Encontrar corredores com poucos pedidos (potenciais candidatos à eliminação)
    std::vector<std::pair<int, int>> corredoresPoucoUtilizados;
    for (const auto& [corredorId, pedidos] : pedidosPorCorredor) {
        corredoresPoucoUtilizados.push_back({corredorId, pedidos.size()});
    }
    
    // Ordenar por utilização (crescente)
    std::sort(corredoresPoucoUtilizados.begin(), corredoresPoucoUtilizados.end(),
             [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Para cada corredor pouco utilizado, tentar eliminar trocando seus pedidos
    for (const auto& [corredorId, numPedidos] : corredoresPoucoUtilizados) {
        // Se for usado por apenas um pedido, tentar removê-lo
        if (numPedidos == 1) {
            int pedidoId = pedidosPorCorredor[corredorId][0];
            
            // Verificar se podemos remover este pedido mantendo LB
            int totalUnidadesSemPedido = 0;
            for (int pid : solucao.pedidosWave) {
                if (pid != pedidoId) {
                    for (const auto& [_, qtd] : backlog_.pedido[pid]) {
                        totalUnidadesSemPedido += qtd;
                    }
                }
            }
            
            if (totalUnidadesSemPedido >= LB) {
                // Criar movimento para remover este pedido
                Movimento mov;
                mov.tipo = TipoMovimento::REMOVER;
                mov.pedidosRemover = {pedidoId};
                mov.pedidosAdicionar = {};
                movimentos.push_back(mov);
            }
        }
        // Se for usado por múltiplos pedidos, tentar substituí-los
        else if (numPedidos > 1 && numPedidos <= 3) { // Limitar para não explodir em combinações
            // Identificar pedidos candidatos a substituir os atuais
            std::vector<int> pedidosDoCorretor = pedidosPorCorredor[corredorId];
            
            // Calcular total de unidades desses pedidos
            int totalUnidadesPedidos = 0;
            for (int pid : pedidosDoCorretor) {
                for (const auto& [_, qtd] : backlog_.pedido[pid]) {
                    totalUnidadesPedidos += qtd;
                }
            }
            
            // Buscar pedidos alternativos que não usem este corredor
            std::vector<std::pair<int, int>> pedidosAlternativos;
            for (int p = 0; p < backlog_.numPedidos; p++) {
                // Pular se já está na solução
                if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), p) != solucao.pedidosWave.end()) {
                    continue;
                }
                
                // Verificar se este pedido não usa o corredor em questão
                bool usaCorreidor = false;
                for (const auto& [itemId, _] : backlog_.pedido[p]) {
                    if (localizador_.getCorredoresComItem(itemId).find(corredorId) != 
                        localizador_.getCorredoresComItem(itemId).end()) {
                        usaCorreidor = true;
                        break;
                    }
                }
                
                if (!usaCorreidor) {
                    // Calcular total de unidades desse pedido
                    int unidades = 0;
                    for (const auto& [_, qtd] : backlog_.pedido[p]) {
                        unidades += qtd;
                    }
                    
                    pedidosAlternativos.push_back({p, unidades});
                }
            }
            
            // Ordenar alternativas por unidades (decrescente)
            std::sort(pedidosAlternativos.begin(), pedidosAlternativos.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            // Tentar diferentes combinações de substituição
            // Aqui poderia ser implementado um algoritmo mais sofisticado de knapsack
            // Para simplificar, apenas verifico se existe algum pedido com unidades similares
            for (const auto& [pedidoAlt, unidades] : pedidosAlternativos) {
                // Se for próximo em unidades, propor substituição
                if (std::abs(unidades - totalUnidadesPedidos) <= totalUnidadesPedidos * 0.2) {
                    Movimento mov;
                    mov.tipo = TipoMovimento::SWAP;
                    mov.pedidosRemover = pedidosDoCorretor;
                    mov.pedidosAdicionar = {pedidoAlt};
                    movimentos.push_back(mov);
                    break;
                }
            }
        }
    }
    
    return movimentos;
}