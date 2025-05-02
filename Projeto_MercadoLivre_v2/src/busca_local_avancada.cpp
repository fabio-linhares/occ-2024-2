#include "busca_local_avancada.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <set>


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
    // Inicialização padrão das configurações
    configTabu_ = ConfigTabu{};
    configVNS_ = ConfigVNS{};
    configILS_ = ConfigILS{};
    
    // Inicializar estatísticas
    estatisticas_.iteracoesTotais = 0;
    estatisticas_.melhorias = 0;
    estatisticas_.tempoExecucaoMs = 0;
    estatisticas_.algoritmoUsado = "Nenhum";

    inicializarMemoriaLongoPrazo(backlog_.numPedidos);
}

void BuscaLocalAvancada::iniciarEstatisticas(const Solucao& solucaoInicial) {
    // Resetar todas as estatísticas
    estatisticas_.iteracoesTotais = 0;
    estatisticas_.melhorias = 0;
    estatisticas_.tempoExecucaoMs = 0;
    estatisticas_.movimentosAceitos = 0;
    estatisticas_.movimentosRejeitados = 0;
    estatisticas_.iteracoesIntensificacao = 0;
    estatisticas_.iteracoesDiversificacao = 0;
    estatisticas_.movimentosTabu = 0;
    estatisticas_.aspiracoesSucedidas = 0;
    estatisticas_.mudancasVizinhanca = 0;
    estatisticas_.shakesSucedidos = 0;
    estatisticas_.perturbacoes = 0;
    estatisticas_.buscasLocais = 0;
    
    // Registrar valor objetivo inicial
    estatisticas_.valorObjetivoInicial = solucaoInicial.valorObjetivo;
    estatisticas_.melhorValorObjetivo = solucaoInicial.valorObjetivo;
    estatisticas_.melhoria = 0.0;
    
    // Iniciar cronômetro
    tempoInicio_ = std::chrono::high_resolution_clock::now();
}

BuscaLocalAvancada::Solucao BuscaLocalAvancada::otimizar(
    const Solucao& solucaoInicial, 
    int LB, int UB, 
    TipoBuscaLocal tipoBusca
) {
    // Registrar tempo de início
    tempoInicio_ = std::chrono::high_resolution_clock::now();
    
    // Armazenar o tipo de algoritmo usado nas estatísticas
    switch (tipoBusca) {
        case TipoBuscaLocal::BUSCA_TABU:
            estatisticas_.algoritmoUsado = "Busca Tabu";
            return buscaTabu(solucaoInicial, LB, UB);
        case TipoBuscaLocal::VNS:
            estatisticas_.algoritmoUsado = "VNS";
            return vns(solucaoInicial, LB, UB);
        case TipoBuscaLocal::ILS:
            estatisticas_.algoritmoUsado = "ILS";
            return ils(solucaoInicial, LB, UB);
        default:
            estatisticas_.algoritmoUsado = "Busca Tabu (default)";
            return buscaTabu(solucaoInicial, LB, UB);
    }
}

void BuscaLocalAvancada::configurarTabu(const ConfigTabu& config) {
    configTabu_ = config;
}

void BuscaLocalAvancada::configurarVNS(const ConfigVNS& config) {
    configVNS_ = config;
}

void BuscaLocalAvancada::configurarILS(const ConfigILS& config) {
    configILS_ = config;
}

std::string BuscaLocalAvancada::obterEstatisticas() const {
    std::stringstream ss;
    ss << "===== Estatísticas de Execução =====\n";
    ss << "Algoritmo: " << estatisticas_.algoritmoUsado << "\n";
    ss << "Iterações totais: " << estatisticas_.iteracoesTotais << "\n";
    ss << "Melhorias: " << estatisticas_.melhorias << "\n";
    
    // Calcular percentual de melhoria
    double percentualMelhoria = 0.0;
    if (estatisticas_.valorObjetivoInicial > 0) {
        percentualMelhoria = (estatisticas_.melhorValorObjetivo - estatisticas_.valorObjetivoInicial) 
                           / estatisticas_.valorObjetivoInicial * 100.0;
    }
    
    ss << "Valor objetivo inicial: " << std::fixed << std::setprecision(2) 
       << estatisticas_.valorObjetivoInicial << "\n";
    ss << "Melhor valor objetivo: " << std::fixed << std::setprecision(2) 
       << estatisticas_.melhorValorObjetivo << "\n";
    ss << "Melhoria: " << std::fixed << std::setprecision(2) 
       << percentualMelhoria << "%\n";
    ss << "Tempo de execução: " << estatisticas_.tempoExecucaoMs << " ms\n";
    
    // Adicionar estatísticas específicas do algoritmo usado
    if (estatisticas_.algoritmoUsado.find("Tabu") != std::string::npos) {
        ss << "\n--- Estatísticas da Busca Tabu ---\n";
        ss << "Movimentos tabu encontrados: " << estatisticas_.movimentosTabu << "\n";
        ss << "Aspirações sucedidas: " << estatisticas_.aspiracoesSucedidas << "\n";
        ss << "Iterações em intensificação: " << estatisticas_.iteracoesIntensificacao << "\n";
        ss << "Iterações em diversificação: " << estatisticas_.iteracoesDiversificacao << "\n";
    } 
    else if (estatisticas_.algoritmoUsado == "VNS") {
        ss << "\n--- Estatísticas do VNS ---\n";
        ss << "Mudanças de vizinhança: " << estatisticas_.mudancasVizinhanca << "\n";
        ss << "Shakes sucedidos: " << estatisticas_.shakesSucedidos << "\n";
        ss << "Buscas locais: " << estatisticas_.buscasLocais << "\n";
    } 
    else if (estatisticas_.algoritmoUsado == "ILS") {
        ss << "\n--- Estatísticas do ILS ---\n";
        ss << "Perturbações: " << estatisticas_.perturbacoes << "\n";
        ss << "Buscas locais: " << estatisticas_.buscasLocais << "\n";
    }
    
    return ss.str();
}

bool BuscaLocalAvancada::tempoExcedido() const {
    auto tempoAtual = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(tempoAtual - tempoInicio_).count();
    return duracao > limiteTempo_ * 1000; // Converter segundos para milissegundos
}

// Implementação da Busca Tabu
BuscaLocalAvancada::Solucao BuscaLocalAvancada::buscaTabu(
    const Solucao& solucaoInicial, 
    int LB, int UB
) {
    // Inicializar solução atual e melhor solução global
    Solucao solucaoAtual = solucaoInicial;
    Solucao melhorSolucao = solucaoInicial;
    
    // Calcular valor objetivo inicial
    calcularValorObjetivo(solucaoAtual);
    calcularValorObjetivo(melhorSolucao);
    
    // Inicializar estatísticas
    iniciarEstatisticas(solucaoInicial);
    estatisticas_.algoritmoUsado = "Busca Tabu";
    estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
    
    // Estruturas da busca tabu
    std::unordered_map<int, int> listaTabu; // pedidoId -> iteracao de liberação
    std::unordered_map<int, int> frequenciaPedidos; // pedidoId -> frequência
    
    // Inicializar frequências
    for (int i = 0; i < backlog_.numPedidos; i++) {
        frequenciaPedidos[i] = 0;
    }
    
    // Parâmetros da busca
    int iteracaoAtual = 0;
    int iteracoesSemMelhoria = 0;
    bool modoIntensificacao = false;
    bool modoDiversificacao = false;
    
    // Loop principal da busca tabu
    while (!tempoExcedido() && iteracoesSemMelhoria < configTabu_.maxIteracoesSemMelhoria) {
        estatisticas_.iteracoesTotais++;
        
        // Gerar vizinhança
        std::vector<Movimento> vizinhanca;
        
        if (modoIntensificacao) {
            // Intensificação: explorar regiões promissoras 
            vizinhanca = gerarMovimentosIntensificacao(solucaoAtual, LB, UB);
            estatisticas_.iteracoesIntensificacao++;
        } else if (modoDiversificacao) {
            // Diversificação: explorar áreas pouco visitadas
            vizinhanca = gerarMovimentosDiversificacao(solucaoAtual, LB, UB);
            estatisticas_.iteracoesDiversificacao++;
        } else {
            // Vizinhança padrão: combinação de swaps e chain-exchange
            std::vector<Movimento> swaps = gerarMovimentosSwap(solucaoAtual, LB, UB);
            std::vector<Movimento> chains = gerarMovimentosChainExchange(solucaoAtual, LB, UB);
            
            vizinhanca.insert(vizinhanca.end(), swaps.begin(), swaps.end());
            vizinhanca.insert(vizinhanca.end(), chains.begin(), chains.end());
        }
        
        // Se não encontramos vizinhos viáveis, tentar estratégias alternativas
        if (vizinhanca.empty()) {
            if (!modoIntensificacao && !modoDiversificacao) {
                modoIntensificacao = true;
                continue;
            } else if (modoIntensificacao) {
                modoIntensificacao = false;
                modoDiversificacao = true;
                continue;
            } else {
                // Se já tentamos tudo, reiniciar da melhor solução com perturbação
                solucaoAtual = perturbarSolucao(melhorSolucao, 0.3, LB, UB);
                modoDiversificacao = false;
                estatisticas_.reiniciosForçados++;
                continue;
            }
        }
        
        // Encontrar o melhor movimento não-tabu ou com aspiração
        Movimento melhorMovimento;
        double melhorDelta = -std::numeric_limits<double>::max();
        bool movimentoEncontrado = false;
        
        for (const Movimento& movimento : vizinhanca) {
            // Verificar se o movimento é tabu
            bool ehTabu = false;
            for (int pedidoId : movimento.pedidosRemover) {
                if (listaTabu.find(pedidoId) != listaTabu.end() && 
                    listaTabu[pedidoId] > iteracaoAtual) {
                    ehTabu = true;
                    break;
                }
            }
            
            // Calcular delta real aplicando o movimento
            Solucao novaSolucao = aplicarMovimento(solucaoAtual, movimento);
            double delta = novaSolucao.valorObjetivo - solucaoAtual.valorObjetivo;
            
            // Critério de aceitação: não é tabu OU satisfaz critério de aspiração
            if (!ehTabu || novaSolucao.valorObjetivo > melhorSolucao.valorObjetivo) {
                if (delta > melhorDelta) {
                    melhorDelta = delta;
                    melhorMovimento = movimento;
                    movimentoEncontrado = true;
                    
                    if (ehTabu) {
                        estatisticas_.aspiracoesSucedidas++;
                    }
                }
            } else {
                estatisticas_.movimentosTabu++;
            }
        }
        
        // Se não encontramos movimento viável, tentar estratégias alternativas
        if (!movimentoEncontrado) {
            if (!modoIntensificacao && !modoDiversificacao) {
                modoIntensificacao = true;
            } else if (modoIntensificacao) {
                modoIntensificacao = false;
                modoDiversificacao = true;
            } else {
                modoDiversificacao = false;
                // Reiniciar com perturbação forte
                solucaoAtual = perturbarSolucao(melhorSolucao, 0.4, LB, UB);
                estatisticas_.reiniciosForçados++;
            }
            continue;
        }
        
        // Aplicar o melhor movimento
        solucaoAtual = aplicarMovimento(solucaoAtual, melhorMovimento);
        iteracaoAtual++;
        
        // Atualizar estruturas tabu
        for (int pedidoId : melhorMovimento.pedidosRemover) {
            // Duração tabu dinâmica baseada no tamanho do problema e iteração atual
            int duracaoTabu = configTabu_.duracaoTabuBase + 
                            (iteracaoAtual % 5); // Pequena variação
            listaTabu[pedidoId] = iteracaoAtual + duracaoTabu;
            
            // Atualizar frequência para memória de longo prazo
            frequenciaPedidos[pedidoId]++;
            frequenciaPedidos_[pedidoId]++;
            recenciaPedidos_[pedidoId] = iteracaoAtual;
        }
        
        for (int pedidoId : melhorMovimento.pedidosAdicionar) {
            frequenciaPedidos[pedidoId]++;
        }
        
        // Verificar se melhorou a melhor solução global
        if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
            melhorSolucao = solucaoAtual;
            iteracoesSemMelhoria = 0;
            estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
            estatisticas_.melhorias++;
            
            // Atualizar qualidade
            for (int pedidoId : solucaoAtual.pedidosWave) {
                qualidadePedidos_[pedidoId] += 1.0;
            }
            
            // Reset dos modos de intensificação/diversificação
            modoIntensificacao = false;
            modoDiversificacao = false;
        } else {
            iteracoesSemMelhoria++;
        }
        
        // Ativar estratégias de intensificação/diversificação baseado no progresso
        if (iteracoesSemMelhoria > 0 && iteracoesSemMelhoria % configTabu_.ciclosIntensificacao == 0) {
            modoIntensificacao = true;
            modoDiversificacao = false;
        } else if (iteracoesSemMelhoria > 0 && 
                 iteracoesSemMelhoria % configTabu_.ciclosDiversificacao == 0) {
            modoIntensificacao = false;
            modoDiversificacao = true;
        } else {
            modoIntensificacao = false;
            modoDiversificacao = false;
        }
    }
    
    // Registrar estatísticas finais
    estatisticas_.tempoExecucaoMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - tempoInicio_).count();
    
    return melhorSolucao;
}

// Implementação básica para o restante dos métodos necessários
// Estas implementações iniciais podem ser expandidas posteriormente

std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosSwap(
    const Solucao& solucao, 
    int LB, int UB
) {
    std::vector<Movimento> movimentos;
    
    // Obter todos os pedidos não incluídos na solução atual
    std::vector<int> pedidosForaDaWave;
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) 
            == solucao.pedidosWave.end()) {
            pedidosForaDaWave.push_back(pedidoId);
        }
    }
    
    // Para cada pedido na wave atual, tentar trocar por um pedido fora da wave
    for (int pedidoNaWave : solucao.pedidosWave) {
        // Calcular unidades e corredores atuais do pedido na wave
        int unidadesPedidoAtual = 0;
        std::set<int> corredoresPedidoAtual;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoNaWave]) {
            unidadesPedidoAtual += quantidade;
            
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresPedidoAtual.insert(corredorId);
            }
        }
        
        for (int pedidoFora : pedidosForaDaWave) {
            // Verificar se troca mantém a solução dentro dos limites LB e UB
            int unidadesPedidoNovo = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoFora]) {
                unidadesPedidoNovo += quantidade;
            }
            
            int novoTotalUnidades = solucao.totalUnidades - unidadesPedidoAtual + unidadesPedidoNovo;
            
            if (novoTotalUnidades < LB || novoTotalUnidades > UB) {
                continue;  // Troca violaria os limites
            }
            
            // Criar movimento de troca
            Movimento movimento;
            movimento.tipo = TipoMovimento::SWAP;
            movimento.pedidosRemover.push_back(pedidoNaWave);
            movimento.pedidosAdicionar.push_back(pedidoFora);
            
            // Estimar o impacto da troca
            Solucao novaSolucao = aplicarMovimento(solucao, movimento);
            double novoValorObjetivo = calcularValorObjetivo(novaSolucao);
            movimento.deltaValorObjetivo = novoValorObjetivo - solucao.valorObjetivo;
            
            // Adicionar à lista de movimentos se for potencialmente benéfico
            if (movimento.deltaValorObjetivo >= -0.001) {  // Pequena tolerância para diversificação
                movimentos.push_back(movimento);
            }
        }
    }
    
    return movimentos;
}

// Implementações iniciais para o restante dos métodos
// Estas serão completadas em implementações futuras

BuscaLocalAvancada::Solucao BuscaLocalAvancada::vns(
    const Solucao& solucaoInicial, 
    int LB, int UB
) {
    // Inicializar melhor solução e solução atual
    Solucao melhorSolucao = solucaoInicial;
    Solucao solucaoAtual = solucaoInicial;
    
    // Calcular valor objetivo inicial
    calcularValorObjetivo(melhorSolucao);
    calcularValorObjetivo(solucaoAtual);
    
    // Inicializar estatísticas
    iniciarEstatisticas(solucaoInicial);
    estatisticas_.algoritmoUsado = "Variable Neighborhood Search";
    estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
    
    // Variáveis de controle
    int iteracoesSemMelhoria = 0;
    int k = 1; // Índice da estrutura de vizinhança atual
    
    // Loop principal do VNS
    while (!tempoExcedido() && iteracoesSemMelhoria < configVNS_.maxIteracoesSemMelhoria) {
        estatisticas_.iteracoesTotais++;
        
        // Fase de Shaking: perturbar a solução atual com intensidade proporcional a k
        double intensidadePerturbacao = 0.1 * k; // Ajustar conforme necessário
        Solucao solucaoPerturbada = perturbarSolucao(solucaoAtual, intensidadePerturbacao, LB, UB);
        estatisticas_.perturbacoes++;
        
        // Fase de Busca Local: explorar a vizinhança da solução perturbada
        Solucao solucaoMelhorada;
        
        // Escolher tipo de vizinhança baseado em k
        switch (k % 3) {
            case 0:
                // Vizinhança simples (swaps)
                solucaoMelhorada = buscaLocalBasica(solucaoPerturbada, 0, LB, UB);
                break;
                
            case 1:
                // Vizinhança de chain-exchange
                solucaoMelhorada = buscaLocalBasica(solucaoPerturbada, 1, LB, UB);
                break;
                
            case 2:
                // Combinação de vizinhanças
                solucaoMelhorada = buscaLocalBasica(solucaoPerturbada, 2, LB, UB);
                break;
        }
        
        estatisticas_.buscasLocais++;
        
        // Verificar se houve melhoria
        if (solucaoMelhorada.valorObjetivo > solucaoAtual.valorObjetivo) {
            // Aceitar a nova solução
            solucaoAtual = solucaoMelhorada;
            k = 1; // Reiniciar estrutura de vizinhança
            iteracoesSemMelhoria = 0;
            estatisticas_.shakesSucedidos++;
            
            // Verificar se é a melhor global
            if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                melhorSolucao = solucaoAtual;
                estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
                estatisticas_.melhorias++;
            }
        } else {
            // Passar para a próxima estrutura de vizinhança
            k++;
            iteracoesSemMelhoria++;
            
            // Reiniciar ciclo de vizinhanças se atingir o máximo
            if (k > configVNS_.kMax) {
                k = 1;
            }
            
            estatisticas_.mudancasVizinhanca++;
        }
    }
    
    // Registrar estatísticas finais
    estatisticas_.tempoExecucaoMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - tempoInicio_).count();
    
    return melhorSolucao;
}

BuscaLocalAvancada::Solucao BuscaLocalAvancada::ils(
    const Solucao& solucaoInicial, 
    int LB, int UB
) {
    // Inicializar solução atual e melhor solução
    Solucao solucaoAtual = solucaoInicial;
    Solucao melhorSolucao = solucaoInicial;
    
    // Calcular valor objetivo inicial
    calcularValorObjetivo(solucaoAtual);
    calcularValorObjetivo(melhorSolucao);
    
    // Inicializar estatísticas
    iniciarEstatisticas(solucaoInicial);
    estatisticas_.algoritmoUsado = "Iterated Local Search";
    estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
    
    // Primeiro, melhorar a solução inicial com busca local
    solucaoAtual = buscaLocalBasica(solucaoAtual, 0, LB, UB);
    estatisticas_.buscasLocais++;
    
    // Atualizar melhor solução se necessário
    if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
        melhorSolucao = solucaoAtual;
        estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
        estatisticas_.melhorias++;
    }
    
    // Variáveis de controle
    int iteracoesSemMelhoria = 0;
    double nivelPerturbacao = configILS_.nivelPerturbacao;
    double temperatura = configILS_.temperaturaInicial;
    
    // Loop principal do ILS
    for (int iteracao = 0; 
         iteracao < configILS_.maxIteracoes && 
         !tempoExcedido() && 
         iteracoesSemMelhoria < configILS_.maxIteracoesSemMelhoria; 
         iteracao++) {
        
        estatisticas_.iteracoesTotais++;
        
        // Fase de Perturbação
        Solucao solucaoPerturbada = perturbarSolucao(solucaoAtual, nivelPerturbacao, LB, UB);
        estatisticas_.perturbacoes++;
        
        // Fase de Busca Local
        // Alternamos entre tipos de busca local para diversificar
        int tipoBusca = iteracao % 3;
        Solucao solucaoMelhorada = buscaLocalBasica(solucaoPerturbada, tipoBusca, LB, UB);
        estatisticas_.buscasLocais++;
        
        // Fase de Aceitação - baseada em critério de Metropolis
        double delta = solucaoMelhorada.valorObjetivo - solucaoAtual.valorObjetivo;
        
        // Aceitar se melhorou ou probabilisticamente baseado em temperatura
        if (delta > 0) {
            // Aceitar melhoria sempre
            solucaoAtual = solucaoMelhorada;
            iteracoesSemMelhoria = 0;
            
            // Atualizar melhor solução global
            if (solucaoMelhorada.valorObjetivo > melhorSolucao.valorObjetivo) {
                melhorSolucao = solucaoMelhorada;
                estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
                estatisticas_.melhorias++;
            }
        } else {
            // Critério de aceitação de Metropolis para soluções piores
            std::uniform_real_distribution<> dist(0.0, 1.0);
            if (dist(rng_) < exp(delta / temperatura)) {
                solucaoAtual = solucaoMelhorada;
                estatisticas_.aceitacoesEstrategicas++;
            } else {
                iteracoesSemMelhoria++;
            }
        }
        
        // Atualização de parâmetros - resfriamento e ajuste de perturbação
        temperatura *= configILS_.taxaResfriamento; // Resfriar temperatura
        
        // Intensificar (perturbação menor) ou diversificar (perturbação maior)
        if (iteracoesSemMelhoria > configILS_.maxIteracoesSemMelhoria / 2) {
            // Aumentar perturbação para escapar de ótimos locais
            nivelPerturbacao = std::min(nivelPerturbacao * 1.5, 0.9);
        } else if (iteracoesSemMelhoria < configILS_.maxIteracoesSemMelhoria / 4) {
            // Diminuir perturbação para refinar em áreas promissoras
            nivelPerturbacao = std::max(nivelPerturbacao * 0.8, configILS_.nivelPerturbacao);
        }
        
        // Reset periódico para a melhor solução (intensificação)
        if (iteracoesSemMelhoria >= configILS_.maxIteracoesSemMelhoria / 1.5) {
            solucaoAtual = melhorSolucao;
            nivelPerturbacao = configILS_.nivelPerturbacao * 2.0; // Perturbação forte para diversificar
            temperatura = configILS_.temperaturaInicial / 2.0; // Reduzir temperatura para seletividade
            iteracoesSemMelhoria = 0;
            estatisticas_.reiniciosForçados++;
        }
    }
    
    // Registrar estatísticas finais
    estatisticas_.tempoExecucaoMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - tempoInicio_).count();
    
    return melhorSolucao;
}

// Gerar movimentos de chain-exchange (troca em cadeia)
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosChainExchange(
    const Solucao& solucao,
    int LB, int UB
) {
    std::vector<Movimento> movimentos;
    
    // Número total de unidades na solução atual
    int totalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Identificar pedidos não incluídos na solução (candidatos a adicionar)
    std::vector<int> pedidosCandidatos;
    for (int p = 0; p < backlog_.numPedidos; p++) {
        if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), p) 
            == solucao.pedidosWave.end()) {
            pedidosCandidatos.push_back(p);
        }
    }
    
    // Tamanho da cadeia de troca (quantos pedidos trocar de cada vez)
    // Adaptativo: usar cadeias menores para instâncias grandes
    int tamanhoCadeia = (backlog_.numPedidos > 500) ? 1 : 
                       (backlog_.numPedidos > 200) ? 2 : 3;
    
    // Gerar trocas de N pedidos atuais por N candidatos
    // Para instâncias grandes, limitar número de pedidos na cadeia
    
    // Função recursiva para gerar todas as combinações de pedidos a remover
    std::function<void(const std::vector<int>&, std::vector<int>&, size_t, int)> 
    gerarCombinacoes = [&](
        const std::vector<int>& pedidosOrigem, 
        std::vector<int>& combinacaoAtual, 
        size_t inicio, 
        int k
    ) {
        // Caso base: já temos k pedidos na combinação
        if (combinacaoAtual.size() == k) {
            // Calcular unidades removidas
            int unidadesRemovidas = 0;
            for (int pedidoId : combinacaoAtual) {
                for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                    unidadesRemovidas += quantidade;
                }
            }
            
            // Buscar combinações de pedidos a adicionar
            std::vector<int> pedidosRemover = combinacaoAtual;
            
            // Limitamos número de combinações para instâncias grandes
            int maxCandidatos = std::min(static_cast<int>(pedidosCandidatos.size()), 
                                       (backlog_.numPedidos > 500) ? 20 : 
                                       (backlog_.numPedidos > 200) ? 50 : 100);
            
            // Escolher aleatoriamente um subconjunto de candidatos para avaliar
            std::vector<int> candidatosAvaliados;
            if (pedidosCandidatos.size() > maxCandidatos) {
                std::vector<int> indices(pedidosCandidatos.size());
                std::iota(indices.begin(), indices.end(), 0);
                std::shuffle(indices.begin(), indices.end(), rng_);
                
                for (int i = 0; i < maxCandidatos; i++) {
                    candidatosAvaliados.push_back(pedidosCandidatos[indices[i]]);
                }
            } else {
                candidatosAvaliados = pedidosCandidatos;
            }
            
            // Gerar combinações de pedidos a adicionar
            std::function<void(const std::vector<int>&, std::vector<int>&, size_t, int)> 
            gerarCombinacoesCandidatos = [&](
                const std::vector<int>& candidatos, 
                std::vector<int>& combinacaoAtual, 
                size_t inicio, 
                int k
            ) {
                // Caso base: completamos a combinação
                if (combinacaoAtual.size() == k) {
                    // Calcular unidades adicionadas
                    int unidadesAdicionadas = 0;
                    for (int pedidoId : combinacaoAtual) {
                        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                            unidadesAdicionadas += quantidade;
                        }
                    }
                    
                    // Verificar viabilidade da troca (LB e UB)
                    int novoTotal = totalUnidades - unidadesRemovidas + unidadesAdicionadas;
                    if (novoTotal >= LB && novoTotal <= UB) {
                        // Criar movimento de chain-exchange
                        Movimento movimento;
                        movimento.tipo = TipoMovimento::CHAIN_EXCHANGE;
                        movimento.pedidosRemover = pedidosRemover;
                        movimento.pedidosAdicionar = combinacaoAtual;
                        
                        // Simular o movimento para calcular seu impacto
                        Solucao solucaoTemp = solucao;
                        
                        // Remover pedidos
                        for (int p : pedidosRemover) {
                            auto it = std::find(solucaoTemp.pedidosWave.begin(), 
                                              solucaoTemp.pedidosWave.end(), p);
                            if (it != solucaoTemp.pedidosWave.end()) {
                                solucaoTemp.pedidosWave.erase(it);
                            }
                        }
                        
                        // Adicionar novos pedidos
                        for (int p : combinacaoAtual) {
                            solucaoTemp.pedidosWave.push_back(p);
                        }
                        
                        // Recalcular corredores
                        std::unordered_set<int> corredores;
                        for (int p : solucaoTemp.pedidosWave) {
                            for (const auto& [itemId, _] : backlog_.pedido[p]) {
                                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                                    corredores.insert(corredorId);
                                }
                            }
                        }
                        
                        solucaoTemp.corredoresWave.assign(corredores.begin(), corredores.end());
                        solucaoTemp.totalUnidades = novoTotal;
                        
                        // Calcular valor objetivo
                        calcularValorObjetivo(solucaoTemp);
                        
                        // Calcular delta
                        movimento.deltaValorObjetivo = solucaoTemp.valorObjetivo - solucao.valorObjetivo;
                        
                        // Adicionar movimento se promissor (delta próximo de zero ou positivo)
                        if (movimento.deltaValorObjetivo > -0.05) {
                            movimentos.push_back(movimento);
                        }
                    }
                    return;
                }
                
                // Recursão: construir combinação
                for (size_t i = inicio; i < candidatos.size(); i++) {
                    combinacaoAtual.push_back(candidatos[i]);
                    gerarCombinacoesCandidatos(candidatos, combinacaoAtual, i + 1, k);
                    combinacaoAtual.pop_back();
                }
            };
            
            // Gerar combinações de pedidos a adicionar
            std::vector<int> combinacaoCandidatos;
            gerarCombinacoesCandidatos(candidatosAvaliados, combinacaoCandidatos, 0, k);
            
            return;
        }
        
        // Recursão: construir combinação de pedidos a remover
        for (size_t i = inicio; i < pedidosOrigem.size(); i++) {
            combinacaoAtual.push_back(pedidosOrigem[i]);
            gerarCombinacoes(pedidosOrigem, combinacaoAtual, i + 1, k);
            combinacaoAtual.pop_back();
        }
    };
    
    // Iniciar geração de combinações
    std::vector<int> combinacaoAtual;
    gerarCombinacoes(solucao.pedidosWave, combinacaoAtual, 0, tamanhoCadeia);
    
    // Limitar número de movimentos para instâncias grandes
    int maxMovimentos = (backlog_.numPedidos > 500) ? 50 : 
                        (backlog_.numPedidos > 200) ? 100 : 200;
    
    if (movimentos.size() > maxMovimentos) {
        // Ordenar por delta (mais promissores primeiro)
        std::sort(movimentos.begin(), movimentos.end(), 
                 [](const Movimento& a, const Movimento& b) {
                     return a.deltaValorObjetivo > b.deltaValorObjetivo;
                 });
        
        // Manter apenas os melhores movimentos
        movimentos.resize(maxMovimentos);
    }
    
    return movimentos;
}

// Gerar movimentos usando Path Relinking
std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosPathRelinking(
    const Solucao& solucaoAtual,
    const Solucao& solucaoGuia,
    int LB, int UB
) {
    std::vector<Movimento> movimentos;
    
    // Identificar diferenças entre solução atual e guia
    
    // Pedidos na guia que não estão na solução atual (candidatos a adicionar)
    std::vector<int> pedidosAdicionar;
    for (int pedidoId : solucaoGuia.pedidosWave) {
        if (std::find(solucaoAtual.pedidosWave.begin(), solucaoAtual.pedidosWave.end(), pedidoId) 
            == solucaoAtual.pedidosWave.end()) {
            pedidosAdicionar.push_back(pedidoId);
        }
    }
    
    // Pedidos na solução atual que não estão na guia (candidatos a remover)
    std::vector<int> pedidosRemover;
    for (int pedidoId : solucaoAtual.pedidosWave) {
        if (std::find(solucaoGuia.pedidosWave.begin(), solucaoGuia.pedidosWave.end(), pedidoId) 
            == solucaoGuia.pedidosWave.end()) {
            pedidosRemover.push_back(pedidoId);
        }
    }
    
    // Total de unidades na solução atual
    int totalUnidades = 0;
    for (int pedidoId : solucaoAtual.pedidosWave) {
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Gerar movimentos de path relinking (swap um-a-um)
    for (int pedidoRemover : pedidosRemover) {
        // Calcular unidades a remover
        int unidadesRemover = 0;
        for (const auto& [_, quantidade] : backlog_.pedido[pedidoRemover]) {
            unidadesRemover += quantidade;
        }
        
        for (int pedidoAdicionar : pedidosAdicionar) {
            // Calcular unidades a adicionar
            int unidadesAdicionar = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoAdicionar]) {
                unidadesAdicionar += quantidade;
            }
            
            // Verificar viabilidade do movimento
            int novoTotal = totalUnidades - unidadesRemover + unidadesAdicionar;
            if (novoTotal >= LB && novoTotal <= UB) {
                // Criar o movimento
                Movimento movimento;
                movimento.tipo = TipoMovimento::PATH_RELINKING;
                movimento.pedidosRemover = {pedidoRemover};
                movimento.pedidosAdicionar = {pedidoAdicionar};
                
                // Calcular impacto
                Solucao solucaoTemp = solucaoAtual;
                
                // Remover pedido
                auto it = std::find(solucaoTemp.pedidosWave.begin(), 
                                  solucaoTemp.pedidosWave.end(), 
                                  pedidoRemover);
                if (it != solucaoTemp.pedidosWave.end()) {
                    solucaoTemp.pedidosWave.erase(it);
                }
                
                // Adicionar novo pedido
                solucaoTemp.pedidosWave.push_back(pedidoAdicionar);
                
                // Recalcular corredores
                std::unordered_set<int> corredores;
                for (int p : solucaoTemp.pedidosWave) {
                    for (const auto& [itemId, _] : backlog_.pedido[p]) {
                        for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                            corredores.insert(corredorId);
                        }
                    }
                }
                
                solucaoTemp.corredoresWave.assign(corredores.begin(), corredores.end());
                solucaoTemp.totalUnidades = novoTotal;
                
                // Calcular valor objetivo
                calcularValorObjetivo(solucaoTemp);
                
                // Calcular delta
                movimento.deltaValorObjetivo = solucaoTemp.valorObjetivo - solucaoAtual.valorObjetivo;
                
                // Adicionar movimento
                movimentos.push_back(movimento);
            }
        }
    }
    
    // Ordenar movimentos por Delta (melhores primeiro)
    std::sort(movimentos.begin(), movimentos.end(), 
             [](const Movimento& a, const Movimento& b) {
                 return a.deltaValorObjetivo > b.deltaValorObjetivo;
             });
    
    // Para instâncias grandes, limitar número de movimentos
    int maxMovimentos = (backlog_.numPedidos > 500) ? 30 : 
                       (backlog_.numPedidos > 200) ? 50 : 100;
    
    if (movimentos.size() > maxMovimentos) {
        movimentos.resize(maxMovimentos);
    }
    
    return movimentos;
}

// Completar a implementação da função calcularValorObjetivo
double BuscaLocalAvancada::calcularValorObjetivo(Solucao& solucao) {
    // Se não há pedidos ou corredores, o valor objetivo é zero
    if (solucao.pedidosWave.empty() || solucao.corredoresWave.empty()) {
        solucao.valorObjetivo = 0.0;
        return solucao.valorObjetivo;
    }
    
    // Contagem total de unidades na solução
    int totalUnidades = 0;
    
    // Calcular total de unidades em todos os pedidos selecionados
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    // Armazenar o total de unidades para referência futura
    solucao.totalUnidades = totalUnidades;
    
    // Calcular valor objetivo: unidades por corredor (eficiência)
    // Quanto maior este valor, melhor a solução
    solucao.valorObjetivo = static_cast<double>(totalUnidades) / solucao.corredoresWave.size();
    return solucao.valorObjetivo;
}

// Aplicar um movimento a uma solução
BuscaLocalAvancada::Solucao BuscaLocalAvancada::aplicarMovimento(
    const Solucao& solucao, 
    const Movimento& movimento
) {
    // Criar nova solução como cópia da original
    Solucao novaSolucao = solucao;
    
    // Aplicar movimento de acordo com seu tipo
    switch (movimento.tipo) {
        case TipoMovimento::ADICIONAR: {
            // Adicionar pedidos à solução
            for (int pedidoId : movimento.pedidosAdicionar) {
                // Verificar se o pedido já está na solução
                if (std::find(novaSolucao.pedidosWave.begin(), 
                             novaSolucao.pedidosWave.end(), 
                             pedidoId) == novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.push_back(pedidoId);
                }
            }
            break;
        }
        
        case TipoMovimento::REMOVER: {
            // Remover pedidos da solução
            for (int pedidoId : movimento.pedidosRemover) {
                auto it = std::find(novaSolucao.pedidosWave.begin(), 
                                   novaSolucao.pedidosWave.end(), 
                                   pedidoId);
                if (it != novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.erase(it);
                }
            }
            break;
        }
        
        case TipoMovimento::SWAP: {
            // Remover pedidos
            for (int pedidoId : movimento.pedidosRemover) {
                auto it = std::find(novaSolucao.pedidosWave.begin(), 
                                   novaSolucao.pedidosWave.end(), 
                                   pedidoId);
                if (it != novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.erase(it);
                }
            }
            
            // Adicionar novos pedidos
            for (int pedidoId : movimento.pedidosAdicionar) {
                if (std::find(novaSolucao.pedidosWave.begin(), 
                             novaSolucao.pedidosWave.end(), 
                             pedidoId) == novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.push_back(pedidoId);
                }
            }
            break;
        }
        
        case TipoMovimento::CHAIN_EXCHANGE: {
            // Troca em cadeia - remove uma sequência e adiciona outra
            for (int pedidoId : movimento.pedidosRemover) {
                auto it = std::find(novaSolucao.pedidosWave.begin(), 
                                   novaSolucao.pedidosWave.end(), 
                                   pedidoId);
                if (it != novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.erase(it);
                }
            }
            
            for (int pedidoId : movimento.pedidosAdicionar) {
                if (std::find(novaSolucao.pedidosWave.begin(), 
                             novaSolucao.pedidosWave.end(), 
                             pedidoId) == novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.push_back(pedidoId);
                }
            }
            break;
        }
        
        case TipoMovimento::PATH_RELINKING: {
            // Aplicar movimento de path relinking
            // Similar ao SWAP, mas orientado a convergir para solução guia
            for (int pedidoId : movimento.pedidosRemover) {
                auto it = std::find(novaSolucao.pedidosWave.begin(), 
                                   novaSolucao.pedidosWave.end(), 
                                   pedidoId);
                if (it != novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.erase(it);
                }
            }
            
            for (int pedidoId : movimento.pedidosAdicionar) {
                if (std::find(novaSolucao.pedidosWave.begin(), 
                             novaSolucao.pedidosWave.end(), 
                             pedidoId) == novaSolucao.pedidosWave.end()) {
                    novaSolucao.pedidosWave.push_back(pedidoId);
                }
            }
            break;
        }
    }
    
    // Recalcular corredores necessários após os movimentos
    std::unordered_set<int> corredores;
    for (int pedidoId : novaSolucao.pedidosWave) {
        for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredores.insert(corredorId);
            }
        }
    }
    
    // Converter conjunto para vetor
    novaSolucao.corredoresWave.assign(corredores.begin(), corredores.end());
    
    // Recalcular valor objetivo
    calcularValorObjetivo(novaSolucao);
    
    return novaSolucao;
}

// Perturbar uma solução (para ILS e VNS)
BuscaLocalAvancada::Solucao BuscaLocalAvancada::perturbarSolucao(
    const Solucao& solucao, 
    double intensidade,
    int LB, int UB
) {
    Solucao novaSolucao = solucao;
    
    // Quantidade de perturbações baseada na intensidade (proporção do tamanho)
    int numPerturbacoes = std::max(1, static_cast<int>(intensidade * solucao.pedidosWave.size()));
    
    // Registrar pedidos originais para evitar reimportação acidental
    std::unordered_set<int> pedidosOriginais(
        novaSolucao.pedidosWave.begin(), 
        novaSolucao.pedidosWave.end()
    );
    
    // Identificar pedidos candidatos a entrar na solução
    std::vector<int> pedidosCandidatos;
    for (int p = 0; p < backlog_.numPedidos; p++) {
        if (pedidosOriginais.find(p) == pedidosOriginais.end()) {
            pedidosCandidatos.push_back(p);
        }
    }
    
    // Se não houver pedidos candidatos, aumentar força da perturbação via remoção
    if (pedidosCandidatos.empty()) {
        numPerturbacoes = std::min(numPerturbacoes * 2, 
                                 static_cast<int>(novaSolucao.pedidosWave.size() / 2));
        
        // Apenas remover pedidos
        for (int i = 0; i < numPerturbacoes && !novaSolucao.pedidosWave.empty(); i++) {
            std::uniform_int_distribution<> dist(0, novaSolucao.pedidosWave.size() - 1);
            int idx = dist(rng_);
            novaSolucao.pedidosWave.erase(novaSolucao.pedidosWave.begin() + idx);
        }
    } else {
        // Remover alguns pedidos aleatoriamente
        for (int i = 0; i < numPerturbacoes && !novaSolucao.pedidosWave.empty(); i++) {
            std::uniform_int_distribution<> dist(0, novaSolucao.pedidosWave.size() - 1);
            int idx = dist(rng_);
            novaSolucao.pedidosWave.erase(novaSolucao.pedidosWave.begin() + idx);
        }
        
        // Adicionar novos pedidos aleatoriamente
        for (int i = 0; i < numPerturbacoes && !pedidosCandidatos.empty(); i++) {
            std::uniform_int_distribution<> dist(0, pedidosCandidatos.size() - 1);
            int idx = dist(rng_);
            int pedidoId = pedidosCandidatos[idx];
            
            novaSolucao.pedidosWave.push_back(pedidoId);
            
            // Remover este pedido dos candidatos para evitar duplicação
            pedidosCandidatos.erase(pedidosCandidatos.begin() + idx);
        }
    }
    
    // Recalcular corredores e total de unidades
    std::unordered_set<int> corredores;
    int totalUnidades = 0;
    
    for (int pedidoId : novaSolucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            totalUnidades += quantidade;
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredores.insert(corredorId);
            }
        }
    }
    
    // Verificar restrições de unidades (LB e UB)
    // Se solução violar LB, adicionar pedidos aleatórios até satisfazer
    std::shuffle(pedidosCandidatos.begin(), pedidosCandidatos.end(), rng_);
    
    while (totalUnidades < LB && !pedidosCandidatos.empty()) {
        int pedidoId = pedidosCandidatos.back();
        pedidosCandidatos.pop_back();
        
        // Calcular impacto do pedido
        int unidadesPedido = 0;
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }
        
        // Adicionar se não violar UB
        if (totalUnidades + unidadesPedido <= UB) {
            novaSolucao.pedidosWave.push_back(pedidoId);
            totalUnidades += unidadesPedido;
            
            // Atualizar corredores
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    corredores.insert(corredorId);
                }
            }
        }
    }
    
    // Se solução violar UB, remover pedidos até satisfazer
    while (totalUnidades > UB && !novaSolucao.pedidosWave.empty()) {
        // Escolher pedido para remover
        std::uniform_int_distribution<> dist(0, novaSolucao.pedidosWave.size() - 1);
        int idx = dist(rng_);
        int pedidoId = novaSolucao.pedidosWave[idx];
        
        // Calcular unidades do pedido
        int unidadesPedido = 0;
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesPedido += quantidade;
        }
        
        // Remover pedido
        novaSolucao.pedidosWave.erase(novaSolucao.pedidosWave.begin() + idx);
        totalUnidades -= unidadesPedido;
        
        // Recalcular corredores
        corredores.clear();
        for (int p : novaSolucao.pedidosWave) {
            for (const auto& [itemId, _] : backlog_.pedido[p]) {
                for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                    corredores.insert(corredorId);
                }
            }
        }
    }
    
    // Se a solução ainda não satisfaz LB, retornar a original
    if (totalUnidades < LB) {
        return solucao;
    }
    
    // Atualizar corredores e recalcular valor objetivo
    novaSolucao.corredoresWave.assign(corredores.begin(), corredores.end());
    novaSolucao.totalUnidades = totalUnidades;
    calcularValorObjetivo(novaSolucao);
    
    return novaSolucao;
}

BuscaLocalAvancada::Solucao BuscaLocalAvancada::buscaLocalBasica(
    const Solucao& solucao,
    int tipoVizinhanca,
    int LB, int UB
) {
    Solucao melhorSolucao = solucao;
    Solucao solucaoAtual = solucao;
    
    // Calcular valor objetivo para garantir que esteja atualizado
    calcularValorObjetivo(melhorSolucao);
    calcularValorObjetivo(solucaoAtual);
    
    bool melhorou = true;
    int iteracao = 0;
    const int MAX_ITERACOES = 50;  // Limite de iterações para busca local rápida
    
    while (melhorou && iteracao < MAX_ITERACOES && !tempoExcedido()) {
        melhorou = false;
        iteracao++;
        
        // Gerar vizinhança apropriada baseada no tipo
        std::vector<Movimento> movimentos;
        
        switch (tipoVizinhanca) {
            case 0: // SWAP_SIMPLE
                movimentos = gerarMovimentosSwap(solucaoAtual, LB, UB);
                break;
            case 1: // SWAP_CHAIN_1X2
                // Função auxiliar que gera apenas movimentos 1x2
                // (não implementada aqui, mas seria uma versão filtrada de gerarMovimentosChainExchange)
                movimentos = gerarMovimentosSwap(solucaoAtual, LB, UB);
                break;
            case 2: // SWAP_CHAIN_2X1
                // Similar ao caso anterior
                movimentos = gerarMovimentosSwap(solucaoAtual, LB, UB);
                break;
            case 3: // SWAP_CHAIN_2X2
                movimentos = gerarMovimentosChainExchange(solucaoAtual, LB, UB);
                break;
            default:
                movimentos = gerarMovimentosSwap(solucaoAtual, LB, UB);
        }
        
        // Ordenar movimentos por valor objetivo decrescente
        std::sort(movimentos.begin(), movimentos.end(),
                 [](const Movimento& a, const Movimento& b) {
                     return a.deltaValorObjetivo > b.deltaValorObjetivo;
                 });
        
        // Aplicar o melhor movimento que melhora a solução
        for (const auto& movimento : movimentos) {
            if (movimento.deltaValorObjetivo > 0) {
                solucaoAtual = aplicarMovimento(solucaoAtual, movimento);
                melhorou = true;
                
                // Verificar se melhorou a melhor solução
                if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                    melhorSolucao = solucaoAtual;
                }
                
                break;  // Sair depois de aplicar o melhor movimento
            }
        }
    }
    
    return melhorSolucao;
}

void BuscaLocalAvancada::inicializarMemoriaLongoPrazo(int numPedidos) {
    frequenciaPedidos_.resize(numPedidos, 0);
    recenciaPedidos_.resize(numPedidos, 0);
    qualidadePedidos_.resize(numPedidos, 0.0);
}

std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosIntensificacao(
    const Solucao& solucao, 
    int LB, int UB
) {
    std::vector<Movimento> movimentos;
    
    // Identificar pedidos de alta qualidade que não estão na solução atual
    std::vector<std::pair<double, int>> pedidosDeAltaQualidade;
    
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) 
            == solucao.pedidosWave.end() && 
            verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
            
            if (qualidadePedidos_[pedidoId] > 0) {
                pedidosDeAltaQualidade.push_back({qualidadePedidos_[pedidoId], pedidoId});
            }
        }
    }
    
    // Ordenar por qualidade descendente
    std::sort(pedidosDeAltaQualidade.begin(), pedidosDeAltaQualidade.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Limitar à metade superior dos pedidos de alta qualidade
    int limite = std::min(20, static_cast<int>(pedidosDeAltaQualidade.size() / 2) + 1);
    
    // Criar movimentos específicos para intensificação, tentar incluir pedidos de alta qualidade
    for (int i = 0; i < limite && i < static_cast<int>(pedidosDeAltaQualidade.size()); i++) {
        int pedidoCandidato = pedidosDeAltaQualidade[i].second;
        
        // Para cada pedido de baixa qualidade na solução atual, considerar trocá-lo
        for (int pedidoAtual : solucao.pedidosWave) {
            // Verificar se a troca mantém a solução dentro dos limites LB e UB
            int unidadesPedidoAtual = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoAtual]) {
                unidadesPedidoAtual += quantidade;
            }
            
            int unidadesPedidoNovo = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoCandidato]) {
                unidadesPedidoNovo += quantidade;
            }
            
            int novoTotalUnidades = solucao.totalUnidades - unidadesPedidoAtual + unidadesPedidoNovo;
            
            if (novoTotalUnidades >= LB && novoTotalUnidades <= UB) {
                Movimento movimento;
                movimento.tipo = TipoMovimento::SWAP;
                movimento.pedidosRemover.push_back(pedidoAtual);
                movimento.pedidosAdicionar.push_back(pedidoCandidato);
                
                // Estimar o impacto da troca
                Solucao novaSolucao = aplicarMovimento(solucao, movimento);
                double novoValorObjetivo = calcularValorObjetivo(novaSolucao);
                movimento.deltaValorObjetivo = novoValorObjetivo - solucao.valorObjetivo;
                
                // Adicionar movimento mesmo com impacto levemente negativo, para permitir exploração
                if (movimento.deltaValorObjetivo > -0.01) {
                    movimentos.push_back(movimento);
                }
            }
        }
    }
    
    return movimentos;
}

std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosDiversificacao(
    const Solucao& solucao, 
    int LB, int UB
) {
    std::vector<Movimento> movimentos;
    
    // Identificar pedidos raramente usados (baixa frequência)
    std::vector<std::pair<int, int>> pedidosRaros;
    
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) 
            == solucao.pedidosWave.end() && 
            verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
            
            pedidosRaros.push_back({frequenciaPedidos_[pedidoId], pedidoId});
        }
    }
    
    // Ordenar por frequência ascendente (menos usados primeiro)
    std::sort(pedidosRaros.begin(), pedidosRaros.end());
    
    // Identificar pedidos frequentemente usados na solução atual
    std::vector<std::pair<int, int>> pedidosFrequentes;
    for (int pedidoId : solucao.pedidosWave) {
        pedidosFrequentes.push_back({frequenciaPedidos_[pedidoId], pedidoId});
    }
    
    // Ordenar por frequência descendente (mais usados primeiro)
    std::sort(pedidosFrequentes.begin(), pedidosFrequentes.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Limite para considerar apenas os mais frequentes/raros
    int limite = std::min(10, static_cast<int>(std::min(pedidosRaros.size(), pedidosFrequentes.size())));
    
    // Tentar trocar pedidos frequentes por pedidos raros
    for (int i = 0; i < limite && i < static_cast<int>(pedidosFrequentes.size()); i++) {
        int pedidoFrequente = pedidosFrequentes[i].second;
        
        for (int j = 0; j < limite && j < static_cast<int>(pedidosRaros.size()); j++) {
            int pedidoRaro = pedidosRaros[j].second;
            
            // Verificar viabilidade da troca
            int unidadesPedidoFrequente = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoFrequente]) {
                unidadesPedidoFrequente += quantidade;
            }
            
            int unidadesPedidoRaro = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoRaro]) {
                unidadesPedidoRaro += quantidade;
            }
            
            int novoTotalUnidades = solucao.totalUnidades - unidadesPedidoFrequente + unidadesPedidoRaro;
            
            if (novoTotalUnidades >= LB && novoTotalUnidades <= UB) {
                Movimento movimento;
                movimento.tipo = TipoMovimento::SWAP;
                movimento.pedidosRemover.push_back(pedidoFrequente);
                movimento.pedidosAdicionar.push_back(pedidoRaro);
                
                // Na diversificação, aceitamos movimentos mesmo com impacto levemente negativo
                // para explorar regiões não visitadas
                movimentos.push_back(movimento);
            }
        }
    }
    
    // Se não encontramos muitos movimentos, adicionar também chain-exchange para diversificação
    if (movimentos.size() < 5) {
        auto chainMovimentos = gerarMovimentosChainExchange(solucao, LB, UB);
        movimentos.insert(movimentos.end(), chainMovimentos.begin(), chainMovimentos.end());
    }
    
    return movimentos;
}