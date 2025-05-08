#include "otimizador_distribuido.h"
#include "parser.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "otimizador_dinkelbach.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>

OtimizadorDistribuido::OtimizadorDistribuido(
    const std::string& diretorioEntrada,
    const std::string& diretorioSaida,
    int numUnidades
) : 
    diretorioEntrada_(diretorioEntrada),
    diretorioSaida_(diretorioSaida) 
{
    // Determinar número de unidades de processamento automaticamente se não especificado
    numUnidadesProcessamento_ = numUnidades;
    if (numUnidadesProcessamento_ <= 0) {
        numUnidadesProcessamento_ = std::thread::hardware_concurrency();
        if (numUnidadesProcessamento_ == 0) {
            numUnidadesProcessamento_ = 4; // Valor padrão se não for possível detectar
        }
    }
    
    // Criar diretório de saída se não existir
    std::filesystem::create_directories(diretorioSaida_);
}

std::vector<std::string> OtimizadorDistribuido::decomporInstancia(
    const std::string& caminhoInstancia,
    int numParticoes
) {
    std::vector<std::string> arquivosSubproblemas;
    
    // Se numParticoes não for especificado, usar o número de unidades de processamento
    if (numParticoes <= 0) {
        numParticoes = numUnidadesProcessamento_;
    }
    
    // Carregando a instância original
    InputParser parser;
    auto [deposito, backlog] = parser.parseFile(caminhoInstancia);
    
    // Extrair o nome base do arquivo para criar os arquivos de subproblemas
    std::string nomeBase = std::filesystem::path(caminhoInstancia).stem().string();
    std::string dirTemp = diretorioSaida_ + "/temp_" + nomeBase;
    std::filesystem::create_directories(dirTemp);
    
    // Dividir os pedidos em grupos
    int pedidosPorParticao = std::max(1, backlog.numPedidos / numParticoes);
    
    for (int i = 0; i < numParticoes; i++) {
        // Determinar intervalo de pedidos para esta partição
        int inicio = i * pedidosPorParticao;
        int fim = (i == numParticoes - 1) ? backlog.numPedidos : (i + 1) * pedidosPorParticao;
        
        // Criar uma nova instância apenas com estes pedidos
        Backlog backlogParcial;
        backlogParcial.numPedidos = fim - inicio;
        backlogParcial.wave = backlog.wave;
        
        // Ajustar os limites LB/UB proporcionalmente
        backlogParcial.wave.LB = std::max(1, backlog.wave.LB / numParticoes);
        backlogParcial.wave.UB = backlog.wave.UB / numParticoes;
        
        // Copiar apenas os pedidos relevantes
        backlogParcial.pedido.resize(backlogParcial.numPedidos);
        for (int j = inicio; j < fim; j++) {
            backlogParcial.pedido[j - inicio] = backlog.pedido[j];
        }
        
        // Salvar subproblema em arquivo (implementação simplificada)
        std::string arquivoSubproblema = dirTemp + "/subproblema_" + std::to_string(i) + ".txt";
        
        // Aqui seria necessário implementar uma função para salvar a instância em arquivo
        // Por ora, armazenamos apenas o nome do arquivo
        
        arquivosSubproblemas.push_back(arquivoSubproblema);
    }
    
    return arquivosSubproblemas;
}

std::vector<std::future<Solucao>> OtimizadorDistribuido::resolverSubproblemasParalelo(
    const std::vector<std::string>& arquivosSubproblemas
) {
    std::vector<std::future<Solucao>> resultados;
    
    // Função lambda para resolver um subproblema
    auto resolverSubproblema = [this](const std::string& arquivo) -> Solucao {
        InputParser parser;
        auto [deposito, backlog] = parser.parseFile(arquivo);
        
        // Criar estruturas auxiliares
        LocalizadorItens localizador(deposito.numItens);
        localizador.construir(deposito);
        
        VerificadorDisponibilidade verificador(deposito.numItens);
        verificador.construir(deposito);
        
        // Resolver com Dinkelbach
        OtimizadorDinkelbach otimizador(deposito, backlog, localizador, verificador);
        auto solucaoWave = otimizador.otimizarWave(backlog.wave.LB, backlog.wave.UB);
        
        // Converter para o tipo Solucao
        Solucao solucao;
        solucao.pedidosWave = solucaoWave.pedidosWave;
        solucao.corredoresWave = solucaoWave.corredoresWave;
        solucao.valorObjetivo = solucaoWave.valorObjetivo;
        
        return solucao;
    };
    
    // Iniciar uma tarefa assíncrona para cada subproblema
    for (const auto& arquivo : arquivosSubproblemas) {
        resultados.push_back(std::async(std::launch::async, resolverSubproblema, arquivo));
    }
    
    return resultados;
}

Solucao OtimizadorDistribuido::combinarSolucoes(
    const std::vector<Solucao>& solucoesSubproblemas,
    const Deposito& deposito,
    const Backlog& backlog
) {
    Solucao solucaoCombinada;
    
    // Inicializamos com valores vazios
    solucaoCombinada.valorObjetivo = 0.0;
    
    // Variáveis para calcular o novo valor objetivo
    int totalUnidades = 0;
    std::unordered_set<int> corredoresUnicos;
    
    // Combinamos todas as soluções
    for (const auto& solucao : solucoesSubproblemas) {
        // Adicionamos os pedidos desta solução
        for (int pedidoId : solucao.pedidosWave) {
            solucaoCombinada.pedidosWave.push_back(pedidoId);
            
            // Atualizamos o total de unidades
            for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                totalUnidades += quantidade;
            }
        }
        
        // Adicionamos os corredores
        for (int corredorId : solucao.corredoresWave) {
            corredoresUnicos.insert(corredorId);
        }
    }
    
    // Verificamos se estamos respeitando os limites
    if (solucaoCombinada.pedidosWave.size() < backlog.wave.LB || 
        solucaoCombinada.pedidosWave.size() > backlog.wave.UB) {
        
        // Ajustar a solução para respeitar os limites
        if (solucaoCombinada.pedidosWave.size() < backlog.wave.LB) {
            // Precisamos adicionar mais pedidos
            // Aqui seria necessário implementar uma heurística para adicionar pedidos
            std::cout << "Aviso: Solução combinada tem menos pedidos que o limite LB." << std::endl;
        }
        
        if (solucaoCombinada.pedidosWave.size() > backlog.wave.UB) {
            // Precisamos remover alguns pedidos
            // Ordenar os pedidos por densidade (unidades/corredores)
            std::vector<std::pair<int, double>> pedidosDensidade;
            
            for (size_t i = 0; i < solucaoCombinada.pedidosWave.size(); i++) {
                int pedidoId = solucaoCombinada.pedidosWave[i];
                
                // Calcular unidades
                int unidades = 0;
                for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                    unidades += quantidade;
                }
                
                // Calcular corredores
                std::unordered_set<int> corredores;
                for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                    // Aqui precisaríamos do localizador para saber os corredores
                    // Por simplicidade, usamos uma aproximação
                    corredores.insert(itemId % deposito.numCorredores);
                }
                
                double densidade = unidades / static_cast<double>(corredores.size());
                pedidosDensidade.push_back({pedidoId, densidade});
            }
            
            // Ordenar por densidade (decrescente)
            std::sort(pedidosDensidade.begin(), pedidosDensidade.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            // Manter apenas os UB melhores pedidos
            solucaoCombinada.pedidosWave.clear();
            for (int i = 0; i < backlog.wave.UB; i++) {
                if (i < pedidosDensidade.size()) {
                    solucaoCombinada.pedidosWave.push_back(pedidosDensidade[i].first);
                }
            }
            
            // Recalcular unidades e corredores
            totalUnidades = 0;
            corredoresUnicos.clear();
            
            for (int pedidoId : solucaoCombinada.pedidosWave) {
                for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                    totalUnidades += quantidade;
                    
                    // Aqui, novamente, precisaríamos do localizador
                    // Simplificação para o exemplo
                    corredoresUnicos.insert(itemId % deposito.numCorredores);
                }
            }
        }
    }
    
    // Convertemos os corredores únicos para o formato de vetor
    solucaoCombinada.corredoresWave.assign(corredoresUnicos.begin(), corredoresUnicos.end());
    
    // Calculamos o valor objetivo final
    if (!corredoresUnicos.empty()) {
        solucaoCombinada.valorObjetivo = static_cast<double>(totalUnidades) / corredoresUnicos.size();
    }
    
    return solucaoCombinada;
}

Solucao OtimizadorDistribuido::solucionarInstanciaMassiva(
    const std::string& caminhoInstancia,
    int numParticoes
) {
    std::cout << "Iniciando solução da instância massiva: " << caminhoInstancia << std::endl;
    
    // 1. Decompor a instância em subproblemas
    auto arquivosSubproblemas = decomporInstancia(caminhoInstancia, numParticoes);
    std::cout << "Instância decomposta em " << arquivosSubproblemas.size() << " subproblemas" << std::endl;
    
    // 2. Resolver subproblemas em paralelo
    auto futuros = resolverSubproblemasParalelo(arquivosSubproblemas);
    
    // 3. Coletar resultados
    std::vector<Solucao> solucoes;
    for (auto& futuro : futuros) {
        solucoes.push_back(futuro.get());
    }
    
    // 4. Carregar a instância original para combinar soluções
    InputParser parser;
    auto [deposito, backlog] = parser.parseFile(caminhoInstancia);
    
    // 5. Combinar soluções
    Solucao solucaoFinal = combinarSolucoes(solucoes, deposito, backlog);
    
    // 6. Limpar arquivos temporários
    for (const auto& arquivo : arquivosSubproblemas) {
        std::filesystem::remove(arquivo);
    }
    
    std::cout << "Solução combinada concluída. Valor objetivo: " << solucaoFinal.valorObjetivo << std::endl;
    
    return solucaoFinal;
}