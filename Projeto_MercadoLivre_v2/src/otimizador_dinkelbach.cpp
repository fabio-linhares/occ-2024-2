#include "otimizador_dinkelbach.h"
#include "busca_local_avancada.h"
#include "branch_and_bound_solver.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>
#include <limits>
#include <set>
#include <unordered_set>
#include <queue>
#include <deque>
#include <string>
#include <sstream>
#include <functional>

OtimizadorDinkelbach::OtimizadorDinkelbach(
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador,
    const VerificadorDisponibilidade& verificador
) :
    OtimizadorWave(deposito, backlog, localizador, verificador),
    deposito_(deposito),
    backlog_(backlog),
    localizador_(localizador),
    verificador_(verificador),
    epsilon_(0.0001),
    maxIteracoes_(20000),
    usarBranchAndBound_(true),
    usarBuscaLocalAvancada_(true),
    limiteTempoBuscaLocal_(2.0)
{
    // Inicializar o gerador de números aleatórios com uma semente aleatória
    std::random_device rd;
    geradorPrincipal_.seed(rd());
    
    // Inicializar informações de convergência
    infoConvergencia_.iteracoesRealizadas = 0;
    infoConvergencia_.tempoTotal = 0.0;
    infoConvergencia_.convergiu = false;
    
    // Configurar reinicializações
    configReinicializacao_.numReinicializacoes = 1000; // Aumentado para maior diversificação
    configReinicializacao_.usarSementesAleatorias = true;
    configReinicializacao_.aumentarIteracoesProgressivamente = true;
    configReinicializacao_.variarPerturbacao = true;
    configReinicializacao_.guardarMelhoresSolucoes = true;
    configReinicializacao_.tamanhoPoolSolucoes = 500;
    configReinicializacao_.limiarDiversidade = 0.3;
    configReinicializacao_.maxTentativasSemMelhoria = 2000; // Aumentado para dar mais chances
}

// Função melhorada para estimar lambda inicial com base na análise estatística da instância
double estimarLambdaInicial(const Deposito& deposito, const Backlog& backlog, const LocalizadorItens& localizador) {
    // Coletar estatísticas sobre a instância
    double somaEficiencia = 0.0;
    int contadorPedidos = 0;
    
    // Calcular média de eficiência para uma amostra de pedidos
    int maxPedidosAmostra = std::min(200, backlog.numPedidos); // Aumentado para maior precisão
    std::vector<int> pedidosAmostra(maxPedidosAmostra);
    
    // Selecionar pedidos para amostra (espaçados uniformemente)
    double passo = backlog.numPedidos / static_cast<double>(maxPedidosAmostra);
    for (int i = 0; i < maxPedidosAmostra; i++) {
        int idxPedido = std::min(static_cast<int>(i * passo), backlog.numPedidos - 1);
        pedidosAmostra[i] = idxPedido;
    }
    
    // Adicionar aleatoriedade à amostra para evitar viés
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(pedidosAmostra.begin(), pedidosAmostra.end(), g);
    
    for (int pedidoId : pedidosAmostra) {
        int totalUnidades = 0;
        std::unordered_set<int> corredoresNecessarios;
        
        // Calcular total unidades e corredores necessários
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
            const auto& corredoresComItem = localizador.getCorredoresComItem(itemId);
            for (const auto& [corredorId, _] : corredoresComItem) {
                corredoresNecessarios.insert(corredorId);
            }
        }
        
        // Calcular eficiência se houver corredores
        if (!corredoresNecessarios.empty()) {
            double eficiencia = static_cast<double>(totalUnidades) / corredoresNecessarios.size();
            somaEficiencia += eficiencia;
            contadorPedidos++;
        }
    }
    
    // Se não conseguimos calcular nenhuma eficiência, usar valor padrão baseado nos limites
    if (contadorPedidos == 0) {
        // Usar deposito.numCorredores em vez de backlog.numCorredores
        return (backlog.wave.UB > 0 && deposito.numCorredores > 0) ? 
                             (2.0 * backlog.wave.UB) / (3.0 * deposito.numCorredores) :
                             1.0;
    }
    
    // Retornar média de eficiência como estimativa inicial de lambda
    double mediaEficiencia = somaEficiencia / contadorPedidos;
    
    // Adicionar um fator de ajuste baseado no tamanho da instância
    double fatorAjuste = 1.0;
    if (backlog.numPedidos > 1000) {
        fatorAjuste = 1.2; // Para instâncias maiores, aumentar lambda inicial
    } else if (backlog.numPedidos < 50) {
        fatorAjuste = 0.8; // Para instâncias pequenas, reduzir lambda inicial
    }
    
    return std::max(1.0, mediaEficiencia * fatorAjuste);
}

// Detectar oscilações no valor de lambda
bool OtimizadorDinkelbach::detectarOscilacao(double novoLambda, double limiar) {
    // Verificar se há oscilações em lambda
    if (historicoLambda_.size() < 4) {
        historicoLambda_.push_back(novoLambda);
        return false;
    }
    
    // Adicionar ao histórico de frequência
    infoConvergencia_.frequenciaLambda[novoLambda]++;
    
    // Se um valor de lambda apareceu mais de 3 vezes, há oscilação
    if (infoConvergencia_.frequenciaLambda[novoLambda] > 3) {
        return true;
    }
    
    // Verificar se estamos alternando entre dois valores
    int ultimos = historicoLambda_.size();
    if (ultimos >= 4) {
        double lambda1 = historicoLambda_[ultimos - 4];
        double lambda2 = historicoLambda_[ultimos - 3];
        double lambda3 = historicoLambda_[ultimos - 2];
        double lambda4 = historicoLambda_[ultimos - 1];
        
        // Verificar padrão de oscilação
        if (std::abs(lambda1 - lambda3) < limiar && std::abs(lambda2 - lambda4) < limiar) {
            return true;
        }
        
        // Verificar estagnação
        if (std::abs(lambda1 - lambda2) < limiar && std::abs(lambda2 - lambda3) < limiar && 
            std::abs(lambda3 - lambda4) < limiar) {
            return true;
        }
    }
    
    // Adicionar o novo valor ao histórico e manter apenas os últimos 20 valores
    historicoLambda_.push_back(novoLambda);
    if (historicoLambda_.size() > 20) {
        historicoLambda_.erase(historicoLambda_.begin());
    }
    
    return false;
}

// Detectar ciclos no histórico de lambda
bool OtimizadorDinkelbach::detectarCiclo(const std::vector<double>& lambdas, int janela) {
    int tamanho = lambdas.size();
    if (tamanho < janela * 2) {
        return false;
    }
    
    // Verificar se os últimos 'janela' valores se repetem
    for (int i = 0; i < janela; i++) {
        if (std::abs(lambdas[tamanho - janela - i - 1] - lambdas[tamanho - i - 1]) > epsilon_) {
            return false;
        }
    }
    
    return true;
}

// Quebrar ciclos ou oscilações com uma perturbação forte
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::quebrarCicloOuOscilacao(const SolucaoWave& solucaoAtual, double lambda, int LB, int UB) {
    switch (contadorCiclos_ % 4) {
        case 0:
            // Implementação...
            break;
        case 1:
            // Implementação...
            break;
        case 2:
            // Implementação...
            break;
        case 3:
            // Implementação...
            break;
    }
    
    // Retornar uma solução (implementar conforme necessário)
    return solucaoAtual; // Temporário, implementar adequadamente
}

// Calcular novo valor de lambda com amortecimento para evitar oscilações
double OtimizadorDinkelbach::calcularNovoLambda(const SolucaoWave& solucao, double lambdaAnterior, int iteracao) {
    // Se não temos uma solução válida, manter lambda anterior
    if (solucao.pedidosWave.empty() || solucao.corredoresWave.empty()) {
        return lambdaAnterior;
    }
    
    // Calcular novo lambda: F(x) / G(x)
    double novoLambda = static_cast<double>(solucao.totalUnidades) / solucao.corredoresWave.size();
    
    // Aplicar amortecimento para evitar oscilações
    // Usar taxa de amortecimento decrescente com o número de iterações
    double taxaAmortecimento = std::min(0.9, 0.5 + 0.4 * std::exp(-iteracao / 20.0));
    
    // Se estamos nas primeiras iterações, aplicar amortecimento mais forte
    if (iteracao < 10) {
        taxaAmortecimento = 0.7;
    }
    
    // Se detectamos oscilações recentes, aumentar o amortecimento
    if (contadorCiclos_ > 0 && iteracao - infoConvergencia_.iteracoesRealizadas < 5) {
        taxaAmortecimento = 0.8;
    }
    
    // Calcular lambda amortecido
    double lambdaAmortecido = taxaAmortecimento * lambdaAnterior + (1.0 - taxaAmortecimento) * novoLambda;
    
    // Implementar limites para evitar alterações muito bruscas
    double maxMudanca = 0.2 * lambdaAnterior;
    if (std::abs(lambdaAmortecido - lambdaAnterior) > maxMudanca) {
        if (lambdaAmortecido > lambdaAnterior) {
            lambdaAmortecido = lambdaAnterior + maxMudanca;
        } else {
            lambdaAmortecido = lambdaAnterior - maxMudanca;
        }
    }
    
    return lambdaAmortecido;
}

// Implementação melhorada do algoritmo de Dinkelbach
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::otimizarWave(int LB, int UB) {
    auto inicioExecucao = std::chrono::high_resolution_clock::now();
    
    // Reiniciar estruturas de rastreamento
    historicoLambda_.clear();
    historicoSolucoes_.clear();
    infoConvergencia_ = InfoConvergencia{};
    contadorCiclos_ = 0;
    
    // Usar a função melhorada para estimar lambda inicial
    double lambda = estimarLambdaInicial(deposito_, backlog_, localizador_);
    std::cout << "Lambda inicial estimado: " << lambda << std::endl;
    
    // Construir solução inicial
    SolucaoWave melhorSolucao = construirSolucaoInicial(LB, UB);
    
    // Se não foi possível construir uma solução inicial válida, tentar abordagens alternativas
    if (melhorSolucao.pedidosWave.empty()) {
        // Tentar com diferentes parâmetros
        for (int tentativa = 0; tentativa < 5; tentativa++) {
            double alphaVariado = 0.3 + (tentativa * 0.15); // Variar entre 0.3 e 0.9
            melhorSolucao = construirSolucaoGulosaPonderada(LB, UB, alphaVariado);
            if (!melhorSolucao.pedidosWave.empty()) break;
        }
        
        // Se ainda não temos solução, tentar uma abordagem completamente aleatória
        if (melhorSolucao.pedidosWave.empty()) {
            std::vector<int> todosPedidos;
            for (int i = 0; i < backlog_.numPedidos; i++) {
                todosPedidos.push_back(i);
            }
            
            std::shuffle(todosPedidos.begin(), todosPedidos.end(), geradorPrincipal_);
            
            // Tentar construir uma solução viável adicionando pedidos
            SolucaoWave solucaoAleatoria;
            solucaoAleatoria.totalUnidades = 0;
            
            for (int pedidoId : todosPedidos) {
                // Verificar disponibilidade
                if (!verificador_.verificarDisponibilidade(backlog_.pedido[pedidoId])) {
                    continue;
                }
                
                // Calcular unidades adicionais
                int unidadesAdicionais = 0;
                for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                    unidadesAdicionais += quantidade;
                }
                
                // Verificar se excede UB
                if (solucaoAleatoria.totalUnidades + unidadesAdicionais > UB) {
                    continue;
                }
                
                // Adicionar pedido
                solucaoAleatoria.pedidosWave.push_back(pedidoId);
                solucaoAleatoria.totalUnidades += unidadesAdicionais;
                
                // Atualizar corredores
                std::unordered_set<int> corredoresUnicos;
                for (int pid : solucaoAleatoria.pedidosWave) {
                    for (const auto& [itemId, _] : backlog_.pedido[pid]) {
                        const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
                        for (const auto& [corredorId, _] : corredoresComItem) {
                            corredoresUnicos.insert(corredorId);
                        }
                    }
                }
                
                solucaoAleatoria.corredoresWave.assign(corredoresUnicos.begin(), corredoresUnicos.end());
                
                // Verificar se atingimos o limite inferior
                if (solucaoAleatoria.totalUnidades >= LB) {
                    break;
                }
            }
            
            // Verificar se obtivemos uma solução viável
            if (solucaoAleatoria.totalUnidades >= LB && solucaoAleatoria.totalUnidades <= UB) {
                melhorSolucao = solucaoAleatoria;
            } else {
                // Como último recurso, retornar erro e sugerir revisar os limites da instância
                std::cerr << "ERRO: Não foi possível construir uma solução inicial viável." << std::endl;
                std::cerr << "Verifique se os limites LB=" << LB << " e UB=" << UB << " são consistentes." << std::endl;
                return SolucaoWave{};
            }
        }
    }
    
    // Calcular valor objetivo inicial
    if (!melhorSolucao.corredoresWave.empty()) {
        melhorSolucao.valorObjetivo = static_cast<double>(melhorSolucao.totalUnidades) / 
                                     melhorSolucao.corredoresWave.size();
    } else {
        melhorSolucao.valorObjetivo = 0.0;
    }
    
    std::cout << "Solução inicial: " << melhorSolucao.totalUnidades << " unidades, " 
              << melhorSolucao.corredoresWave.size() << " corredores, "
              << "valor objetivo " << melhorSolucao.valorObjetivo << std::endl;
    
    // Se a solução inicial já tem valor objetivo melhor que lambda, atualizamos lambda
    if (melhorSolucao.valorObjetivo > lambda) {
        lambda = melhorSolucao.valorObjetivo;
    }
    
    // Guardar a solução inicial no histórico
    historicoSolucoes_.push_back(melhorSolucao);
    
    // Loop principal do algoritmo de Dinkelbach
    SolucaoWave solucaoAtual = melhorSolucao;
    int iteracao = 0;
    int iteracoesSemMelhoria = 0;
    bool convergiu = false;
    
    while (iteracao < maxIteracoes_) {
        iteracao++;
        
        // Resolver o subproblema para o valor atual de lambda
        std::pair<SolucaoWave, double> resultado;
        
        if (usarBranchAndBound_ && backlog_.numPedidos <= 200) { // Usar B&B apenas para instâncias pequenas/médias
            resultado = resolverSubproblemaComBranchAndBound(lambda, LB, UB);
        } else {
            resultado = resolverSubproblemaComHeuristica(lambda, LB, UB);
        }
        
        SolucaoWave novaSolucao = resultado.first;
        double valorSubproblema = resultado.second;
        
        // Verificar se o valor do subproblema é muito próximo de zero
        if (std::abs(valorSubproblema) < epsilon_) {
            std::cout << "Convergiu na iteração " << iteracao 
                      << " com lambda = " << lambda 
                      << " e valor objetivo = " << novaSolucao.valorObjetivo << std::endl;
            convergiu = true;
            solucaoAtual = novaSolucao;
            
            // Se esta solução é melhor que a melhor encontrada, atualizar
            if (novaSolucao.valorObjetivo > melhorSolucao.valorObjetivo) {
                melhorSolucao = novaSolucao;
            }
            
            break;
        }
        
        // Verificar se a nova solução melhora a melhor conhecida
        if (novaSolucao.valorObjetivo > melhorSolucao.valorObjetivo + epsilon_) {
            melhorSolucao = novaSolucao;
            iteracoesSemMelhoria = 0;
        } else {
            iteracoesSemMelhoria++;
        }
        
        // Adicionar a nova solução ao histórico
        historicoSolucoes_.push_back(novaSolucao);
        if (historicoSolucoes_.size() > 20) {
            historicoSolucoes_.erase(historicoSolucoes_.begin());
        }
        
        // Calcular novo valor de lambda (com amortecimento)
        double novoLambda = calcularNovoLambda(novaSolucao, lambda, iteracao);
        
        // Verificar se há oscilação ou ciclo
        if (detectarOscilacao(novoLambda, epsilon_)) {
            std::cout << "Detectada oscilação em lambda: " << novoLambda << " vs " << lambda << std::endl;
            
            // Aplicar perturbação para quebrar o ciclo
            SolucaoWave solucaoPerturbada = quebrarCicloOuOscilacao(melhorSolucao, lambda, LB, UB);
            
            // Usar a solução perturbada se for viável
            if (!solucaoPerturbada.pedidosWave.empty() && 
                solucaoPerturbada.totalUnidades >= LB &&
                solucaoPerturbada.totalUnidades <= UB) {
                solucaoAtual = solucaoPerturbada;
                
                // Perturbar também o valor de lambda para sair da região atual
                lambda = lambda * (0.8 + 0.4 * std::generate_canonical<double, 10>(geradorPrincipal_));
                
                // Reiniciar contadores de iterações sem melhoria
                iteracoesSemMelhoria = 0;
                continue;
            }
        }
        
        // Verificar convergência por estagnação
        if (std::abs(novoLambda - lambda) < epsilon_) {
            std::cout << "Convergência por estagnação de lambda na iteração " << iteracao 
                      << " com lambda = " << lambda << std::endl;
            convergiu = true;
            break;
        }
        
        // Verificar se estamos há muitas iterações sem melhoria
        if (iteracoesSemMelhoria > 150) { // Aumento do limite
            // Tentar aplicar busca local avançada para melhorar a melhor solução
            if (usarBuscaLocalAvancada_ && melhorSolucao.pedidosWave.size() >= LB) {
                BuscaLocalAvancada busca(deposito_, backlog_, localizador_, verificador_, limiteTempoBuscaLocal_);
                
                // Converter para o formato esperado pela busca local
                BuscaLocalAvancada::Solucao solBL;
                solBL.pedidosWave = melhorSolucao.pedidosWave;
                solBL.corredoresWave = melhorSolucao.corredoresWave;
                solBL.valorObjetivo = melhorSolucao.valorObjetivo;
                solBL.totalUnidades = melhorSolucao.totalUnidades;
                
                // Executar busca local
                solBL = busca.otimizar(solBL, LB, UB, BuscaLocalAvancada::TipoBuscaLocal::ILS);
                
                // Converter de volta e verificar se melhorou
                if (solBL.valorObjetivo > melhorSolucao.valorObjetivo) {
                    melhorSolucao.pedidosWave = solBL.pedidosWave;
                    melhorSolucao.corredoresWave = solBL.corredoresWave;
                    melhorSolucao.valorObjetivo = solBL.valorObjetivo;
                    melhorSolucao.totalUnidades = solBL.totalUnidades;
                    
                    // Reiniciar contadores
                    iteracoesSemMelhoria = 0;
                    lambda = melhorSolucao.valorObjetivo;
                    continue;
                }
            }
            
            // Se a busca local não ajudou, reiniciar com uma solução diferente
            SolucaoWave novaSolucaoInicial = gerarSolucaoInicialDiversificada(iteracoesSemMelhoria, LB, UB);
            
            if (!novaSolucaoInicial.pedidosWave.empty() && 
                novaSolucaoInicial.totalUnidades >= LB && 
                novaSolucaoInicial.totalUnidades <= UB) {
                solucaoAtual = novaSolucaoInicial;
                
                // Perturbar lambda
                lambda = lambda * (0.9 + 0.2 * std::generate_canonical<double, 10>(geradorPrincipal_));
                
                // Reiniciar contadores
                iteracoesSemMelhoria = 0;
                continue;
            } else {
                // Se não conseguimos encontrar uma nova solução, assumimos convergência
                std::cout << "Convergência por limite de iterações sem melhoria." << std::endl;
                convergiu = true;
                break;
            }
        }
        
        // Atualizar lambda e continuar
        lambda = novoLambda;
        solucaoAtual = novaSolucao;
    }
    
    // Se atingiu o limite de iterações sem convergir
    if (!convergiu) {
        std::cout << "Atingiu o limite máximo de iterações sem convergência." << std::endl;
    }
    
    // Registrar informações de convergência
    infoConvergencia_.iteracoesRealizadas = iteracao;
    infoConvergencia_.convergiu = convergiu;
    
    auto fimExecucao = std::chrono::high_resolution_clock::now();
    infoConvergencia_.tempoTotal = std::chrono::duration<double>(fimExecucao - inicioExecucao).count();
    
    // Sempre retornar a melhor solução encontrada durante o processo
    return melhorSolucao;
}

// Versão com reinicializações múltiplas para escapar de ótimos locais
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::otimizarWaveComReinicializacoes(int LB, int UB) {
    auto inicioExecucao = std::chrono::high_resolution_clock::now();
    
    // Estrutura para armazenar a melhor solução global
    SolucaoWave melhorSolucaoGlobal;
    melhorSolucaoGlobal.valorObjetivo = 0.0;
    
    // Pool de soluções diversas
    std::set<SolucaoWave> poolSolucoes;
    
    // Controle de tentativas sem melhoria
    int tentativasSemMelhoria = 0;
    
    // Realizar múltiplas execuções com diferentes sementes
    for (int i = 0; i < configReinicializacao_.numReinicializacoes; i++) {
        std::cout << "\n=== Execução " << i+1 << " de " << configReinicializacao_.numReinicializacoes << " ===" << std::endl;
        
        // Ajustar parâmetros dinamicamente baseado no progresso
        double ajusteTemporal = ajustarParametrosDinamicos(i, configReinicializacao_.numReinicializacoes);
        
        // Atualizar limite de iterações baseado no progresso
        int maxIterLocal = configReinicializacao_.aumentarIteracoesProgressivamente ?
                           static_cast<int>(maxIteracoes_ * (1.0 + i * 0.5) / configReinicializacao_.numReinicializacoes) :
                           maxIteracoes_;
        maxIterLocal = std::min(maxIterLocal, 20000); // Limitar a um máximo razoável
        
        // Gerar uma solução inicial diversa para esta execução
        SolucaoWave solucaoInicial = gerarSolucaoInicialDiversificada(i, LB, UB);
        
        // Se não conseguimos gerar uma solução inicial válida, tentar mecanismos alternativos
        if (solucaoInicial.pedidosWave.empty() || 
            solucaoInicial.totalUnidades < LB || 
            solucaoInicial.totalUnidades > UB) {
            
            // Tentar recuperar uma solução do pool
            if (!poolSolucoes.empty()) {
                // Selecionar uma solução aleatoriamente
                auto it = poolSolucoes.begin();
                std::advance(it, std::rand() % poolSolucoes.size());
                
                // Perturbar essa solução
                solucaoInicial = perturbarSolucao(*it, 0.5, geradorPrincipal_);
            }
            
            // Se ainda não temos solução, pular esta execução
            if (solucaoInicial.pedidosWave.empty() || 
                solucaoInicial.totalUnidades < LB || 
                solucaoInicial.totalUnidades > UB) {
                continue;
            }
        }
        
        // Executar o algoritmo de Dinkelbach com esta solução inicial
        // Configurando limites internos
        int maxIteracoesAtual = maxIterLocal;
        double epsilonAtual = epsilon_ * (1.0 - 0.2 * (i / static_cast<double>(configReinicializacao_.numReinicializacoes)));
        
        // Estimar lambda inicial para esta execução
        double lambdaInicial = estimarLambdaInicial(deposito_, backlog_, localizador_);
        
        // Executar otimização interna
        SolucaoWave solucaoAtual = otimizarWaveInterno(lambdaInicial, LB, UB, maxIteracoesAtual, i % 3 == 0);
        
        // Verificar se encontramos uma solução melhor
        if (solucaoAtual.valorObjetivo > melhorSolucaoGlobal.valorObjetivo + epsilon_) {
            melhorSolucaoGlobal = solucaoAtual;
            tentativasSemMelhoria = 0;
            
            std::cout << "Nova melhor solução global encontrada na execução " << i+1 
                      << ": " << melhorSolucaoGlobal.valorObjetivo << std::endl;
        } else {
            tentativasSemMelhoria++;
        }
        
        // Adicionar a solução ao pool se for diversa o suficiente
        if (configReinicializacao_.guardarMelhoresSolucoes && 
            solucaoAtual.valorObjetivo > 0.8 * melhorSolucaoGlobal.valorObjetivo) {
            
            // Verificar diversidade
            bool suficientementeDiversa = true;
            for (const auto& solExistente : poolSolucoes) {
                if (calcularDiversidade(solucaoAtual, solExistente) < configReinicializacao_.limiarDiversidade) {
                    suficientementeDiversa = false;
                    break;
                }
            }
            
            if (suficientementeDiversa) {
                poolSolucoes.insert(solucaoAtual);
                
                // Manter o tamanho do pool limitado
                while (poolSolucoes.size() > configReinicializacao_.tamanhoPoolSolucoes) {
                    // Remover a pior solução
                    auto piorIt = std::min_element(poolSolucoes.begin(), poolSolucoes.end(),
                                                 [](const auto& a, const auto& b) {
                                                     return a.valorObjetivo < b.valorObjetivo;
                                                 });
                    poolSolucoes.erase(piorIt);
                }
            }
        }
        
        // Verificar critério de parada antecipada
        if (tentativasSemMelhoria >= configReinicializacao_.maxTentativasSemMelhoria) {
            std::cout << "Parando reinicializações após " << tentativasSemMelhoria 
                      << " tentativas sem melhoria." << std::endl;
            break;
        }
        
        // Verificar tempo limite se necessário
        auto tempoAtual = std::chrono::high_resolution_clock::now();
        double tempoDecorrido = std::chrono::duration<double>(tempoAtual - inicioExecucao).count();
        if (tempoDecorrido > 240.0) { // Limite de 4 minutos
            std::cout << "Parando reinicializações após atingir limite de tempo." << std::endl;
            break;
        }
    }
    
    // Registrar tempo total
    auto fimExecucao = std::chrono::high_resolution_clock::now();
    infoConvergencia_.tempoTotal = std::chrono::duration<double>(fimExecucao - inicioExecucao).count();
    
    return melhorSolucaoGlobal;
}

// Método para gerar soluções iniciais diversificadas
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::gerarSolucaoInicialDiversificada(int indiceReinicializacao, int LB, int UB) {
    switch (indiceReinicializacao % 5) {
        case 0: {
            // Abordagem gulosa padrão
            return construirSolucaoInicial(LB, UB);
        }
        case 1: {
            // Abordagem gulosa com fator de aleatoriedade baixo
            return construirSolucaoGulosaPonderada(LB, UB, 0.2);
        }
        case 2: {
            // Abordagem gulosa com fator de aleatoriedade médio
            return construirSolucaoGulosaPonderada(LB, UB, 0.5);
        }
        case 3: {
            // Abordagem gulosa com fator de aleatoriedade alto
            return construirSolucaoGulosaPonderada(LB, UB, 0.8);
        }
        case 4: default: {
            // Abordagem completamente aleatória
            std::vector<int> todosPedidos;
            for (int i = 0; i < backlog_.numPedidos; i++) {
                if (verificador_.verificarDisponibilidade(backlog_.pedido[i])) {
                    todosPedidos.push_back(i);
                }
            }
            
            std::shuffle(todosPedidos.begin(), todosPedidos.end(), geradorPrincipal_);
            
            SolucaoWave solucao;
            solucao.totalUnidades = 0;
            std::unordered_set<int> corredoresIncluidos;
            
            for (int pedidoId : todosPedidos) {
                // Calcular unidades adicionais
                int unidadesAdicionais = 0;
                for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                    unidadesAdicionais += quantidade;
                }
                
                // Verificar se excede UB
                if (solucao.totalUnidades + unidadesAdicionais > UB) {
                    continue;
                }
                
                // Adicionar pedido
                solucao.pedidosWave.push_back(pedidoId);
                solucao.totalUnidades += unidadesAdicionais;
                
                // Atualizar corredores
                for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                    const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
                    for (const auto& [corredorId, _] : corredoresComItem) {
                        corredoresIncluidos.insert(corredorId);
                    }
                }
                
                // Verificar se atingimos o limite inferior
                if (solucao.totalUnidades >= LB) {
                    break;
                }
            }
            
            // Converter para vetor e calcular valor objetivo
            solucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
            
            if (!solucao.corredoresWave.empty()) {
                solucao.valorObjetivo = static_cast<double>(solucao.totalUnidades) / solucao.corredoresWave.size();
            } else {
                solucao.valorObjetivo = 0.0;
            }
            
            return solucao;
        }
    }
}

// Construir uma solução gulosa com fator de aleatoriedade
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::construirSolucaoGulosaPonderada(int LB, int UB, double alpha) {
    // Inicializar solução
    SolucaoWave solucao;
    solucao.totalUnidades = 0;
    std::unordered_set<int> corredoresIncluidos;
    
    // Construir lista de candidatos
    std::vector<std::pair<int, double>> candidatos;
    for (int i = 0; i < backlog_.numPedidos; i++) {
        // Verificar disponibilidade
        if (!verificador_.verificarDisponibilidade(backlog_.pedido[i])) {
            continue;
        }
        
        // Calcular eficiência: unidades / corredores adicionais
        int unidades = 0;
        std::unordered_set<int> corredoresNecessarios;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[i]) {
            unidades += quantidade;
            const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
            for (const auto& [corredorId, _] : corredoresComItem) {
                corredoresNecessarios.insert(corredorId);
            }
        }
        
        // Calcular quantos corredores novos seriam adicionados
        int corredoresNovos = 0;
        for (int corredorId : corredoresNecessarios) {
            if (corredoresIncluidos.find(corredorId) == corredoresIncluidos.end()) {
                corredoresNovos++;
            }
        }
        
        // Evitar divisão por zero
        double eficiencia = corredoresNovos > 0 ? 
                          static_cast<double>(unidades) / corredoresNovos : 
                          static_cast<double>(unidades);
        
        candidatos.push_back({i, eficiencia});
    }
    
    // Verificar se há candidatos
    if (candidatos.empty()) {
        return solucao; // Solução vazia
    }
    
    // Ordenar candidatos por eficiência
    std::sort(candidatos.begin(), candidatos.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Adicionar pedidos à solução
    while (!candidatos.empty()) {
        // Selecionar um candidato com base no fator de aleatoriedade
        int indice;
        if (alpha > 0.0) {
            // Calcular tamanho da lista restrita de candidatos (RCL)
            int tamRCL = std::max(1, static_cast<int>(alpha * candidatos.size()));
            
            // Selecionar aleatoriamente da RCL
            indice = std::uniform_int_distribution<>(0, tamRCL - 1)(geradorPrincipal_);
        } else {
            // Sempre selecionar o melhor candidato
            indice = 0;
        }
        
        int pedidoId = candidatos[indice].first;
        
        // Remover o candidato selecionado
        candidatos.erase(candidatos.begin() + indice);
        
        // Calcular unidades adicionais
        int unidadesAdicionais = 0;
        std::unordered_set<int> novosCorredores;
        
        for (const auto& [itemId, quantidade] : backlog_.pedido[pedidoId]) {
            unidadesAdicionais += quantidade;
            const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
            for (const auto& [corredorId, _] : corredoresComItem) {
                novosCorredores.insert(corredorId);
            }
        }
        
        // Verificar se excede UB
        if (solucao.totalUnidades + unidadesAdicionais > UB) {
            continue;
        }
        
        // Adicionar pedido
        solucao.pedidosWave.push_back(pedidoId);
        solucao.totalUnidades += unidadesAdicionais;
        
        // Atualizar corredores
        for (int corredorId : novosCorredores) {
            corredoresIncluidos.insert(corredorId);
        }
        
        // Verificar se atingimos o limite inferior
        if (solucao.totalUnidades >= LB) {
            break;
        }
        
        // Atualizar eficiências dos candidatos restantes
        for (auto& [_, eficiencia] : candidatos) {
            // Adicionar ruído aleatório para diversificar
            eficiencia *= (0.95 + 0.1 * std::generate_canonical<double, 10>(geradorPrincipal_));
        }
        
        // Reordenar candidatos
        std::sort(candidatos.begin(), candidatos.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
    }
    
    // Verificar se atingimos o limite inferior
    if (solucao.totalUnidades < LB) {
        // Tentar com abordagem aleatória como último recurso
        std::vector<int> todosPedidos;
        for (int i = 0; i < backlog_.numPedidos; i++) {
            if (verificador_.verificarDisponibilidade(backlog_.pedido[i]) &&
                std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), i) == solucao.pedidosWave.end()) {
                todosPedidos.push_back(i);
            }
        }
        
        std::shuffle(todosPedidos.begin(), todosPedidos.end(), geradorPrincipal_);
        
        for (int pedidoId : todosPedidos) {
            // Calcular unidades adicionais
            int unidadesAdicionais = 0;
            for (const auto& [_, quantidade] : backlog_.pedido[pedidoId]) {
                unidadesAdicionais += quantidade;
            }
            
            // Verificar se excede UB
            if (solucao.totalUnidades + unidadesAdicionais > UB) {
                continue;
            }
            
            // Adicionar pedido
            solucao.pedidosWave.push_back(pedidoId);
            solucao.totalUnidades += unidadesAdicionais;
            
            // Atualizar corredores
            for (const auto& [itemId, _] : backlog_.pedido[pedidoId]) {
                const auto& corredoresComItem = localizador_.getCorredoresComItem(itemId);
                for (const auto& [corredorId, _] : corredoresComItem) {
                    corredoresIncluidos.insert(corredorId);
                }
            }
            
            // Verificar se atingimos o limite inferior
            if (solucao.totalUnidades >= LB) {
                break;
            }
        }
    }
    
    // Converter corredores para vetor
    solucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
    
    // Calcular valor objetivo
    if (!solucao.corredoresWave.empty()) {
        solucao.valorObjetivo = static_cast<double>(solucao.totalUnidades) / solucao.corredoresWave.size();
    } else {
        solucao.valorObjetivo = 0.0;
    }
    
    return solucao;
}

// Ajustar parâmetros dinâmicos durante as reinicializações
double OtimizadorDinkelbach::ajustarParametrosDinamicos(int indiceReinicializacao, int totalReinicializacoes) {
    // Calcular progresso (entre 0.0 e 1.0)
    double progresso = static_cast<double>(indiceReinicializacao) / totalReinicializacoes;
    
    // Ajustar epsilon - tornar mais restritivo conforme avança
    epsilon_ = 0.0001 * (1.0 + progresso);
    
    // Ajustar limites de iterações - aumentar conforme avança
    maxIteracoes_ = 2000 + static_cast<int>(18000 * progresso);
    
    // Ajustar uso de estratégias - dar mais ênfase à busca local nas fases finais
    if (progresso > 0.7) {
        usarBuscaLocalAvancada_ = true;
        limiteTempoBuscaLocal_ = 3.0 + progresso * 7.0; // Entre 3 e 10 segundos
    }
    
    // Retornar um fator de ajuste que pode ser usado para outras configurações
    return 1.0 + progresso;
}

// Calcular diversidade entre duas soluções
double OtimizadorDinkelbach::calcularDiversidade(const SolucaoWave& s1, const SolucaoWave& s2) {
    if (s1.pedidosWave.empty() || s2.pedidosWave.empty()) {
        return 1.0; // Máxima diversidade se uma das soluções for vazia
    }
    
    // Contar elementos comuns
    std::unordered_set<int> pedidosS1(s1.pedidosWave.begin(), s1.pedidosWave.end());
    int comuns = 0;
    
    for (int pedidoId : s2.pedidosWave) {
        if (pedidosS1.find(pedidoId) != pedidosS1.end()) {
            comuns++;
        }
    }
    
    // Calcular índice de Jaccard
    int total = s1.pedidosWave.size() + s2.pedidosWave.size() - comuns;
    double similaridade = total > 0 ? static_cast<double>(comuns) / total : 0.0;
    
    // Diversidade = 1 - similaridade
    return 1.0 - similaridade;
}

// Versão interna de otimização para uso com múltiplas reinicializações
OtimizadorDinkelbach::SolucaoWave OtimizadorDinkelbach::otimizarWaveInterno(double lambdaInicial, int LB, int UB, int maxIter, bool logProgress) {
    // Similar ao método otimizarWave, mas com menos logs e configurações específicas
    double lambda = lambdaInicial;
    
    // Construir solução inicial padrão
    SolucaoWave melhorSolucao = construirSolucaoInicial(LB, UB);
    
    // Se não foi possível construir uma solução inicial, tentar alternativa
    if (melhorSolucao.pedidosWave.empty() || melhorSolucao.totalUnidades < LB) {
        melhorSolucao = construirSolucaoGulosaPonderada(LB, UB, 0.5);
    }
    
    // Se ainda não temos solução, retornar vazio
    if (melhorSolucao.pedidosWave.empty() || melhorSolucao.totalUnidades < LB) {
        return SolucaoWave{};
    }
    
    // Calcular valor objetivo inicial
    if (!melhorSolucao.corredoresWave.empty()) {
        melhorSolucao.valorObjetivo = static_cast<double>(melhorSolucao.totalUnidades) / 
                                     melhorSolucao.corredoresWave.size();
    } else {
        melhorSolucao.valorObjetivo = 0.0;
    }
    
    // Loop principal do algoritmo de Dinkelbach
    SolucaoWave solucaoAtual = melhorSolucao;
    int iteracao = 0;
    int iteracoesSemMelhoria = 0;
    
    while (iteracao < maxIter) {
        iteracao++;
        
        // Resolver o subproblema para o valor atual de lambda
        std::pair<SolucaoWave, double> resultado;
        
        // Usar heurística mais rápida para internas
        resultado = resolverSubproblemaComHeuristica(lambda, LB, UB);
        
        SolucaoWave novaSolucao = resultado.first;
        double valorSubproblema = resultado.second;
        
        // Verificar se o valor do subproblema é muito próximo de zero
        if (std::abs(valorSubproblema) < epsilon_) {
            if (logProgress) {
                std::cout << "  Convergiu na iteração " << iteracao 
                          << " com lambda = " << lambda 
                          << " e valor objetivo = " << novaSolucao.valorObjetivo << std::endl;
            }
            
            // Se esta solução é melhor que a melhor encontrada, atualizar
            if (novaSolucao.valorObjetivo > melhorSolucao.valorObjetivo) {
                melhorSolucao = novaSolucao;
            }
            
            break;
        }
        
        // Verificar se a nova solução melhora a melhor conhecida
        if (novaSolucao.valorObjetivo > melhorSolucao.valorObjetivo + epsilon_) {
            melhorSolucao = novaSolucao;
            iteracoesSemMelhoria = 0;
        } else {
            iteracoesSemMelhoria++;
        }
        
        // Verificar se há oscilação ou ciclo
        double novoLambda = calcularNovoLambda(novaSolucao, lambda, iteracao);
        
        if (detectarOscilacao(novoLambda, epsilon_)) {
            if (logProgress) {
                std::cout << "  Detectada oscilação em lambda: " << novoLambda << " vs " << lambda << std::endl;
            }
            
            // Aplicar perturbação simplificada
            std::mt19937 geradorLocal(std::random_device{}());
            SolucaoWave solucaoPerturbada = perturbarSolucao(melhorSolucao, 0.4, geradorLocal);
            
            // Usar a solução perturbada se for viável
            if (!solucaoPerturbada.pedidosWave.empty() && 
                solucaoPerturbada.totalUnidades >= LB &&
                solucaoPerturbada.totalUnidades <= UB) {
                solucaoAtual = solucaoPerturbada;
                
                // Perturbar também o valor de lambda
                lambda = lambda * (0.8 + 0.4 * std::generate_canonical<double, 10>(geradorPrincipal_));
                
                // Reiniciar contadores
                iteracoesSemMelhoria = 0;
                continue;
            }
        }
        
        // Verificar convergência por estagnação
        if (std::abs(novoLambda - lambda) < epsilon_) {
            break;
        }
        
        // Verificar se estamos há muitas iterações sem melhoria
        if (iteracoesSemMelhoria > 100) {
            // Parar esta execução interna
            break;
        }
        
        // Atualizar lambda e continuar
        lambda = novoLambda;
        solucaoAtual = novaSolucao;
    }
    
    return melhorSolucao;
}

// Implementar um método principal unificado para escolher automat