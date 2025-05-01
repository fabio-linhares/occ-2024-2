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
    // Inicialização
    Solucao melhorSolucao = solucaoInicial;
    Solucao solucaoAtual = solucaoInicial;
    
    // Calcular valor objetivo para garantir que esteja atualizado
    calcularValorObjetivo(melhorSolucao);
    calcularValorObjetivo(solucaoAtual);
    
    // Estrutura para lista tabu: <pedidoId, iteração até quando é tabu>
    std::unordered_map<int, int> listaTabu;
    
    // Frequência para memória de longo prazo
    std::unordered_map<int, int> frequenciaPedidos;
    
    int iteracaoAtual = 0;
    int iteracoesSemMelhoria = 0;
    bool intensificacao = false;
    bool diversificacao = false;
    
    // Loop principal da busca tabu
    while (!tempoExcedido() && iteracoesSemMelhoria < configTabu_.maxIteracoesSemMelhoria) {
        // Gerar vizinhança
        std::vector<Movimento> movimentos;
        
        if (intensificacao) {
            // Durante intensificação, focar em movimentos de pequeno impacto
            movimentos = gerarMovimentosSwap(solucaoAtual, LB, UB);
        } else if (diversificacao) {
            // Durante diversificação, usar chain-exchange para movimentos maiores
            movimentos = gerarMovimentosChainExchange(solucaoAtual, LB, UB);
        } else {
            // Normalmente, usar uma mistura de movimentos
            auto swaps = gerarMovimentosSwap(solucaoAtual, LB, UB);
            auto chains = gerarMovimentosChainExchange(solucaoAtual, LB, UB);
            
            movimentos.insert(movimentos.end(), swaps.begin(), swaps.end());
            movimentos.insert(movimentos.end(), chains.begin(), chains.end());
        }
        
        // Filtrar movimentos tabu, a menos que satisfaçam critério de aspiração
        std::vector<Movimento> movimentosPermitidos;
        for (const auto& movimento : movimentos) {
            bool tabu = false;
            
            // Verificar se o movimento é tabu
            for (int pedidoId : movimento.pedidosRemover) {
                if (listaTabu.find(pedidoId) != listaTabu.end() && 
                    listaTabu[pedidoId] > iteracaoAtual) {
                    tabu = true;
                    break;
                }
            }
            
            for (int pedidoId : movimento.pedidosAdicionar) {
                if (listaTabu.find(pedidoId) != listaTabu.end() && 
                    listaTabu[pedidoId] > iteracaoAtual) {
                    tabu = true;
                    break;
                }
            }
            
            // Critério de aspiração: aceitar movimento tabu se ele melhora a melhor solução
            Solucao novaSolucao = aplicarMovimento(solucaoAtual, movimento);
            double novoValor = calcularValorObjetivo(novaSolucao);
            
            if (!tabu || novoValor > melhorSolucao.valorObjetivo) {
                movimentosPermitidos.push_back(movimento);
            }
        }
        
        if (movimentosPermitidos.empty()) {
            // Se não há movimentos permitidos, diversificar
            diversificacao = true;
            intensificacao = false;
            continue;
        }
        
        // Selecionar o melhor movimento não tabu
        auto melhorMovimento = *std::max_element(movimentosPermitidos.begin(), movimentosPermitidos.end(),
            [](const Movimento& a, const Movimento& b) {
                return a.deltaValorObjetivo < b.deltaValorObjetivo;
            });
        
        // Aplicar o movimento
        solucaoAtual = aplicarMovimento(solucaoAtual, melhorMovimento);
        
        // Atualizar lista tabu
        int duracaoTabu = configTabu_.duracaoTabuBase;
        
        // Duração tabu dinâmica
        if (iteracoesSemMelhoria > configTabu_.maxIteracoesSemMelhoria / 2) {
            // Aumentar duração tabu quando estagnado
            duracaoTabu = configTabu_.duracaoTabuBase * 2;
        }
        
        for (int pedidoId : melhorMovimento.pedidosRemover) {
            listaTabu[pedidoId] = iteracaoAtual + duracaoTabu;
        }
        
        for (int pedidoId : melhorMovimento.pedidosAdicionar) {
            listaTabu[pedidoId] = iteracaoAtual + duracaoTabu;
        }
        
        // Atualizar frequência para memória de longo prazo
        if (configTabu_.usarMemoriaLongoPrazo) {
            for (int pedidoId : melhorMovimento.pedidosRemover) {
                frequenciaPedidos[pedidoId]++;
            }
            
            for (int pedidoId : melhorMovimento.pedidosAdicionar) {
                frequenciaPedidos[pedidoId]++;
            }
        }
        
        // Verificar se melhorou a melhor solução
        if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
            melhorSolucao = solucaoAtual;
            iteracoesSemMelhoria = 0;
            estatisticas_.melhorias++;
            
            // Desativar mecanismos de intensificação/diversificação após melhoria
            intensificacao = false;
            diversificacao = false;
        } else {
            iteracoesSemMelhoria++;
        }
        
        // Verificar se é momento de intensificação ou diversificação
        if (configTabu_.usarMemoriaLongoPrazo) {
            if (iteracoesSemMelhoria > 0 && iteracoesSemMelhoria % configTabu_.ciclosIntensificacao == 0) {
                intensificacao = true;
                diversificacao = false;
            } else if (iteracoesSemMelhoria > 0 && iteracoesSemMelhoria % configTabu_.ciclosDiversificacao == 0) {
                intensificacao = false;
                diversificacao = true;
            }
        }
        
        iteracaoAtual++;
        estatisticas_.iteracoesTotais++;
    }
    
    // Registrar tempo de execução nas estatísticas
    auto tempoFim = std::chrono::high_resolution_clock::now();
    estatisticas_.tempoExecucaoMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tempoFim - tempoInicio_).count();
    
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
            movimento.tipo = Movimento::Tipo::SWAP;
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

BuscaLocalAvancada::Solucao BuscaLocalAvancada::vns(const Solucao& solucaoInicial, int LB, int UB) {
    // Inicialização
    iniciarEstatisticas(solucaoInicial);
    
    Solucao melhorSolucao = solucaoInicial;
    Solucao solucaoAtual = solucaoInicial;
    
    // Calcular valor objetivo para garantir que esteja atualizado
    calcularValorObjetivo(melhorSolucao);
    calcularValorObjetivo(solucaoAtual);
    
    // Estatísticas iniciais
    estatisticas_.valorObjetivoInicial = solucaoInicial.valorObjetivo;
    estatisticas_.melhorValorObjetivo = solucaoInicial.valorObjetivo;
    
    // Definir estruturas de vizinhança
    enum Vizinhanca {
        SWAP_SIMPLE = 0,     // Troca simples de 1x1 pedidos
        SWAP_CHAIN_1X2 = 1,  // Troca de 1 por 2 pedidos
        SWAP_CHAIN_2X1 = 2,  // Troca de 2 por 1 pedidos
        SWAP_CHAIN_2X2 = 3   // Troca de 2 por 2 pedidos
    };
    
    int k = 0;  // Índice da vizinhança atual
    int kMax = configVNS_.kMax;  // Número máximo de vizinhanças
    
    int iteracoesSemMelhoria = 0;
    int iteracao = 0;
    
    // Loop principal do VNS
    while (iteracao < configVNS_.maxIteracoesSemMelhoria && !tempoExcedido()) {
        iteracao++;
        estatisticas_.iteracoesTotais++;
        
        // 1. Shaking (Perturbação) - Gerar solução da vizinhança k
        Solucao solucaoPerturbada = perturbarSolucao(solucaoAtual, k, LB, UB);
        estatisticas_.perturbacoes++;
        
        // 2. Local Search - Busca local a partir da solução perturbada
        Solucao solucaoMelhorada = buscaLocalBasica(solucaoPerturbada, k, LB, UB);
        estatisticas_.buscasLocais++;
        
        // 3. Move or Not - Decidir se aceita a nova solução
        if (solucaoMelhorada.valorObjetivo > solucaoAtual.valorObjetivo) {
            // Aceitar a solução melhorada
            solucaoAtual = solucaoMelhorada;
            k = 0;  // Reiniciar com a primeira vizinhança
            iteracoesSemMelhoria = 0;
            estatisticas_.shakesSucedidos++;
            
            // Verificar se melhorou a melhor solução global
            if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                melhorSolucao = solucaoAtual;
                estatisticas_.melhorias++;
                estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
            }
        } else {
            // Não aceitar, mudar para próxima vizinhança
            k = (k + 1) % kMax;
            iteracoesSemMelhoria++;
            estatisticas_.mudancasVizinhanca++;
        }
        
        // Condição de parada
        if (iteracoesSemMelhoria >= configVNS_.maxIteracoesSemMelhoria) {
            break;
        }
    }
    
    // Registrar tempo de execução nas estatísticas
    auto tempoFim = std::chrono::high_resolution_clock::now();
    estatisticas_.tempoExecucaoMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tempoFim - tempoInicio_).count();
    
    // Calcular melhoria
    if (estatisticas_.valorObjetivoInicial > 0) {
        estatisticas_.melhoria = (estatisticas_.melhorValorObjetivo - estatisticas_.valorObjetivoInicial) 
                               / estatisticas_.valorObjetivoInicial * 100.0;
    }
    
    return melhorSolucao;
}

BuscaLocalAvancada::Solucao BuscaLocalAvancada::ils(const Solucao& solucaoInicial, int LB, int UB) {
    // Inicialização
    iniciarEstatisticas(solucaoInicial);
    
    Solucao melhorSolucao = solucaoInicial;
    Solucao solucaoAtual = solucaoInicial;
    
    // Calcular valor objetivo para garantir que esteja atualizado
    calcularValorObjetivo(melhorSolucao);
    calcularValorObjetivo(solucaoAtual);
    
    // Estatísticas iniciais
    estatisticas_.valorObjetivoInicial = solucaoInicial.valorObjetivo;
    estatisticas_.melhorValorObjetivo = solucaoInicial.valorObjetivo;
    
    // Realizar busca local inicial
    solucaoAtual = buscaLocalBasica(solucaoAtual, 0, LB, UB);
    estatisticas_.buscasLocais++;
    
    // Atualizar melhor solução se necessário
    if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
        melhorSolucao = solucaoAtual;
        estatisticas_.melhorias++;
        estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
    }
    
    int iteracoesSemMelhoria = 0;
    
    // Loop principal do ILS
    for (int i = 0; i < configILS_.maxIteracoes && !tempoExcedido(); i++) {
        estatisticas_.iteracoesTotais++;
        
        // 1. Perturbação - aumentar intensidade gradualmente se não houver melhoria
        double intensidadePerturbacao = configILS_.nivelPerturbacao * 
                                         (1.0 + iteracoesSemMelhoria / 10.0);
        
        // Limitar intensidade máxima
        intensidadePerturbacao = std::min(intensidadePerturbacao, 3.0);
        
        Solucao solucaoPerturbada = perturbarSolucao(solucaoAtual, intensidadePerturbacao, LB, UB);
        estatisticas_.perturbacoes++;
        
        // 2. Busca Local
        Solucao solucaoMelhorada = buscaLocalBasica(solucaoPerturbada, 0, LB, UB);
        estatisticas_.buscasLocais++;
        
        // 3. Critério de Aceitação - aceitar se melhorar a solução atual
        if (solucaoMelhorada.valorObjetivo > solucaoAtual.valorObjetivo) {
            solucaoAtual = solucaoMelhorada;
            iteracoesSemMelhoria = 0;
            
            // Verificar se melhorou a melhor solução global
            if (solucaoAtual.valorObjetivo > melhorSolucao.valorObjetivo) {
                melhorSolucao = solucaoAtual;
                estatisticas_.melhorias++;
                estatisticas_.melhorValorObjetivo = melhorSolucao.valorObjetivo;
            }
        } else {
            iteracoesSemMelhoria++;
            
            // Aceitação baseada em probabilidade decrescente (semelhante a SA)
            double delta = solucaoMelhorada.valorObjetivo - solucaoAtual.valorObjetivo;
            double temperatura = 0.1 * std::exp(-0.1 * iteracoesSemMelhoria);
            double probabilidade = std::exp(delta / temperatura);
            
            std::uniform_real_distribution<> dist(0.0, 1.0);
            if (dist(rng_) < probabilidade) {
                solucaoAtual = solucaoMelhorada;
            }
        }
        
        // Critério de parada - se não houve melhoria por muitas iterações
        if (iteracoesSemMelhoria >= configILS_.maxIteracoesSemMelhoria) {
            break;
        }
    }
    
    // Registrar tempo de execução nas estatísticas
    auto tempoFim = std::chrono::high_resolution_clock::now();
    estatisticas_.tempoExecucaoMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tempoFim - tempoInicio_).count();
    
    // Calcular melhoria
    if (estatisticas_.valorObjetivoInicial > 0) {
        estatisticas_.melhoria = (estatisticas_.melhorValorObjetivo - estatisticas_.valorObjetivoInicial) 
                               / estatisticas_.valorObjetivoInicial * 100.0;
    }
    
    return melhorSolucao;
}

std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosChainExchange(
    const Solucao& solucao, 
    int LB, int UB
) {
    std::vector<Movimento> movimentos;
    
    // Obter todos os pedidos disponíveis não incluídos na solução atual
    std::vector<int> pedidosForaDaWave;
    for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
        if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) 
            == solucao.pedidosWave.end() && 
            verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
            pedidosForaDaWave.push_back(pedidoId);
        }
    }
    
    // Pré-calcular informações para cada pedido (unidades e corredores)
    struct InfoPedido {
        int unidades;
        std::unordered_set<int> corredores;
    };
    
    std::unordered_map<int, InfoPedido> infoPedidos;
    
    // Para pedidos na wave
    for (int pedidoId : solucao.pedidosWave) {
        InfoPedido info;
        info.unidades = 0;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            info.unidades += quantidade;
            
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                info.corredores.insert(corredorId);
            }
        }
        
        infoPedidos[pedidoId] = info;
    }
    
    // Para pedidos fora da wave
    for (int pedidoId : pedidosForaDaWave) {
        InfoPedido info;
        info.unidades = 0;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            info.unidades += quantidade;
            
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                info.corredores.insert(corredorId);
            }
        }
        
        infoPedidos[pedidoId] = info;
    }
    
    // Definir limites para controle de explosão combinatória
    const int MAX_COMBINATIONS = std::min(2000, static_cast<int>(solucao.pedidosWave.size() * pedidosForaDaWave.size() / 2));
    int combinationsChecked = 0;
    
    // === Chain Exchange 2x2: Remover 2 pedidos e adicionar 2 pedidos ===
    if (solucao.pedidosWave.size() >= 2 && pedidosForaDaWave.size() >= 2) {
        // Para cada par de pedidos na wave
        for (size_t i = 0; i < solucao.pedidosWave.size() && combinationsChecked < MAX_COMBINATIONS; i++) {
            for (size_t j = i + 1; j < solucao.pedidosWave.size() && combinationsChecked < MAX_COMBINATIONS; j++) {
                int pedidoRemove1 = solucao.pedidosWave[i];
                int pedidoRemove2 = solucao.pedidosWave[j];
                
                int unidadesRemovidas = infoPedidos[pedidoRemove1].unidades + infoPedidos[pedidoRemove2].unidades;
                
                // Unir corredores dos pedidos a remover para análise de impacto
                std::unordered_set<int> corredoresRemovidos;
                corredoresRemovidos.insert(infoPedidos[pedidoRemove1].corredores.begin(), infoPedidos[pedidoRemove1].corredores.end());
                corredoresRemovidos.insert(infoPedidos[pedidoRemove2].corredores.begin(), infoPedidos[pedidoRemove2].corredores.end());
                
                // Para cada par de pedidos fora da wave
                for (size_t k = 0; k < pedidosForaDaWave.size() && combinationsChecked < MAX_COMBINATIONS; k++) {
                    for (size_t l = k + 1; l < pedidosForaDaWave.size() && combinationsChecked < MAX_COMBINATIONS; l++) {
                        combinationsChecked++;
                        
                        int pedidoAdd1 = pedidosForaDaWave[k];
                        int pedidoAdd2 = pedidosForaDaWave[l];
                        
                        int unidadesAdicionadas = infoPedidos[pedidoAdd1].unidades + infoPedidos[pedidoAdd2].unidades;
                        
                        // Verificar se a troca mantém a solução dentro dos limites
                        int novoTotalUnidades = solucao.totalUnidades - unidadesRemovidas + unidadesAdicionadas;
                        if (novoTotalUnidades < LB || novoTotalUnidades > UB) {
                            continue;
                        }
                        
                        // Criar movimento de troca
                        Movimento movimento;
                        movimento.tipo = Movimento::Tipo::CHAIN_EXCHANGE;
                        movimento.pedidosRemover = {pedidoRemove1, pedidoRemove2};
                        movimento.pedidosAdicionar = {pedidoAdd1, pedidoAdd2};
                        
                        // Estimar o impacto da troca
                        Solucao novaSolucao = aplicarMovimento(solucao, movimento);
                        double novoValorObjetivo = calcularValorObjetivo(novaSolucao);
                        movimento.deltaValorObjetivo = novoValorObjetivo - solucao.valorObjetivo;
                        
                        // Adicionar à lista de movimentos se for potencialmente benéfico
                        // ou com uma pequena tolerância negativa para diversificação
                        if (movimento.deltaValorObjetivo >= -0.005) {
                            movimentos.push_back(movimento);
                        }
                    }
                }
            }
        }
    }
    
    // === Chain Exchange 1x2: Remover 1 pedido e adicionar 2 pedidos ===
    if (solucao.pedidosWave.size() >= 1 && pedidosForaDaWave.size() >= 2) {
        combinationsChecked = 0; // Reset para este tipo de movimento
        
        for (int pedidoRemove : solucao.pedidosWave) {
            int unidadesRemovidas = infoPedidos[pedidoRemove].unidades;
            
            // Para cada par de pedidos fora da wave
            for (size_t k = 0; k < pedidosForaDaWave.size() && combinationsChecked < MAX_COMBINATIONS; k++) {
                for (size_t l = k + 1; l < pedidosForaDaWave.size() && combinationsChecked < MAX_COMBINATIONS; l++) {
                    combinationsChecked++;
                    
                    int pedidoAdd1 = pedidosForaDaWave[k];
                    int pedidoAdd2 = pedidosForaDaWave[l];
                    
                    int unidadesAdicionadas = infoPedidos[pedidoAdd1].unidades + infoPedidos[pedidoAdd2].unidades;
                    
                    // Verificar limites
                    int novoTotalUnidades = solucao.totalUnidades - unidadesRemovidas + unidadesAdicionadas;
                    if (novoTotalUnidades < LB || novoTotalUnidades > UB) {
                        continue;
                    }
                    
                    // Criar movimento
                    Movimento movimento;
                    movimento.tipo = Movimento::Tipo::CHAIN_EXCHANGE;
                    movimento.pedidosRemover = {pedidoRemove};
                    movimento.pedidosAdicionar = {pedidoAdd1, pedidoAdd2};
                    
                    // Estimar impacto
                    Solucao novaSolucao = aplicarMovimento(solucao, movimento);
                    double novoValorObjetivo = calcularValorObjetivo(novaSolucao);
                    movimento.deltaValorObjetivo = novoValorObjetivo - solucao.valorObjetivo;
                    
                    // Adicionar apenas se melhorar ou for próximo
                    if (movimento.deltaValorObjetivo >= -0.005) {
                        movimentos.push_back(movimento);
                    }
                }
            }
        }
    }
    
    // === Chain Exchange 2x1: Remover 2 pedidos e adicionar 1 pedido ===
    if (solucao.pedidosWave.size() >= 2 && pedidosForaDaWave.size() >= 1) {
        combinationsChecked = 0; // Reset para este tipo de movimento
        
        // Para cada par de pedidos na wave
        for (size_t i = 0; i < solucao.pedidosWave.size() && combinationsChecked < MAX_COMBINATIONS; i++) {
            for (size_t j = i + 1; j < solucao.pedidosWave.size() && combinationsChecked < MAX_COMBINATIONS; j++) {
                int pedidoRemove1 = solucao.pedidosWave[i];
                int pedidoRemove2 = solucao.pedidosWave[j];
                
                int unidadesRemovidas = infoPedidos[pedidoRemove1].unidades + infoPedidos[pedidoRemove2].unidades;
                
                // Para cada pedido fora da wave
                for (int pedidoAdd : pedidosForaDaWave) {
                    combinationsChecked++;
                    
                    int unidadesAdicionadas = infoPedidos[pedidoAdd].unidades;
                    
                    // Verificar limites
                    int novoTotalUnidades = solucao.totalUnidades - unidadesRemovidas + unidadesAdicionadas;
                    if (novoTotalUnidades < LB || novoTotalUnidades > UB) {
                        continue;
                    }
                    
                    // Criar movimento
                    Movimento movimento;
                    movimento.tipo = Movimento::Tipo::CHAIN_EXCHANGE;
                    movimento.pedidosRemover = {pedidoRemove1, pedidoRemove2};
                    movimento.pedidosAdicionar = {pedidoAdd};
                    
                    // Estimar impacto
                    Solucao novaSolucao = aplicarMovimento(solucao, movimento);
                    double novoValorObjetivo = calcularValorObjetivo(novaSolucao);
                    movimento.deltaValorObjetivo = novoValorObjetivo - solucao.valorObjetivo;
                    
                    // Este tipo de movimento geralmente reduz unidades, então só aceitamos se melhorar
                    if (movimento.deltaValorObjetivo > 0) {
                        movimentos.push_back(movimento);
                    }
                }
            }
        }
    }
    
    return movimentos;
}

std::vector<BuscaLocalAvancada::Movimento> BuscaLocalAvancada::gerarMovimentosPathRelinking(
    const Solucao& solucao,
    const Solucao& solucaoGuia,
    int LB, int UB
) {
    std::vector<Movimento> movimentos;
    
    // Identificar diferenças entre as soluções
    std::unordered_set<int> pedidosNaSolucao(solucao.pedidosWave.begin(), solucao.pedidosWave.end());
    std::unordered_set<int> pedidosNaGuia(solucaoGuia.pedidosWave.begin(), solucaoGuia.pedidosWave.end());
    
    // Pedidos para adicionar (estão na guia mas não na solução atual)
    std::vector<int> pedidosParaAdicionar;
    for (int pedidoId : solucaoGuia.pedidosWave) {
        if (pedidosNaSolucao.find(pedidoId) == pedidosNaSolucao.end()) {
            pedidosParaAdicionar.push_back(pedidoId);
        }
    }
    
    // Pedidos para remover (estão na solução atual mas não na guia)
    std::vector<int> pedidosParaRemover;
    for (int pedidoId : solucao.pedidosWave) {
        if (pedidosNaGuia.find(pedidoId) == pedidosNaGuia.end()) {
            pedidosParaRemover.push_back(pedidoId);
        }
    }
    
    // Tentar movimentos de swap entre pedidos para adicionar e remover
    for (int pedidoRemover : pedidosParaRemover) {
        for (int pedidoAdicionar : pedidosParaAdicionar) {
            // Verificar viabilidade da troca
            int unidadesPedidoRemover = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoRemover]) {
                unidadesPedidoRemover += quantidade;
            }
            
            int unidadesPedidoAdicionar = 0;
            for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoAdicionar]) {
                unidadesPedidoAdicionar += quantidade;
            }
            
            int novoTotalUnidades = solucao.totalUnidades - unidadesPedidoRemover + unidadesPedidoAdicionar;
            
            if (novoTotalUnidades >= LB && novoTotalUnidades <= UB) {
                Movimento movimento;
                movimento.tipo = Movimento::Tipo::SWAP;
                movimento.pedidosRemover.push_back(pedidoRemover);
                movimento.pedidosAdicionar.push_back(pedidoAdicionar);
                
                // Estimar o impacto
                Solucao novaSolucao = aplicarMovimento(solucao, movimento);
                double novoValorObjetivo = calcularValorObjetivo(novaSolucao);
                movimento.deltaValorObjetivo = novoValorObjetivo - solucao.valorObjetivo;
                
                // No path-relinking, aceitamos mesmo movimentos que não melhoram,
                // desde que nos aproximem da solução guia
                movimentos.push_back(movimento);
            }
        }
    }
    
    // Ordenar movimentos primeiro pelo valor objetivo (melhores primeiro)
    // e depois pela contribuição para aproximar as soluções
    std::sort(movimentos.begin(), movimentos.end(),
             [](const Movimento& a, const Movimento& b) {
                 if (a.deltaValorObjetivo > 0 && b.deltaValorObjetivo <= 0) {
                     return true;
                 }
                 if (a.deltaValorObjetivo <= 0 && b.deltaValorObjetivo > 0) {
                     return false;
                 }
                 return a.deltaValorObjetivo > b.deltaValorObjetivo;
             });
    
    return movimentos;
}

BuscaLocalAvancada::Solucao BuscaLocalAvancada::aplicarMovimento(
    const Solucao& solucao,
    const Movimento& movimento
) {
    Solucao novaSolucao = solucao;
    
    // Remover pedidos
    for (int pedidoId : movimento.pedidosRemover) {
        auto it = std::find(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end(), pedidoId);
        if (it != novaSolucao.pedidosWave.end()) {
            novaSolucao.pedidosWave.erase(it);
        }
    }
    
    // Adicionar pedidos
    for (int pedidoId : movimento.pedidosAdicionar) {
        if (std::find(novaSolucao.pedidosWave.begin(), novaSolucao.pedidosWave.end(), pedidoId) 
            == novaSolucao.pedidosWave.end()) {
            novaSolucao.pedidosWave.push_back(pedidoId);
        }
    }
    
    // Reconstruir lista de corredores e calcular unidades
    std::unordered_set<int> corredoresUnicos;
    novaSolucao.totalUnidades = 0;
    
    for (int pedidoId : novaSolucao.pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            novaSolucao.totalUnidades += quantidade;
            
            for (const auto& [corredorId, _] : localizador_.getCorredoresComItem(itemId)) {
                corredoresUnicos.insert(corredorId);
            }
        }
    }
    
    novaSolucao.corredoresWave.assign(corredoresUnicos.begin(), corredoresUnicos.end());
    
    // Atualizar valor objetivo
    calcularValorObjetivo(novaSolucao);
    
    return novaSolucao;
}

double BuscaLocalAvancada::calcularValorObjetivo(Solucao& solucao) {
    if (solucao.pedidosWave.empty() || solucao.corredoresWave.empty()) {
        solucao.valorObjetivo = 0.0;
        return 0.0;
    }
    
    // O valor objetivo é a relação unidades/corredores
    solucao.valorObjetivo = static_cast<double>(solucao.totalUnidades) / solucao.corredoresWave.size();
    return solucao.valorObjetivo;
}

bool BuscaLocalAvancada::solucaoViavel(const Solucao& solucao, int LB, int UB) const {
    // Verificar limites de unidades
    if (solucao.totalUnidades < LB || solucao.totalUnidades > UB) {
        return false;
    }
    return true;
}

BuscaLocalAvancada::Solucao BuscaLocalAvancada::perturbarSolucao(
    const Solucao& solucao,
    double intensidade,
    int LB, int UB
) {
    Solucao solucaoPerturbada = solucao;
    
    // Definir número de perturbações baseado na intensidade
    int numPerturbacoes = std::max(1, static_cast<int>(solucao.pedidosWave.size() * 0.1 * (1 + intensidade)));
    
    // Diferentes tipos de perturbação baseados no valor de intensidade
    for (int i = 0; i < numPerturbacoes; i++) {
        std::vector<Movimento> movimentosPossiveis;
        
        if (intensidade < 1.0) {
            // Perturbação leve: movimentos de swap simples
            auto swaps = gerarMovimentosSwap(solucaoPerturbada, LB, UB);
            movimentosPossiveis.insert(movimentosPossiveis.end(), swaps.begin(), swaps.end());
        } else if (intensidade < 2.0) {
            // Perturbação média: chain exchanges 1x2 ou 2x1
            std::vector<int> pedidosNaWave = solucaoPerturbada.pedidosWave;
            std::vector<int> pedidosForaDaWave;
            
            for (int pedidoId = 0; pedidoId < backlog_.numPedidos; pedidoId++) {
                if (std::find(pedidosNaWave.begin(), pedidosNaWave.end(), pedidoId) == pedidosNaWave.end() &&
                    verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
                    pedidosForaDaWave.push_back(pedidoId);
                }
            }
            
            // Escolher aleatoriamente se faz 1x2 ou 2x1
            if (!pedidosNaWave.empty() && pedidosForaDaWave.size() >= 2) {
                std::uniform_int_distribution<> dist(0, 1);
                if (dist(rng_) == 0 && pedidosNaWave.size() >= 2) {
                    // Remover 2, adicionar 1
                    std::uniform_int_distribution<> dist1(0, pedidosNaWave.size() - 1);
                    std::uniform_int_distribution<> dist2(0, pedidosNaWave.size() - 2);
                    std::uniform_int_distribution<> dist3(0, pedidosForaDaWave.size() - 1);
                    
                    int idx1 = dist1(rng_);
                    int idx2;
                    do {
                        idx2 = dist2(rng_);
                        if (idx2 >= idx1) idx2++;
                    } while (idx1 == idx2);
                    
                    int idxAdd = dist3(rng_);
                    
                    Movimento m;
                    m.tipo = Movimento::Tipo::CHAIN_EXCHANGE;
                    m.pedidosRemover = {pedidosNaWave[idx1], pedidosNaWave[idx2]};
                    m.pedidosAdicionar = {pedidosForaDaWave[idxAdd]};
                    
                    // Verificar viabilidade
                    Solucao testeSolucao = aplicarMovimento(solucaoPerturbada, m);
                    if (solucaoViavel(testeSolucao, LB, UB)) {
                        movimentosPossiveis.push_back(m);
                    }
                } else {
                    // Remover 1, adicionar 2
                    std::uniform_int_distribution<> dist1(0, pedidosNaWave.size() - 1);
                    std::uniform_int_distribution<> dist2(0, pedidosForaDaWave.size() - 1);
                    std::uniform_int_distribution<> dist3(0, pedidosForaDaWave.size() - 2);
                    
                    int idxRemove = dist1(rng_);
                    int idxAdd1 = dist2(rng_);
                    int idxAdd2;
                    do {
                        idxAdd2 = dist3(rng_);
                        if (idxAdd2 >= idxAdd1) idxAdd2++;
                    } while (idxAdd1 == idxAdd2);
                    
                    Movimento m;
                    m.tipo = Movimento::Tipo::CHAIN_EXCHANGE;
                    m.pedidosRemover = {pedidosNaWave[idxRemove]};
                    m.pedidosAdicionar = {pedidosForaDaWave[idxAdd1], pedidosForaDaWave[idxAdd2]};
                    
                    // Verificar viabilidade
                    Solucao testeSolucao = aplicarMovimento(solucaoPerturbada, m);
                    if (solucaoViavel(testeSolucao, LB, UB)) {
                        movimentosPossiveis.push_back(m);
                    }
                }
            }
        } else {
            // Perturbação forte: chain exchanges 2x2
            auto chains = gerarMovimentosChainExchange(solucaoPerturbada, LB, UB);
            movimentosPossiveis.insert(movimentosPossiveis.end(), chains.begin(), chains.end());
        }
        
        // Se encontramos movimentos possíveis, aplicar um aleatoriamente
        if (!movimentosPossiveis.empty()) {
            std::uniform_int_distribution<> dist(0, movimentosPossiveis.size() - 1);
            int idx = dist(rng_);
            solucaoPerturbada = aplicarMovimento(solucaoPerturbada, movimentosPossiveis[idx]);
        }
    }
    
    return solucaoPerturbada;
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
                movimento.tipo = Movimento::Tipo::SWAP;
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
                movimento.tipo = Movimento::Tipo::SWAP;
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