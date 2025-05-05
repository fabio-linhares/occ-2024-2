#include "busca_local_avancada.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <set>
#include <numeric> // For std::iota

// Helper struct for Tabu List Key
struct MovimentoTabuKey {
    BuscaLocalAvancada::TipoMovimento tipo;
    std::vector<int> pedidosAdd; // Sorted
    std::vector<int> pedidosRem; // Sorted

    bool operator==(const MovimentoTabuKey& other) const {
        return tipo == other.tipo && pedidosAdd == other.pedidosAdd && pedidosRem == other.pedidosRem;
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
    if (limiteTempo_ <= 0) return false;
    auto tempoAtual = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(tempoAtual - tempoInicio_).count() > limiteTempo_;
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
             vizinhanca = gerarMovimentosIntensificacao(solucaoAtual, LB, UB); // Needs implementation
             estatisticas_.iteracoesIntensificacao++;
        } else if (modoDiversificacao) {
             vizinhanca = gerarMovimentosDiversificacao(solucaoAtual, LB, UB); // Needs implementation
             estatisticas_.iteracoesDiversificacao++;
        } else {
             vizinhanca = gerarVizinhanca(solucaoAtual, LB, UB); // Standard neighborhood
        }


        Movimento melhorMovimento;
        double melhorDelta = -std::numeric_limits<double>::infinity();
        bool encontrouNaoTabu = false;

        for (const auto& movimento : vizinhanca) {
            // Create key for tabu check
            MovimentoTabuKey key;
            key.tipo = movimento.tipo;
            key.pedidosAdd = movimento.pedidosAdicionar;
            key.pedidosRem = movimento.pedidosRemover;
            std::sort(key.pedidosAdd.begin(), key.pedidosAdd.end());
            std::sort(key.pedidosRem.begin(), key.pedidosRem.end());

            bool isTabu = listaTabu.count(key) && listaTabu[key] > iteracao;
            estatisticas_.movimentosTabu += isTabu;

            // Calculate objective after applying move (approximate or exact)
            // double valorVizinho = solucaoAtual.valorObjetivo + movimento.deltaValorObjetivo; // Use pre-calculated delta
            // For simplicity/robustness, let's recalculate (can be optimized)
            Solucao solucaoVizinha = aplicarMovimento(solucaoAtual, movimento);
            if (!solucaoViavel(solucaoVizinha, LB, UB)) continue; // Skip infeasible neighbors
            recalcularSolucao(solucaoVizinha); // Calculate exact objective
            double valorVizinho = solucaoVizinha.valorObjetivo;
            double deltaAtual = valorVizinho - solucaoAtual.valorObjetivo;


            bool aspiracao = valorVizinho > melhorSolucao.valorObjetivo;
            if (aspiracao) estatisticas_.aspiracoesSucedidas++;

            if ((!isTabu || aspiracao) && deltaAtual > melhorDelta) {
                melhorDelta = deltaAtual;
                melhorMovimento = movimento;
                encontrouNaoTabu = !isTabu || aspiracao;
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
     Solucao solucaoAtual = buscaLocalBasica(solucaoInicial, 0, LB, UB); // Start from a local optimum
     Solucao melhorSolucao = solucaoAtual;
     int iter = 0;
     int iterSemMelhoria = 0;

     while (iter < configILS_.maxIteracoes && !tempoExcedido()) {
         estatisticas_.iteracoesTotais++;
         iter++;

         // 1. Perturbation: Apply perturbation to the current solution s
         double intensidadePert = configILS_.intensidadePerturbacaoBase + 
                                  (double)iterSemMelhoria * 0.01; // Usar valor fixo em vez de fatorAumentoIntensidade
         Solucao solucaoPerturbada = perturbarSolucao(solucaoAtual, intensidadePert, LB, UB);
         estatisticas_.perturbacoes++;

         // 2. Local Search: Apply local search to the perturbed solution s'
         Solucao solucaoAposLS = buscaLocalBasica(solucaoPerturbada, 0, LB, UB);
         estatisticas_.buscasLocais++;

         // 3. Acceptance Criterion: Decide whether to accept s'' as the new current solution
         // Accept if better or based on some probability (e.g., simulated annealing like)
         // Simple acceptance: always accept if better, otherwise keep current
         if (solucaoAposLS.valorObjetivo > solucaoAtual.valorObjetivo) {
             solucaoAtual = solucaoAposLS;
             iterSemMelhoria = 0; // Reset on improvement
             if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                 melhorSolucao = solucaoAtual;
                 estatisticas_.melhorias++;
                 // std::cout << "Nova melhor solucao (ILS) it " << iter << ": BOV = " << melhorSolucao.valorObjetivo << std::endl;
             }
         } else {
             iterSemMelhoria++;
             // Could add more sophisticated acceptance (e.g., accept worse with probability)
         }

         // Optional: Reset if stuck for too long
         if (iterSemMelhoria > configILS_.perturbacoesSemMelhoria * 2) { // Usar um múltiplo dos parâmetros existentes
              solucaoAtual = perturbarSolucao(melhorSolucao, configILS_.intensidadePerturbacaoBase * 5, LB, UB);
              solucaoAtual = buscaLocalBasica(solucaoAtual, 0, LB, UB);
              iterSemMelhoria = 0;
              // std::cout << "ILS Resetting..." << std::endl;
         }
     }
     return melhorSolucao;
}


// --- Neighborhood Generation (Basic Example: Add/Remove) ---
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarVizinhanca(
    const Solucao& solucao, int LB, int UB, int tipoVizinhanca)
{
    std::vector<Movimento> vizinhanca;
    std::vector<int> pedidosFora;
    std::unordered_set<int> pedidosDentro(solucao.pedidosWave.begin(), solucao.pedidosWave.end());

    for(int i=0; i<backlog_.numPedidos; ++i) {
        if(pedidosDentro.find(i) == pedidosDentro.end()) {
            pedidosFora.push_back(i);
        }
    }

    // 1. Add Moves: Try adding one order from outside
    for (int pedidoAdd : pedidosFora) {
        Movimento m;
        m.tipo = TipoMovimento::ADICIONAR;
        m.pedidosAdicionar = {pedidoAdd};
        m.pedidosRemover = {};
        // Evaluate delta efficiently (or mark for later evaluation)
        m.deltaValorObjetivo = avaliarMovimento(solucao, m); // Needs implementation
        vizinhanca.push_back(m);
    }

    // 2. Remove Moves: Try removing one order from inside
    for (int pedidoRem : solucao.pedidosWave) {
        Movimento m;
        m.tipo = TipoMovimento::REMOVER;
        m.pedidosAdicionar = {};
        m.pedidosRemover = {pedidoRem};
        m.deltaValorObjetivo = avaliarMovimento(solucao, m); // Needs implementation
        vizinhanca.push_back(m);
    }

     // 3. Swap Moves (Optional, can be separate neighborhood)
     if (tipoVizinhanca >= 1) {
         auto swapMoves = gerarMovimentosSwap(solucao, LB, UB);
         vizinhanca.insert(vizinhanca.end(), swapMoves.begin(), swapMoves.end());
     }


    // Filter out moves leading to obviously infeasible states (e.g., violating UB/LB drastically)
    // More rigorous check happens when evaluating the move in the main loop

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