#include "solucionar_desafio.h"
#include "parser.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "analisador_relevancia.h"
#include "gestor_waves.h" 
#include "seletor_waves.h"
#include "otimizador_wave.h"
#include "otimizador_dinkelbach.h"
#include "busca_local_avancada.h"
#include "formatacao_terminal.h"
#include "otimizador_paralelo.h"
#include "branch_and_bound_solver.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <thread>
#include <mutex>
#include <vector>
#include <limits>
#include <iomanip> // Para formatação do tempo
#include <thread>
#include <sstream> // Para capturar saída em buffer
#include <iomanip> // Para formatação do tempo
#include <map>
#include <string>

using namespace FormatacaoTerminal;

// Estrutura para armazenar os tempos de execução
struct TemposExecucao {
    std::chrono::time_point<std::chrono::high_resolution_clock> inicioGeral;
    std::chrono::duration<double> tempoTotalExecucao;
    std::unordered_map<std::string, double> temposPorInstancia; // Nome do arquivo -> tempo em segundos
};

// Variável global para armazenar os tempos
TemposExecucao temposExecucao;

// Função para formatar o tempo em segundos
std::string formatarTempo(double segundos) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << segundos << " s";
    return oss.str();
}

// Função para salvar os tempos em um arquivo
void salvarTemposExecucao() {
    std::ofstream outFile("data/tempos_execucao.csv");
    if (outFile.is_open()) {
        outFile << "instancia,tempo_segundos\n";
        for (const auto& [instancia, tempo] : temposExecucao.temposPorInstancia) {
            outFile << instancia << "," << tempo << "\n";
        }
        outFile << "TOTAL," << temposExecucao.tempoTotalExecucao.count() << "\n";
        outFile.close();
    }
}

// Função para salvar uma solução em arquivo
void salvarSolucao(const std::vector<int>& pedidosWave, 
                  const std::vector<int>& corredoresWave, 
                  const std::string& arquivoSaida) {
    std::ofstream file(arquivoSaida);
    if (file.is_open()) {
        // Escrever o número de pedidos
        file << pedidosWave.size() << std::endl;
        
        // Escrever os IDs dos pedidos (todos na mesma linha)
        for (size_t i = 0; i < pedidosWave.size(); ++i) {
            file << pedidosWave[i];
            if (i < pedidosWave.size() - 1) {
                file << " ";
            }
        }
        file << std::endl;
        
        // Escrever o número de corredores
        file << corredoresWave.size() << std::endl;
        
        // Escrever os IDs dos corredores (todos na mesma linha)
        for (size_t i = 0; i < corredoresWave.size(); ++i) {
            file << corredoresWave[i];
            if (i < corredoresWave.size() - 1) {
                file << " ";
            }
        }
        file << std::endl;
        
        file.close();
        std::cout << "Solução salva em: " << arquivoSaida << std::endl;
    } else {
        std::cerr << "Erro ao abrir arquivo para salvar solução: " << arquivoSaida << std::endl;
    }
}

// Função auxiliar para gerar um número aleatório dentro de um intervalo
int gerarNumeroAleatorio(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::mutex mutex;
    
    // Verificar e corrigir caso o intervalo seja inválido
    if (min > max) {
        std::swap(min, max);
    }
    
    std::uniform_int_distribution<> distrib(min, max);
    
    // Proteger o acesso ao gerador com mutex
    std::lock_guard<std::mutex> lock(mutex);
    return distrib(gen);
}

// Função para verificar se uma solução é viável
// Função para gerar uma solução gulosa básica que satisfaça as restrições LB e UB
Solucao gerarSolucaoGulosaBasica(const Deposito& deposito, const Backlog& backlog, int LB, int UB) {
    Solucao solucao;
    std::vector<std::pair<int, double>> pedidosCandidatos;
    
    // Classificar todos os pedidos por eficiência (unidades por corredor)
    for (int i = 0; i < backlog.numPedidos; i++) {
        int unidadesPedido = 0;
        std::unordered_set<int> corredores;
        
        for (const auto& [itemId, qtd] : backlog.pedido[i]) {
            unidadesPedido += qtd;
            
            // Identificar corredores para este item
            for (int c = 0; c < deposito.numCorredores; c++) {
                if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                    corredores.insert(c);
                }
            }
        }
        
        // Eficiência = unidades / número de corredores
        double eficiencia = corredores.empty() ? 0 : static_cast<double>(unidadesPedido) / corredores.size();
        
        pedidosCandidatos.push_back({i, eficiencia});
    }
    
    // Ordenar pedidos por eficiência (maior primeiro)
    std::sort(pedidosCandidatos.begin(), pedidosCandidatos.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Adicionar pedidos até atingir LB
    std::unordered_set<int> corredoresIncluidos;
    int totalUnidades = 0;
    
    for (const auto& [pedidoId, _] : pedidosCandidatos) {
        // Se já ultrapassamos UB, parar
        if (totalUnidades >= UB) break;
        
        // Calcular unidades que este pedido adicionará
        int unidadesPedido = 0;
        for (const auto& [_, qtd] : backlog.pedido[pedidoId]) {
            unidadesPedido += qtd;
        }
        
        // Se adicionar este pedido ultrapassar UB e já tivermos atingido LB, pular
        if (totalUnidades >= LB && totalUnidades + unidadesPedido > UB) 
            continue;
        
        // Adicionar pedido
        solucao.pedidosWave.push_back(pedidoId);
        totalUnidades += unidadesPedido;
        
        // Adicionar corredores necessários
        for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
            for (int c = 0; c < deposito.numCorredores; c++) {
                if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                    corredoresIncluidos.insert(c);
                }
            }
        }
        
        // Se já atingimos LB e temos uma solução razoável, podemos parar
        if (totalUnidades >= LB * 1.2) break;
    }
    
    // Verificar se atingimos LB
    if (totalUnidades < LB) {
        // Se não, adicionar mais pedidos mesmo que ultrapassem UB
        for (const auto& [pedidoId, _] : pedidosCandidatos) {
            if (std::find(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), pedidoId) 
                != solucao.pedidosWave.end()) 
                continue; // Pular se já incluído
            
            int unidadesPedido = 0;
            for (const auto& [_, qtd] : backlog.pedido[pedidoId]) {
                unidadesPedido += qtd;
            }
            
            // Adicionar pedido
            solucao.pedidosWave.push_back(pedidoId);
            totalUnidades += unidadesPedido;
            
            // Adicionar corredores necessários
            for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
                for (int c = 0; c < deposito.numCorredores; c++) {
                    if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                        corredoresIncluidos.insert(c);
                    }
                }
            }
            
            if (totalUnidades >= LB) break;
        }
    }
    
    // Definir corredores e valor objetivo
    solucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
    solucao.valorObjetivo = totalUnidades - solucao.corredoresWave.size();
    
    return solucao;
}

bool verificarSolucaoViavel(
    const Deposito& deposito, 
    const Backlog& backlog,
    const std::vector<int>& pedidosWave, 
    const std::vector<int>& corredoresWave, 
    std::ostream& output
) {
    // Verificar limite inferior de unidades
    int totalUnidades = 0;
    for (int pedidoId : pedidosWave) {
        for (const auto& [_, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    if (totalUnidades < backlog.wave.LB) {
        output << colorir("ERRO: Solução inviável - ", VERMELHO) 
               << "Total de unidades (" << totalUnidades 
               << ") abaixo do limite inferior (" << backlog.wave.LB << ")\n";
        return false;
    }
    
    if (totalUnidades > backlog.wave.UB) {
        output << colorir("ERRO: Solução inviável - ", VERMELHO) 
               << "Total de unidades (" << totalUnidades 
               << ") acima do limite superior (" << backlog.wave.UB << ")\n";
        return false;
    }
    
    // Verificar disponibilidade de estoque
    std::map<int, int> estoqueUsado;
    for (int pedidoId : pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            estoqueUsado[itemId] += quantidade;
        }
    }
    
    for (const auto& [itemId, qtdTotal] : estoqueUsado) {
        int estoqueDisponivel = 0;
        for (int corredorId = 0; corredorId < deposito.numCorredores; corredorId++) {
            auto it = deposito.corredor[corredorId].find(itemId);
            if (it != deposito.corredor[corredorId].end()) {
                estoqueDisponivel += it->second;
            }
        }
        
        if (estoqueDisponivel < qtdTotal) {
            output << colorir("ERRO: Solução inviável - ", VERMELHO) 
                   << "Estoque insuficiente para item " << itemId 
                   << " (requer " << qtdTotal << ", disponível " << estoqueDisponivel << ")\n";
            return false;
        }
    }
    
    return true;
}

bool repararSolucaoInviavel(const Deposito& deposito, const Backlog& backlog, 
                           Solucao& solucao, int LB, int UB, std::ostream& output) {
    // Calcular unidades atuais e corredores
    int unidadesAtuais = 0;
    std::unordered_set<int> corredoresSet;
    
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, qtd] : backlog.pedido[pedidoId]) {
            unidadesAtuais += qtd;
            
            for (int c = 0; c < deposito.numCorredores; c++) {
                if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                    corredoresSet.insert(c);
                }
            }
        }
    }
    
    // Verificar violações de estoque
    std::map<int, int> demandaTotalPorItem;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, qtd] : backlog.pedido[pedidoId]) {
            demandaTotalPorItem[itemId] += qtd;
        }
    }
    
    std::map<int, int> estoqueDisponivel;
    for (int c = 0; c < deposito.numCorredores; c++) {
        for (const auto& [itemId, qtd] : deposito.corredor[c]) {
            estoqueDisponivel[itemId] += qtd;
        }
    }
    
    // Identificar itens com estoque insuficiente
    std::vector<int> pedidosProblematicos;
    for (const auto& [itemId, qtdNecessaria] : demandaTotalPorItem) {
        if (estoqueDisponivel[itemId] < qtdNecessaria) {
            // Encontrar pedidos que usam este item
            for (int i = 0; i < solucao.pedidosWave.size(); i++) {
                int pedidoId = solucao.pedidosWave[i];
                for (const auto& [pItemId, _] : backlog.pedido[pedidoId]) {
                    if (pItemId == itemId) {
                        pedidosProblematicos.push_back(i);
                        break;
                    }
                }
            }
        }
    }
    
    // Remover pedidos problemáticos (do fim para o início para não invalidar índices)
    std::sort(pedidosProblematicos.begin(), pedidosProblematicos.end(), std::greater<int>());
    for (int idx : pedidosProblematicos) {
        if (idx >= 0 && idx < solucao.pedidosWave.size()) {
            output << "Removendo pedido " << solucao.pedidosWave[idx] 
                   << " devido a limitações de estoque." << std::endl;
            solucao.pedidosWave.erase(solucao.pedidosWave.begin() + idx);
        }
    }
    
    // Recalcular unidades e corredores após remoções
    unidadesAtuais = 0;
    corredoresSet.clear();
    
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [itemId, qtd] : backlog.pedido[pedidoId]) {
            unidadesAtuais += qtd;
            
            for (int c = 0; c < deposito.numCorredores; c++) {
                if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                    corredoresSet.insert(c);
                }
            }
        }
    }
    
    // Caso 1: Muito poucas unidades (abaixo de LB)
    if (unidadesAtuais < LB) {
        output << "Reparo: adicionando pedidos para atingir LB (" << LB << ")" << std::endl;
        
        // Identificar pedidos não incluídos e ordenar por eficiência
        std::vector<std::pair<int, double>> pedidosCandidatos;
        std::unordered_set<int> pedidosIncluidos(solucao.pedidosWave.begin(), solucao.pedidosWave.end());
        
        for (int i = 0; i < backlog.numPedidos; i++) {
            if (pedidosIncluidos.find(i) != pedidosIncluidos.end()) {
                continue; // Pular pedidos já incluídos
            }
            
            int unidadesPedido = 0;
            std::unordered_set<int> corredoresAdicionais;
            bool estoqueValido = true;
            
            // Verificar estoque suficiente
            for (const auto& [itemId, qtd] : backlog.pedido[i]) {
                unidadesPedido += qtd;
                
                // Verificar disponibilidade considerando o que já foi alocado
                int demandaAtual = demandaTotalPorItem[itemId];
                if (demandaAtual + qtd > estoqueDisponivel[itemId]) {
                    estoqueValido = false;
                    break;
                }
                
                // Mapear corredores adicionais
                for (int c = 0; c < deposito.numCorredores; c++) {
                    if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end() &&
                        corredoresSet.find(c) == corredoresSet.end()) {
                        corredoresAdicionais.insert(c);
                    }
                }
            }
            
            if (!estoqueValido) continue; // Pular se não tem estoque suficiente
            
            // Calcular eficiência (unidades por corredor adicional)
            double eficiencia = corredoresAdicionais.empty() ? 
                unidadesPedido * 10.0 : static_cast<double>(unidadesPedido) / corredoresAdicionais.size();
            
            pedidosCandidatos.push_back({i, eficiencia});
        }
        
        // Ordenar por eficiência (maior primeiro)
        std::sort(pedidosCandidatos.begin(), pedidosCandidatos.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Adicionar pedidos até atingir LB
        for (const auto& [pedidoId, _] : pedidosCandidatos) {
            if (unidadesAtuais >= LB) break;
            
            // Verificar se adicionar este pedido não excede UB
            int unidadesPedido = 0;
            for (const auto& [_, qtd] : backlog.pedido[pedidoId]) {
                unidadesPedido += qtd;
            }
            
            if (unidadesAtuais + unidadesPedido > UB) continue;
            
            // Adicionar pedido
            solucao.pedidosWave.push_back(pedidoId);
            output << "Adicionando pedido " << pedidoId << " com " 
                   << unidadesPedido << " unidades" << std::endl;
            
            // Atualizar unidades e corredores
            unidadesAtuais += unidadesPedido;
            
            for (const auto& [itemId, qtd] : backlog.pedido[pedidoId]) {
                // Atualizar demanda para verificação de estoque
                demandaTotalPorItem[itemId] += qtd;
                
                for (int c = 0; c < deposito.numCorredores; c++) {
                    if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                        corredoresSet.insert(c);
                    }
                }
            }
        }
    }
    // Caso 2: Muitas unidades (acima de UB)
    else if (unidadesAtuais > UB) {
        output << "Reparo: removendo pedidos para respeitar UB (" << UB << ")" << std::endl;
        
        // Implementação para remover pedidos eficientemente até ficar abaixo de UB
        std::vector<std::pair<double, int>> pedidosEficiencia;
        
        // Calcular eficiência de cada pedido (unidades por corredor utilizado)
        for (int i = 0; i < solucao.pedidosWave.size(); i++) {
            int pedidoId = solucao.pedidosWave[i];
            int unidadesPedido = 0;
            std::unordered_set<int> corredoresExclusivos;
            
            // Contar unidades deste pedido
            for (const auto& [itemId, qtd] : backlog.pedido[pedidoId]) {
                unidadesPedido += qtd;
                
                // Verificar corredores que este pedido utiliza
                for (int c = 0; c < deposito.numCorredores; c++) {
                    if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                        corredoresExclusivos.insert(c);
                    }
                }
            }
            
            // Eficiência = unidades / corredores (quanto maior, melhor)
            double eficiencia = corredoresExclusivos.empty() ? 
                unidadesPedido : static_cast<double>(unidadesPedido) / corredoresExclusivos.size();
            
            pedidosEficiencia.push_back({eficiencia, i});
        }
        
        // Ordenar por eficiência (menor primeiro, para remover)
        std::sort(pedidosEficiencia.begin(), pedidosEficiencia.end());
        
        // Remover pedidos até ficar abaixo do UB
        for (const auto& [_, idx] : pedidosEficiencia) {
            if (unidadesAtuais <= UB) break;
            
            int pedidoId = solucao.pedidosWave[idx];
            int unidadesPedido = 0;
            
            for (const auto& [_, qtd] : backlog.pedido[pedidoId]) {
                unidadesPedido += qtd;
            }
            
            output << "Removendo pedido " << pedidoId << " com " 
                   << unidadesPedido << " unidades" << std::endl;
            
            // Remover este pedido (marcando como -1 para remoção posterior)
            solucao.pedidosWave[idx] = -1;
            unidadesAtuais -= unidadesPedido;
        }
        
        // Remover os pedidos marcados com -1
        solucao.pedidosWave.erase(
            std::remove(solucao.pedidosWave.begin(), solucao.pedidosWave.end(), -1),
            solucao.pedidosWave.end()
        );
    }
    
    // Atualizar corredores da solução
    solucao.corredoresWave.assign(corredoresSet.begin(), corredoresSet.end());
    
    // Recalcular valor objetivo
    int novoTotalUnidades = 0;
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [_, qtd] : backlog.pedido[pedidoId]) {
            novoTotalUnidades += qtd;
        }
    }
    
    solucao.valorObjetivo = novoTotalUnidades - solucao.corredoresWave.size();
    
    output << "Após reparo: " << solucao.pedidosWave.size() << " pedidos, " 
           << novoTotalUnidades << " unidades, " 
           << solucao.corredoresWave.size() << " corredores" << std::endl;
    
    // Verificar se a solução é viável agora
    return (novoTotalUnidades >= LB && novoTotalUnidades <= UB);
}

Solucao otimizarSolucao(const Deposito& deposito, const Backlog& backlog, const Solucao& solucaoInicial,
                       const LocalizadorItens& localizador,
                       const VerificadorDisponibilidade& verificador,
                       const AnalisadorRelevancia& analisador) {
    // Determinar o número ideal de threads
    unsigned int threadsOtimas = std::thread::hardware_concurrency();
    if (threadsOtimas == 0) threadsOtimas = 4; // Fallback
    
    // Limitar para instâncias pequenas
    if (backlog.numPedidos < 100) {
        threadsOtimas = std::min(threadsOtimas, 2u);
    }
    
    // Criar o otimizador paralelo
    OtimizadorParalelo otimizador(deposito, backlog, localizador, verificador, analisador, threadsOtimas);
    
    // Configurar tempo máximo
    otimizador.setTempoMaximo(60.0); // 60 segundos
    
    // Otimizar
    Solucao solucaoOtimizada = otimizador.otimizar(solucaoInicial);
    
    return solucaoOtimizada;
}

bool verificarLimites(const Solucao& solucao, const Backlog& backlog, int& totalUnidades) {
    totalUnidades = 0;
    
    for (int pedidoId : solucao.pedidosWave) {
        for (const auto& [_, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
        }
    }
    
    return (totalUnidades >= backlog.wave.LB && totalUnidades <= backlog.wave.UB);
}

Solucao ajustarParaLimites(
    const Solucao& solucao, 
    const Deposito& deposito,
    const Backlog& backlog,
    const LocalizadorItens& localizador, 
    const VerificadorDisponibilidade& verificador
) {
    Solucao ajustada = solucao;
    int totalUnidades = 0;
    
    if (verificarLimites(ajustada, backlog, totalUnidades)) {
        return ajustada; // Já está nos limites
    }
    
    // Caso 1: Muito poucas unidades (abaixo de LB)
    if (totalUnidades < backlog.wave.LB) {
        std::vector<std::pair<double, int>> candidatos;
        
        // Identificar pedidos não incluídos e ordenar por eficiência
        for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
            if (std::find(ajustada.pedidosWave.begin(), ajustada.pedidosWave.end(), pedidoId) 
                != ajustada.pedidosWave.end() || 
                !verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
                continue;
            }
            
            int unidades = 0;
            std::unordered_set<int> corredoresNovos;
            
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                unidades += quantidade;
                
                for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                    if (std::find(ajustada.corredoresWave.begin(), 
                                ajustada.corredoresWave.end(), 
                                corredorId) == ajustada.corredoresWave.end()) {
                        corredoresNovos.insert(corredorId);
                    }
                }
            }
            
            double eficiencia = corredoresNovos.empty() ? unidades : 
                              static_cast<double>(unidades) / corredoresNovos.size();
            candidatos.push_back({eficiencia, pedidoId});
        }
        
        // Ordenar candidatos por eficiência (maior primeiro)
        std::sort(candidatos.begin(), candidatos.end(),
                 [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Adicionar até atingir LB
        for (const auto& [_, pedidoId] : candidatos) {
            if (totalUnidades >= backlog.wave.LB) break;
            
            int unidadesPedido = 0;
            for (const auto& [_, quantidade] : backlog.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }
            
            ajustada.pedidosWave.push_back(pedidoId);
            totalUnidades += unidadesPedido;
        }
    }
    // Caso 2: Muitas unidades (acima de UB)
    else if (totalUnidades > backlog.wave.UB) {
        // Ordenar pedidos por eficiência (menor primeiro)
        std::vector<std::pair<double, int>> pedidosAtuais;
        for (int pedidoId : ajustada.pedidosWave) {
            int unidades = 0;
            for (const auto& [_, qtd] : backlog.pedido[pedidoId]) {
                unidades += qtd;
            }
            
            double eficiencia = unidades; // Simplificado, podemos melhorar
            pedidosAtuais.push_back({eficiencia, pedidoId});
        }
        
        // Ordenar por eficiência (menor primeiro)
        std::sort(pedidosAtuais.begin(), pedidosAtuais.end());
        
        // Remover até ficar abaixo de UB
        std::vector<int> novaPedidosWave;
        int novoTotalUnidades = 0;
        
        for (const auto& [_, pedidoId] : pedidosAtuais) {
            int unidadesPedido = 0;
            for (const auto& [_, quantidade] : backlog.pedido[pedidoId]) {
                unidadesPedido += quantidade;
            }
            
            if (novoTotalUnidades + unidadesPedido <= backlog.wave.UB) {
                novaPedidosWave.push_back(pedidoId);
                novoTotalUnidades += unidadesPedido;
            }
        }
        
        ajustada.pedidosWave = novaPedidosWave;
        totalUnidades = novoTotalUnidades;
    }
    
    // Atualizar corredores da solução ajustada
    std::unordered_set<int> corredoresSet;
    for (int pedidoId : ajustada.pedidosWave) {
        for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
            for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                corredoresSet.insert(corredorId);
            }
        }
    }
    
    ajustada.corredoresWave.assign(corredoresSet.begin(), corredoresSet.end());
    
    // Calcular valor objetivo (BOV)
    ajustada.valorObjetivo = totalUnidades / static_cast<double>(ajustada.corredoresWave.size());
    
    return ajustada;
}

// Função para processar um único arquivo (modificada para medir tempo)
void processarArquivo(const std::filesystem::path& arquivoPath, 
                     const std::string& diretorioSaida,
                     std::mutex& cout_mutex,
                     std::mutex& tempos_mutex) {
    std::string arquivoEntrada = arquivoPath.string();
    std::string nomeArquivo = arquivoPath.filename().string();
    
    // Iniciar cronômetro para esta instância
    auto inicioInstancia = std::chrono::high_resolution_clock::now();
    
    // Buffer para armazenar toda a saída desta instância
    std::ostringstream output;
    
    output << "\n" << separador() << "\n" 
           << colorirBold("▶ Processando instância: ", VERDE) 
           << colorirBold(nomeArquivo, AMARELO) << "\n" 
           << separador() << "\n\n";

    try {
        // Carregar a instância
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(arquivoEntrada);
        
        // Determinar os limites LB e UB com base nas restrições do problema
        int limiteLB = 0;
        int limiteUB = std::numeric_limits<int>::max();
        
        // Formatar detalhes da instância
        output << criarCabecalhoCaixa("DETALHES DA INSTÂNCIA") << "\n";
        output << criarLinhaCaixa(colorir("• Pedidos:    ", VERDE) + 
                                std::to_string(backlog.numPedidos)) << "\n";
        output << criarLinhaCaixa(colorir("• Itens:      ", VERDE) + 
                                std::to_string(deposito.numItens)) << "\n";
        output << criarLinhaCaixa(colorir("• Corredores: ", VERDE) + 
                                std::to_string(deposito.numCorredores)) << "\n";
        output << criarRodapeCaixa() << "\n\n";
        
        // Preparar estruturas auxiliares
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);
        
        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);
        
        AnalisadorRelevancia analisador(backlog.numPedidos);
        
        // Pré-processar relevância dos pedidos
        for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
            if (verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
                analisador.calcularRelevancia(pedidoId, backlog, localizador);
            }
        }
        
        // Acessar os limites da wave a partir do backlog
        if (backlog.wave.LB > 0) {
            limiteLB = backlog.wave.LB;
        } else {
            // Caso contrário, use 30% do total de unidades como LB
            int totalUnidades = 0;
            for (int i = 0; i < backlog.numPedidos; i++) {
                for (const auto& [_, quantidade] : backlog.pedido[i]) {
                    totalUnidades += quantidade;
                }
            }
            limiteLB = std::max(30, totalUnidades / 10); // Pelo menos 30 unidades ou 10% do total
        }
        
        if (backlog.wave.UB > 0) {
            limiteUB = backlog.wave.UB;
        } else {
            // Caso contrário, use 3x o LB como UB
            limiteUB = limiteLB * 3;
        }
        
        // Formatar limites da instância
        output << criarCabecalhoCaixa("LIMITES DA INSTÂNCIA") << "\n";
        output << criarLinhaCaixa(colorir("• Limite Inferior (LB): ", BRANCO) + 
                                colorirBold(std::to_string(limiteLB), VERDE)) << "\n";
        output << criarLinhaCaixa(colorir("• Limite Superior (UB): ", BRANCO) + 
                                colorirBold(std::to_string(limiteUB), VERMELHO)) << "\n";
        output << criarRodapeCaixa() << "\n\n";
        
        output << status("Validando instância...") << "\n\n";
        
        // Estratégia de solução baseada no tamanho da instância
        if (backlog.numPedidos <= 200) {
            // MÉTODO EXATO: Para instâncias pequenas e médias, usar Branch-and-Bound
            output << colorir("Usando método exato (Branch-and-Bound) para instância pequena/média...\n", VERDE);
            
            // Configurações adaptativas para Branch-and-Bound
            double tempoLimiteBnB;
            BranchAndBoundSolver::EstrategiaSelecionarVariavel estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MAIOR_IMPACTO;
            double coeficienteLimite;

            // Ajustar parâmetros com base no tamanho da instância
            if (backlog.numPedidos <= 20) {
                // Instâncias muito pequenas - busca quase exaustiva
                tempoLimiteBnB = 300.0; // 5 minutos
                estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MAIOR_IMPACTO;
                coeficienteLimite = 0.9; // Muito otimista
            } else if (backlog.numPedidos <= 100) {
                // Instâncias pequenas
                tempoLimiteBnB = 180.0; // 3 minutos
                estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::PSEUDO_CUSTO;
                coeficienteLimite = 0.8;
            } else if (backlog.numPedidos <= 1000) {
                // Instâncias médias
                tempoLimiteBnB = 120.0; // 2 minutos
                estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MAIOR_IMPACTO;
                coeficienteLimite = 0.7;
            } else {
                // Instâncias grandes
                tempoLimiteBnB = 60.0; // 1 minuto
                estrategia = BranchAndBoundSolver::EstrategiaSelecionarVariavel::MAIOR_IMPACTO;
                coeficienteLimite = 0.6;
            }

            // Instanciar o solver com os parâmetros ajustados
            BranchAndBoundSolver solver(
                deposito, 
                backlog, 
                localizador, 
                verificador, 
                tempoLimiteBnB,
                estrategia
            );

            solver.setCoeficienteLimite(coeficienteLimite);
            solver.setUsarCortesCobertura(true);
            solver.setUsarCortesDominancia(true);

            // Adicionar no resolverBranchAndBoundPersonalizado
            // Antes de iniciar a busca:

            // Gerar solução inicial gulosa garantindo viabilidade
            std::vector<int> pedidosCandidatos(backlog.numPedidos);
            std::iota(pedidosCandidatos.begin(), pedidosCandidatos.end(), 0);

            // Ordenar por eficiência (unidades por corredor adicional)
            std::sort(pedidosCandidatos.begin(), pedidosCandidatos.end(),
                [&](int a, int b) {
                    // Calcular unidades de cada pedido
                    int unidadesA = 0, unidadesB = 0;
                    for (auto& [_, qtd] : backlog.pedido[a]) unidadesA += qtd;
                    for (auto& [_, qtd] : backlog.pedido[b]) unidadesB += qtd;
                    
                    // Calcular corredores adicionais
                    std::unordered_set<int> corredoresA, corredoresB;
                    for (auto& [itemId, _] : backlog.pedido[a]) {
                        for (int c = 0; c < deposito.numCorredores; c++) {
                            if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                                corredoresA.insert(c);
                            }
                        }
                    }
                    
                    for (auto& [itemId, _] : backlog.pedido[b]) {
                        for (int c = 0; c < deposito.numCorredores; c++) {
                            if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                                corredoresB.insert(c);
                            }
                        }
                    }
                    
                    // Retornar verdadeiro se A é mais eficiente que B
                    return (unidadesA * corredoresB.size()) > (unidadesB * corredoresA.size());
                });

            // Construir solução inicial garantindo LB
            std::vector<int> solucaoInicial;
            std::unordered_set<int> corredoresIncluidos;
            int totalUnidades = 0;

            for (int pedidoId : pedidosCandidatos) {
                if (totalUnidades >= limiteLB) break;  // Já atingiu LB
                
                // Adicionar pedido à solução
                solucaoInicial.push_back(pedidoId);
                
                // Atualizar unidades
                for (auto& [itemId, qtd] : backlog.pedido[pedidoId]) {
                    totalUnidades += qtd;
                    
                    // Adicionar corredores necessários
                    for (int c = 0; c < deposito.numCorredores; c++) {
                        if (deposito.corredor[c].find(itemId) != deposito.corredor[c].end()) {
                            corredoresIncluidos.insert(c);
                        }
                    }
                }
            }

            // O lambda inicial é 0 para maximizar unidades
            double lambda = 0.0;
            
            // Declarar a variável melhorSolucao
            Solucao melhorSolucao;

            // Inicializar a melhorSolucao com esta solução gulosa
            if (totalUnidades >= limiteLB) {
                melhorSolucao.pedidosWave = solucaoInicial;
                melhorSolucao.corredoresWave.assign(corredoresIncluidos.begin(), corredoresIncluidos.end());
                melhorSolucao.valorObjetivo = totalUnidades - lambda * corredoresIncluidos.size();
                
                std::cout << "Solução inicial gulosa: " << totalUnidades << " unidades, " 
                          << corredoresIncluidos.size() << " corredores, valor objetivo " 
                          << melhorSolucao.valorObjetivo << std::endl;
            }

            auto solucaoBnB = solver.resolver(lambda, backlog.wave.LB, backlog.wave.UB);
            
            // Verificar se a solução é viável
            if (solucaoBnB.totalUnidades >= backlog.wave.LB) {
                Solucao solucao;
                solucao.pedidosWave = solucaoBnB.pedidosWave;
                solucao.corredoresWave = solucaoBnB.corredoresWave;
                solucao.valorObjetivo = solucaoBnB.valorObjetivo;
                
                // Antes de salvar a solução, verificar viabilidade
                if (!verificarSolucaoViavel(deposito, backlog, solucao.pedidosWave, solucao.corredoresWave, output)) {
                    output << colorir("AVISO: Solução inviável detectada. Aplicando correção...\n", AMARELO);
                    
                    // Converter SolucaoWave para Solucao antes de reparar
                    Solucao solucaoConvertida;
                    solucaoConvertida.pedidosWave = solucao.pedidosWave;
                    solucaoConvertida.corredoresWave = solucao.corredoresWave;
                    solucaoConvertida.valorObjetivo = solucao.valorObjetivo;
                    
                    // Tentar reparar a solução
                    bool corrigido = repararSolucaoInviavel(deposito, backlog, solucaoConvertida, backlog.wave.LB, backlog.wave.UB, output);
                    
                    // Atualizar a solução original com a versão reparada
                    solucao.pedidosWave = solucaoConvertida.pedidosWave;
                    solucao.corredoresWave = solucaoConvertida.corredoresWave;
                    solucao.valorObjetivo = solucaoConvertida.valorObjetivo;
                    
                    if (!corrigido) {
                        output << colorir("ERRO: Não foi possível reparar a solução. Utilizando solução gulosa básica.\n", VERMELHO);
                        // Gerar uma solução gulosa simples
                        Solucao solucaoGulosa = gerarSolucaoGulosaBasica(deposito, backlog, backlog.wave.LB, backlog.wave.UB);
                        solucao.pedidosWave = solucaoGulosa.pedidosWave;
                        solucao.corredoresWave = solucaoGulosa.corredoresWave;
                        solucao.valorObjetivo = solucaoGulosa.valorObjetivo;
                    }
                }
                
                // Exibir informações
                output << "Branch-and-Bound concluído com sucesso!\n";
                output << "Valor objetivo: " << solucao.valorObjetivo << "\n";
                output << "Total de unidades: " << solucaoBnB.totalUnidades << "\n";
                output << "Total de corredores: " << solucaoBnB.corredoresWave.size() << "\n";
                
                // Salvar solução
                std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
                std::string arquivoSaida = diretorioSaida + "/" + nomeArquivoSemExtensao + ".sol";
                salvarSolucao(solucao.pedidosWave, solucao.corredoresWave, arquivoSaida);
            } else {
                // Se BnB falhar, tentar Dinkelbach com mais tempo
                output << colorir("Branch-and-Bound não encontrou solução viável. Tentando Dinkelbach...\n", AMARELO);
                
                // Configurar Dinkelbach com reinicializações múltiplas
                OtimizadorDinkelbach otimizador(deposito, backlog, localizador, verificador);
                otimizador.configurarParametros(0.00001, 2000, true); // epsilon, maxIteracoes, usarBranchAndBound
                otimizador.setUsarBuscaLocalAvancada(true);
                otimizador.setLimiteTempoBuscaLocal(5.0); // tempo limite para busca local em segundos

                // Configurar reinicializações múltiplas
                OtimizadorDinkelbach::ConfigReinicializacao configReinic;
                configReinic.numReinicializacoes = 10;          // Usar 10 reinicializações
                configReinic.aumentarIteracoesProgressivamente = true;
                configReinic.variarPerturbacao = true;
                configReinic.tamanhoPoolSolucoes = 5;
                configReinic.usarSementesAleatorias = true;

                otimizador.configurarReinicializacoes(configReinic);
                otimizador.habilitarReinicializacoesMultiplas(true);

                // Executar com reinicializações múltiplas
                auto solucao = otimizador.otimizarWaveComReinicializacoes(backlog.wave.LB, backlog.wave.UB);
                
                // Converter SolucaoWave para Solucao
                Solucao solucaoConvertida;
                solucaoConvertida.pedidosWave = solucao.pedidosWave;
                solucaoConvertida.corredoresWave = solucao.corredoresWave;
                solucaoConvertida.valorObjetivo = solucao.valorObjetivo;
                
                // Antes de salvar a solução, verificar viabilidade
                if (!verificarSolucaoViavel(deposito, backlog, solucaoConvertida.pedidosWave, solucaoConvertida.corredoresWave, output)) {
                    output << colorir("AVISO: Solução inviável detectada. Aplicando correção...\n", AMARELO);
                    
                    // Tentar reparar a solução
                    bool corrigido = repararSolucaoInviavel(deposito, backlog, solucaoConvertida, backlog.wave.LB, backlog.wave.UB, output);
                    
                    if (!corrigido) {
                        output << colorir("ERRO: Não foi possível reparar a solução. Utilizando solução gulosa básica.\n", VERMELHO);
                        // Gerar uma solução gulosa simples
                        Solucao solucaoGulosa = gerarSolucaoGulosaBasica(deposito, backlog, backlog.wave.LB, backlog.wave.UB);
                        solucaoConvertida.pedidosWave = solucaoGulosa.pedidosWave;
                        solucaoConvertida.corredoresWave = solucaoGulosa.corredoresWave;
                        solucaoConvertida.valorObjetivo = solucaoGulosa.valorObjetivo;
                    }
                }
                
                // Salvar solução
                std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
                std::string arquivoSaida = diretorioSaida + "/" + nomeArquivoSemExtensao + ".sol";
                salvarSolucao(solucaoConvertida.pedidosWave, solucaoConvertida.corredoresWave, arquivoSaida);
                
                // Exibir informações
                output << "Otimização Dinkelbach concluída.\n";
                output << "Valor objetivo: " << solucaoConvertida.valorObjetivo << "\n";
                output << "Pedidos na wave: " << solucaoConvertida.pedidosWave.size() << "\n";
                output << "Corredores: " << solucaoConvertida.corredoresWave.size() << "\n";
            }
        } else {
            // MÉTODO HÍBRIDO: Para instâncias grandes, usar Dinkelbach + Busca Local Avançada
            output << colorir("Usando método híbrido para instância grande...\n", VERDE);
            
            // Configurar Dinkelbach para execução mais longa e precisa
            OtimizadorDinkelbach otimizador(deposito, backlog, localizador, verificador);
            otimizador.configurarParametros(0.00001, 150, false); // Mais iterações, sem BnB interno
            otimizador.setUsarBuscaLocalAvancada(true);
            otimizador.setLimiteTempoBuscaLocal(10.0); // Tempo generoso para busca local
            
            auto solucao = otimizador.otimizarWave(backlog.wave.LB, backlog.wave.UB);
            
            // Antes de salvar a solução, verificar viabilidade
            if (!verificarSolucaoViavel(deposito, backlog, solucao.pedidosWave, solucao.corredoresWave, output)) {
                output << colorir("AVISO: Solução inviável detectada. Aplicando correção...\n", AMARELO);
                
                // Converter SolucaoWave para Solucao antes de reparar
                Solucao solucaoConvertida;
                solucaoConvertida.pedidosWave = solucao.pedidosWave;
                solucaoConvertida.corredoresWave = solucao.corredoresWave;
                solucaoConvertida.valorObjetivo = solucao.valorObjetivo;
                
                // Tentar reparar a solução
                bool corrigido = repararSolucaoInviavel(deposito, backlog, solucaoConvertida, backlog.wave.LB, backlog.wave.UB, output);
                
                // Atualizar a solução original com a versão reparada
                solucao.pedidosWave = solucaoConvertida.pedidosWave;
                solucao.corredoresWave = solucaoConvertida.corredoresWave;
                solucao.valorObjetivo = solucaoConvertida.valorObjetivo;
                
                if (!corrigido) {
                    output << colorir("ERRO: Não foi possível reparar a solução. Utilizando solução gulosa básica.\n", VERMELHO);
                    // Gerar uma solução gulosa simples
                    Solucao solucaoGulosa = gerarSolucaoGulosaBasica(deposito, backlog, backlog.wave.LB, backlog.wave.UB);
                    solucao.pedidosWave = solucaoGulosa.pedidosWave;
                    solucao.corredoresWave = solucaoGulosa.corredoresWave;
                    solucao.valorObjetivo = solucaoGulosa.valorObjetivo;
                }
            }
            
            // Aplicar refinamento com Busca Local Avançada
            Solucao solucaoBL;
            solucaoBL.pedidosWave = solucao.pedidosWave;
            solucaoBL.corredoresWave = solucao.corredoresWave;
            solucaoBL.valorObjetivo = solucao.valorObjetivo;
            
            // Configuração para ILS (Iterated Local Search)
            BuscaLocalAvancada buscaLocal(deposito, backlog, localizador, verificador, 20.0);
            
            // Criar configuração para ILS usando a estrutura definida na classe BuscaLocalAvancada
            BuscaLocalAvancada::ConfigILS configILS;
            configILS.maxIteracoes = 2000;
            configILS.perturbacoesSemMelhoria = 1000;
            configILS.intensidadePerturbacaoBase = 0.3;
            buscaLocal.configurarILS(configILS);
            
            // Converter Solucao para BuscaLocalAvancada::Solucao
            BuscaLocalAvancada::Solucao solucaoBL_interna;
            solucaoBL_interna.pedidosWave = solucaoBL.pedidosWave;
            solucaoBL_interna.corredoresWave = solucaoBL.corredoresWave;
            solucaoBL_interna.valorObjetivo = solucaoBL.valorObjetivo;
            
            // Usar o enum apropriado para a estratégia de busca local
            auto solucaoRefinada = buscaLocal.otimizar(solucaoBL_interna, backlog.wave.LB, backlog.wave.UB, 
                                                      BuscaLocalAvancada::TipoBuscaLocal::ILS);
            
            // Atualizar a solução com o resultado refinado
            solucao.pedidosWave = solucaoRefinada.pedidosWave;
            solucao.corredoresWave = solucaoRefinada.corredoresWave;
            solucao.valorObjetivo = solucaoRefinada.valorObjetivo;
            
            // Salvar solução
            std::string nomeArquivoSemExtensao = nomeArquivo.substr(0, nomeArquivo.find_last_of("."));
            std::string arquivoSaida = diretorioSaida + "/" + nomeArquivoSemExtensao + ".sol";
            salvarSolucao(solucao.pedidosWave, solucao.corredoresWave, arquivoSaida);
            
            // Exibir informações
            output << "Otimização Híbrida concluída.\n";
            output << "Valor objetivo final: " << solucaoRefinada.valorObjetivo << "\n";
            output << "Pedidos na wave: " << solucaoRefinada.pedidosWave.size() << "\n";
            output << "Corredores: " << solucaoRefinada.corredoresWave.size() << "\n";
            output << "Estatísticas da busca local:\n" << buscaLocal.obterEstatisticas() << "\n";
        }
        
        // Calcular e registrar o tempo de processamento
        auto fimInstancia = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> tempoDecorrido = fimInstancia - inicioInstancia;
        
        // Armazenar o tempo de execução desta instância
        {
            std::lock_guard<std::mutex> lock(tempos_mutex);
            temposExecucao.temposPorInstancia[nomeArquivo] = tempoDecorrido.count();
        }
        
        // Formatar tempo com precisão fixa
        std::stringstream tempoStr;
        tempoStr << std::fixed << std::setprecision(3) << tempoDecorrido.count();
        
        output << criarCabecalhoCaixa("RESULTADOS") << "\n";
        output << criarLinhaCaixa(colorir("✓ Tempo: ", VERDE) + 
                                colorirBold(tempoStr.str() + " s", CIANO)) << "\n";
        output << criarRodapeCaixa() << "\n\n";
        
        // Exibir toda a saída de uma vez, protegida por mutex
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << output.str();
        }
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << erro("ERRO: " + std::string(e.what())) << std::endl << std::endl;
    }
}

void solucionarDesafio(const std::string& diretorioEntrada, const std::string& diretorioSaida) {
    // Iniciar cronômetro para tempo total
    temposExecucao.inicioGeral = std::chrono::high_resolution_clock::now();
    
    std::cout << cabecalho("SOLUÇÃO DO DESAFIO") << std::endl;
    std::cout << colorir("• Diretório de entrada: ", CIANO) << diretorioEntrada << std::endl;
    std::cout << colorir("• Diretório de saída: ", CIANO) << diretorioSaida << std::endl << std::endl;
    
    // Criar diretório de saída se não existir
    std::filesystem::create_directories(diretorioSaida);
    
    // Coletar todos os arquivos de entrada
    std::vector<std::filesystem::path> arquivosEntrada;
    for (const auto& entry : std::filesystem::directory_iterator(diretorioEntrada)) {
        if (entry.is_regular_file()) {
            arquivosEntrada.push_back(entry.path());
        }
    }
    
    // Processamento paralelo
    unsigned int numThreads = std::thread::hardware_concurrency();
    numThreads = std::max(1u, std::min(numThreads, 4u));  // Limitar a 4 threads para reduzir confusão na saída
    
    std::cout << colorir("• Utilizando ", CIANO) << 
                 colorirBold(std::to_string(numThreads), AMARELO) << 
                 colorir(" threads para processamento paralelo.", CIANO) << std::endl << std::endl;
    
    std::vector<std::thread> threads;
    std::mutex cout_mutex;
    std::mutex tempos_mutex; // Mutex adicional para proteger o acesso à estrutura de tempos
    
    // Usar processamento paralelo dividido em grupos
    for (size_t i = 0; i < arquivosEntrada.size(); i += numThreads) {
        size_t remainingFiles = std::min(static_cast<size_t>(numThreads), arquivosEntrada.size() - i);
        threads.reserve(remainingFiles);
        
        for (size_t j = 0; j < remainingFiles; ++j) {
            threads.emplace_back(processarArquivo, arquivosEntrada[i + j], diretorioSaida, 
                              std::ref(cout_mutex), std::ref(tempos_mutex));
        }
        
        // Esperar todas as threads terminarem
        for (auto& thread : threads) {
            thread.join();
        }
        
        threads.clear();
    }
    
    // Calcular o tempo total de execução
    auto fimGeral = std::chrono::high_resolution_clock::now();
    temposExecucao.tempoTotalExecucao = fimGeral - temposExecucao.inicioGeral;
    
    // Salvar os tempos em um arquivo
    salvarTemposExecucao();
    
    std::cout << std::endl;
    std::cout << cabecalho("PROCESSAMENTO CONCLUÍDO") << std::endl;
    std::cout << colorirBold("Todas as instâncias foram processadas com sucesso!", VERDE) << std::endl;
    std::cout << colorir("Tempo total de execução: ", CIANO) << 
               colorirBold(formatarTempo(temposExecucao.tempoTotalExecucao.count()), AMARELO) << std::endl << std::endl;
}