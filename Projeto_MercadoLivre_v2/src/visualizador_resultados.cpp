#include "visualizador_resultados.h"
#include "parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <filesystem>

void VisualizadorResultados::visualizarDeposito(const Deposito& deposito, const std::string& caminhoSaida) {
    std::ofstream outFile(caminhoSaida);
    
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo para visualização: " << caminhoSaida << std::endl;
        return;
    }
    
    // Cabeçalho do arquivo HTML
    outFile << "<!DOCTYPE html>\n"
            << "<html>\n"
            << "<head>\n"
            << "    <title>Visualização do Depósito</title>\n"
            << "    <style>\n"
            << "        .corredor { margin-bottom: 20px; border: 1px solid #ccc; padding: 10px; }\n"
            << "        .corredor-header { font-weight: bold; margin-bottom: 10px; }\n"
            << "        .item { display: inline-block; margin: 5px; padding: 5px; background-color: #f0f0f0; border-radius: 5px; }\n"
            << "        .item-quantidade { font-weight: bold; color: #007bff; }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Visualização do Depósito</h1>\n"
            << "    <div class=\"info\">\n"
            << "        <p>Número de Corredores: " << deposito.numCorredores << "</p>\n"
            << "        <p>Número de Itens: " << deposito.numItens << "</p>\n"
            << "    </div>\n"
            << "    <div class=\"deposito\">\n";
    
    // Para cada corredor, listar seus itens
    for (int c = 0; c < deposito.numCorredores; c++) {
        outFile << "        <div class=\"corredor\">\n"
                << "            <div class=\"corredor-header\">Corredor " << c << " (" 
                << deposito.corredor[c].size() << " itens)</div>\n";
        
        // Listar itens
        for (const auto& [itemId, quantidade] : deposito.corredor[c]) {
            outFile << "            <div class=\"item\">Item " << itemId 
                    << " <span class=\"item-quantidade\">(" << quantidade << ")</span></div>\n";
        }
        
        outFile << "        </div>\n";
    }
    
    // Rodapé do arquivo HTML
    outFile << "    </div>\n"
            << "</body>\n"
            << "</html>\n";
    
    outFile.close();
}

void VisualizadorResultados::visualizarWave(
    const Deposito& deposito,
    const Backlog& backlog,
    const std::vector<int>& pedidosWave,
    const std::vector<int>& corredoresWave,
    const std::string& caminhoSaida
) {
    std::ofstream outFile(caminhoSaida);
    
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo para visualização: " << caminhoSaida << std::endl;
        return;
    }
    
    // Calcular estatísticas da wave
    int totalUnidades = 0;
    std::unordered_map<int, int> itensNaWave; // itemId -> quantidade
    
    for (int pedidoId : pedidosWave) {
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            totalUnidades += quantidade;
            itensNaWave[itemId] += quantidade;
        }
    }
    
    double razaoWave = corredoresWave.empty() ? 0.0 : 
                      static_cast<double>(totalUnidades) / corredoresWave.size();
    
    // Cabeçalho do arquivo HTML
    outFile << "<!DOCTYPE html>\n"
            << "<html>\n"
            << "<head>\n"
            << "    <title>Visualização da Wave</title>\n"
            << "    <style>\n"
            << "        .wave-info { margin-bottom: 20px; }\n"
            << "        .pedidos, .corredores { margin-bottom: 20px; }\n"
            << "        .pedido, .corredor { margin: 5px; padding: 10px; border: 1px solid #ccc; }\n"
            << "        .pedido-header, .corredor-header { font-weight: bold; margin-bottom: 10px; }\n"
            << "        .item { margin: 5px; padding: 5px; background-color: #f0f0f0; border-radius: 5px; }\n"
            << "        .corredor-item { display: inline-block; }\n"
            << "        .item-quantidade { font-weight: bold; color: #007bff; }\n"
            << "        .mapa-calor { margin-top: 30px; border-collapse: collapse; }\n"
            << "        .mapa-calor td { width: 30px; height: 30px; text-align: center; }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Visualização da Wave</h1>\n"
            << "    <div class=\"wave-info\">\n"
            << "        <p>Número de Pedidos: " << pedidosWave.size() << "</p>\n"
            << "        <p>Número de Corredores: " << corredoresWave.size() << "</p>\n"
            << "        <p>Total de Unidades: " << totalUnidades << "</p>\n"
            << "        <p>Razão Unidades/Corredores: " << std::fixed << std::setprecision(2) 
            << razaoWave << "</p>\n"
            << "    </div>\n";
    
    // Visualizar pedidos
    outFile << "    <h2>Pedidos na Wave</h2>\n"
            << "    <div class=\"pedidos\">\n";
    
    for (int pedidoId : pedidosWave) {
        outFile << "        <div class=\"pedido\">\n"
                << "            <div class=\"pedido-header\">Pedido " << pedidoId << " (" 
                << backlog.pedido[pedidoId].size() << " tipos de itens)</div>\n";
        
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            outFile << "            <div class=\"item\">Item " << itemId 
                    << " <span class=\"item-quantidade\">(" << quantidade << ")</span></div>\n";
        }
        
        outFile << "        </div>\n";
    }
    
    outFile << "    </div>\n";
    
    // Visualizar corredores
    outFile << "    <h2>Corredores na Wave</h2>\n"
            << "    <div class=\"corredores\">\n";
    
    for (int corredorId : corredoresWave) {
        outFile << "        <div class=\"corredor\">\n"
                << "            <div class=\"corredor-header\">Corredor " << corredorId << "</div>\n";
        
        for (const auto& [itemId, quantidade] : deposito.corredor[corredorId]) {
            if (itensNaWave.find(itemId) != itensNaWave.end()) {
                outFile << "            <div class=\"corredor-item item\">Item " << itemId 
                        << " <span class=\"item-quantidade\">(" << quantidade << ")</span></div>\n";
            }
        }
        
        outFile << "        </div>\n";
    }
    
    outFile << "    </div>\n";
    
    // Criar mapa de calor mostrando relação entre pedidos e corredores
    outFile << "    <h2>Mapa de Relação Pedidos-Corredores</h2>\n"
            << "    <table class=\"mapa-calor\">\n";
    
    // Cabeçalho da tabela (corredores)
    outFile << "        <tr><td></td>";
    for (int corredorId : corredoresWave) {
        outFile << "<td>C" << corredorId << "</td>";
    }
    outFile << "</tr>\n";
    
    // Corpo da tabela (pedidos x corredores)
    for (int pedidoId : pedidosWave) {
        outFile << "        <tr><td>P" << pedidoId << "</td>";
        
        for (int corredorId : corredoresWave) {
            // Contar quantos itens do pedido estão neste corredor
            int itensComuns = 0;
            
            for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
                if (deposito.corredor[corredorId].find(itemId) != deposito.corredor[corredorId].end()) {
                    itensComuns++;
                }
            }
            
            // Definir cor com base na intensidade da relação
            int intensidade = 255 - std::min(255, itensComuns * 50);
            std::string cor = "rgb(" + std::to_string(intensidade) + "," 
                           + std::to_string(intensidade) + "," 
                           + std::to_string(255) + ")";
            
            outFile << "<td style=\"background-color: " << cor << "\">" 
                    << itensComuns << "</td>";
        }
        
        outFile << "</tr>\n";
    }
    
    outFile << "    </table>\n";
    
    // Rodapé do arquivo HTML
    outFile << "</body>\n"
            << "</html>\n";
    
    outFile.close();
}

void VisualizadorResultados::gerarMapaCalor(
    const Deposito& deposito,
    const Backlog& backlog,
    const std::string& caminhoSaida
) {
    std::ofstream outFile(caminhoSaida);
    
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo para mapa de calor: " << caminhoSaida << std::endl;
        return;
    }
    
    // Código para gerar mapa de calor completo do depósito e backlog
    // Isto pode ser uma visualização estilo heatmap que mostra onde estão
    // as maiores concentrações de itens, quais corredores são mais importantes, etc.
    
    outFile.close();
}

void VisualizadorResultados::gerarComparativoSolucoes(
    const std::vector<std::pair<std::string, Solucao>>& resultados,
    const std::string& caminhoSaida
) {
    std::ofstream outFile(caminhoSaida);
    
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo para comparativo: " << caminhoSaida << std::endl;
        return;
    }
    
    // Código para gerar um comparativo visual entre diferentes soluções
    // Isto poderia incluir gráficos de barras, tabelas comparativas, etc.
    
    outFile.close();
}

void VisualizadorResultados::gerarDashboardInterativo(
    const std::string& diretorioEntrada,
    const std::string& diretorioSaida,
    const std::string& arquivoDashboard
) {
    std::ofstream outFile(arquivoDashboard);
    
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir arquivo para dashboard: " << arquivoDashboard << std::endl;
        return;
    }
    
    // Código para gerar um dashboard HTML interativo que integra
    // várias visualizações e permite filtrar e comparar instâncias
    
    outFile.close();
}