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
    std::unordered_map<int, int> itensNaWave;
    
    for (int pedidoId : pedidosWave) {
        for (const auto& par : backlog.pedido[pedidoId]) {
            int itemId = par.first;
            int quantidade = par.second;
            totalUnidades += quantidade;
            itensNaWave[itemId] += quantidade;
        }
    }
    
    double razaoWave = corredoresWave.empty() ? 0.0 : 
                      static_cast<double>(totalUnidades) / corredoresWave.size();
    
    // Gerar HTML
    outFile << "<!DOCTYPE html>\n"
            << "<html>\n"
            << "<head>\n"
            << "    <title>Visualização da Wave</title>\n"
            << "    <style>\n"
            << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
            << "        .wave-info { margin-bottom: 20px; padding: 10px; background-color: #f0f0f0; }\n"
            << "        .pedidos { margin-bottom: 20px; }\n"
            << "        .corredores { margin-bottom: 20px; }\n"
            << "        table { border-collapse: collapse; width: 100%; }\n"
            << "        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
            << "        th { background-color: #f2f2f2; }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Visualização da Wave</h1>\n";
    
    // Informações resumidas
    outFile << "    <div class=\"wave-info\">\n"
            << "        <h2>Resumo</h2>\n"
            << "        <p><strong>Número de Pedidos:</strong> " << pedidosWave.size() << "</p>\n"
            << "        <p><strong>Número de Corredores:</strong> " << corredoresWave.size() << "</p>\n"
            << "        <p><strong>Total de Unidades:</strong> " << totalUnidades << "</p>\n"
            << "        <p><strong>Razão Unidades/Corredores:</strong> " << std::fixed << std::setprecision(2) << razaoWave << "</p>\n"
            << "    </div>\n";
    
    // Lista de pedidos
    outFile << "    <div class=\"pedidos\">\n"
            << "        <h2>Pedidos na Wave</h2>\n"
            << "        <table>\n"
            << "            <tr><th>ID Pedido</th><th>Itens</th><th>Unidades</th></tr>\n";
    
    for (int pedidoId : pedidosWave) {
        int unidadesPedido = 0;
        for (const auto& par : backlog.pedido[pedidoId]) {
            unidadesPedido += par.second;
        }
        
        outFile << "            <tr>\n"
                << "                <td>" << pedidoId << "</td>\n"
                << "                <td>" << backlog.pedido[pedidoId].size() << "</td>\n"
                << "                <td>" << unidadesPedido << "</td>\n"
                << "            </tr>\n";
    }
    
    outFile << "        </table>\n"
            << "    </div>\n";
    
    // Lista de corredores
    outFile << "    <div class=\"corredores\">\n"
            << "        <h2>Corredores Necessários</h2>\n"
            << "        <table>\n"
            << "            <tr><th>ID Corredor</th><th>Itens Distintos</th></tr>\n";
    
    for (int corredorId : corredoresWave) {
        outFile << "            <tr>\n"
                << "                <td>" << corredorId << "</td>\n"
                << "                <td>" << deposito.corredor[corredorId].size() << "</td>\n"
                << "            </tr>\n";
    }
    
    outFile << "        </table>\n"
            << "    </div>\n";
    
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