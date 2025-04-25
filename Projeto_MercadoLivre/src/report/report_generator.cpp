#include "report/report_generator.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>

bool ReportGenerator::generateReport(const std::string& instanceFile, const std::string& outputPath) {
    try {
        std::cout << "Gerando relatório para instância: " << instanceFile << std::endl;
        
        // Parsear a instância
        InputParser parser;
        Warehouse warehouse = parser.parseFile(instanceFile);
        
        // Criar solução vazia
        Solution solution;
        
        // Criar estruturas auxiliares
        if (!cria_auxiliares(warehouse, solution)) {
            std::cerr << "Erro ao criar estruturas auxiliares." << std::endl;
            return false;
        }
        
        // Gerar relatório com os dados processados
        return generateReportFromProcessedData(warehouse, solution, outputPath);
    }
    catch (const std::exception& e) {
        std::cerr << "Erro ao gerar relatório: " << e.what() << std::endl;
        return false;
    }
}

bool ReportGenerator::generateReportFromProcessedData(const Warehouse& warehouse, 
                                                     const Solution& solution,
                                                     const std::string& outputPath) {
    // Recuperar estruturas auxiliares da solução
    AuxiliaryStructures aux = solution.getAuxiliaryData<AuxiliaryStructures>("structures");
    
    // Calcular estatísticas avançadas
    AuxiliaryStructures::OrderStatistics orderStats;
    calculateOrderStatistics(aux, orderStats);
    
    // Calcular estatísticas de itens
    AuxiliaryStructures::ItemStatistics itemStats;
    calculateItemStatistics(aux, itemStats);
    
    // Nome do arquivo da saída (baseado na data/hora)
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now_time));
    
    std::string filename = outputPath + "/report_" + std::string(timestamp) + ".html";
    
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Erro ao abrir arquivo de saída: " << filename << std::endl;
        return false;
    }
    
    // Gerar HTML
    outFile << generateHtmlHeader(warehouse);
    outFile << generateInstanceSummary(warehouse);
    outFile << generateStatisticalSummary(orderStats, itemStats); 
    outFile << generateOrdersSection(warehouse, aux, orderStats);
    outFile << generateItemsSection(warehouse, aux, itemStats);
    outFile << generateCorridorsSection(warehouse, aux);
    outFile << generateMetricsSection(aux);
    outFile << generateHtmlFooter();
    
    outFile.close();
    
    std::cout << "Relatório gerado com sucesso: " << filename << std::endl;
    return true;
}

std::string ReportGenerator::generateHtmlHeader(const Warehouse& warehouse) {
    std::stringstream ss;
    
    ss << "<!DOCTYPE html>\n"
       << "<html lang=\"pt-br\">\n"
       << "<head>\n"
       << "    <meta charset=\"UTF-8\">\n"
       << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "    <title>Relatório de Estruturas - Mercado Livre Otimizador</title>\n"
       << "    <script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n"
       << "    <style>\n"
       << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
       << "        h1, h2, h3 { color: #333; }\n"
       << "        .section { margin-bottom: 30px; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }\n"
       << "        table { border-collapse: collapse; width: 100%; margin-bottom: 20px; }\n"
       << "        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
       << "        th { background-color: #f2f2f2; }\n"
       << "        tr:nth-child(even) { background-color: #f9f9f9; }\n"
       << "        .chart-container { height: 400px; margin-bottom: 30px; }\n"
       << "        .metric-box { display: inline-block; width: 200px; margin: 10px; padding: 15px; text-align: center; background-color: #f0f0f0; border-radius: 5px; }\n"
       << "        .metric-box .value { font-size: 24px; font-weight: bold; margin: 10px 0; }\n"
       << "        .metric-box .label { font-size: 14px; color: #666; }\n"
       << "        .subsection { margin-bottom: 20px; padding: 10px; border-left: 3px solid #ddd; }\n"
       << "        .stats-container { display: flex; flex-wrap: wrap; justify-content: space-between; margin: 15px 0; }\n"
       << "        .quartile-table { width: 60%; margin: 15px auto; border-collapse: collapse; }\n"
       << "        .quartile-table th, .quartile-table td { border: 1px solid #ddd; padding: 8px; text-align: center; }\n"
       << "        .quartile-table th { background-color: #f5f5f5; }\n"
       << "    </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "    <h1>Relatório de Estruturas Auxiliares - Mercado Livre</h1>\n";
    
    return ss.str();
}

std::string ReportGenerator::generateInstanceSummary(const Warehouse& warehouse) {
    std::stringstream ss;
    
    ss << "    <div class=\"section\">\n"
       << "        <h2>Resumo da Instância</h2>\n"
       << "        <div class=\"metric-box\">\n"
       << "            <div class=\"label\">Pedidos</div>\n"
       << "            <div class=\"value\">" << warehouse.numOrders << "</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-box\">\n"
       << "            <div class=\"label\">Itens</div>\n"
       << "            <div class=\"value\">" << warehouse.numItems << "</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-box\">\n"
       << "            <div class=\"label\">Corredores</div>\n"
       << "            <div class=\"value\">" << warehouse.numCorridors << "</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-box\">\n"
       << "            <div class=\"label\">Limite Inferior (LB)</div>\n"
       << "            <div class=\"value\">" << warehouse.LB << "</div>\n"
       << "        </div>\n"
       << "        <div class=\"metric-box\">\n"
       << "            <div class=\"label\">Limite Superior (UB)</div>\n"
       << "            <div class=\"value\">" << warehouse.UB << "</div>\n"
       << "        </div>\n"
       << "    </div>\n";
    
    return ss.str();
}

std::string ReportGenerator::generateOrdersSection(
    const Warehouse& warehouse, 
    const AuxiliaryStructures& aux,
    const AuxiliaryStructures::OrderStatistics& orderStats) {
    
    std::stringstream ss;
    
    ss << "    <div class=\"section\">\n"
       << "        <h2>Análise de Pedidos</h2>\n";
    
    // Adicionar Análise Estatística Aplicada
    ss << "        <div class=\"subsection\">\n"
       << "            <h3>Insights Estatísticos</h3>\n"
       << "            <p>Baseado nas estatísticas, podemos identificar:</p>\n"
       << "            <ul>\n";
    
    if (orderStats.coefficientOfVariation > 0.5) {
        ss << "                <li><strong>Alta variabilidade</strong> na eficiência dos pedidos (CV = " 
           << std::fixed << std::setprecision(2) << orderStats.coefficientOfVariation 
           << "), indicando heterogeneidade que pode ser explorada.</li>\n";
    } else {
        ss << "                <li><strong>Baixa variabilidade</strong> na eficiência dos pedidos (CV = " 
           << std::fixed << std::setprecision(2) << orderStats.coefficientOfVariation 
           << "), indicando homogeneidade que pode simplificar o agrupamento.</li>\n";
    }
    
    if (orderStats.meanEfficiency > orderStats.medianEfficiency * 1.2) {
        ss << "                <li>Distribuição <strong>assimétrica à direita</strong> (média > mediana), "
           << "sugerindo poucos pedidos muito eficientes que deveriam ser priorizados.</li>\n";
    }
    
    // Adicionar sugestões para otimização
    ss << "                <li>Pedidos com eficiência acima de " 
       << std::fixed << std::setprecision(2) << orderStats.efficiencyQuantiles[2] 
       << " (Q3) são candidatos prioritários para seleção.</li>\n";
    
    ss << "            </ul>\n"
       << "        </div>\n";
    
    // Continuar com o código existente para a tabela de pedidos mais eficientes
    std::vector<std::string> headers = {"ID", "Itens Diferentes", "Itens Totais", "Corredores Necessários", "Eficiência (itens/corredores)", "Contribuição"};
    std::vector<std::vector<std::string>> rows;
    
    int maxRows = std::min(20, (int)aux.orderEfficiency.size());
    for (int i = 0; i < maxRows; i++) {
        int orderIdx = aux.orderEfficiency[i].first;
        double efficiency = aux.orderEfficiency[i].second;
        
        if (efficiency <= 0) continue; // Ignorar pedidos inviáveis
        
        std::vector<std::string> row;
        row.push_back(std::to_string(orderIdx));
        row.push_back(std::to_string(aux.numDiffItemsPerOrder[orderIdx]));
        row.push_back(std::to_string(aux.totalItemsPerOrder[orderIdx]));
        row.push_back(std::to_string(aux.numCorridorsNeededPerOrder[orderIdx]));
        
        std::stringstream effValue;
        effValue << std::fixed << std::setprecision(2) << efficiency;
        row.push_back(effValue.str());
        
        std::stringstream contribValue;
        contribValue << std::fixed << std::setprecision(2) << aux.weights.orderContributionScore[orderIdx];
        row.push_back(contribValue.str());
        
        rows.push_back(row);
    }
    
    ss << "        <h3>Top Pedidos por Eficiência</h3>\n";
    ss << createTable(headers, rows, "topOrdersTable");
    
    // Gráfico de distribuição de eficiência
    std::vector<std::string> labels;
    std::vector<double> values;
    
    // Definir intervalos para o histograma
    const int numBins = 10;
    std::vector<int> bins(numBins, 0);
    double maxEff = 0.0;
    
    for (const auto& [orderIdx, eff] : aux.orderEfficiency) {
        if (eff > maxEff) maxEff = eff;
    }
    
    // Criar histograma
    for (const auto& [orderIdx, eff] : aux.orderEfficiency) {
        if (eff <= 0) continue;
        int bin = std::min(numBins - 1, (int)((eff / maxEff) * numBins));
        bins[bin]++;
    }
    
    // Preparar dados para o gráfico
    for (int i = 0; i < numBins; i++) {
        double lowerBound = (i * maxEff) / numBins;
        double upperBound = ((i + 1) * maxEff) / numBins;
        
        std::stringstream labelSS;
        labelSS << std::fixed << std::setprecision(2) << lowerBound << "-" << upperBound;
        labels.push_back(labelSS.str());
        values.push_back(bins[i]);
    }
    
    ss << "        <h3>Distribuição de Eficiência dos Pedidos</h3>\n";
    ss << "        <div class=\"chart-container\">\n";
    ss << "            <canvas id=\"efficiencyDistChart\"></canvas>\n";
    ss << "        </div>\n";
    ss << createBarChart("efficiencyDistChart", "Distribuição de Eficiência", labels, values);
    
    ss << "    </div>\n";
    
    return ss.str();
}

std::string ReportGenerator::generateItemsSection(const Warehouse& warehouse, const AuxiliaryStructures& aux, const AuxiliaryStructures::ItemStatistics& itemStats) {
    std::stringstream ss;
    
    ss << "    <div class=\"section\">\n"
       << "        <h2>Análise de Itens</h2>\n";
    
    // Tabela dos top itens mais importantes
    std::vector<std::string> headers = {"Item ID", "Frequência", "Leverage Score", "Escassez (Demanda/Oferta)"};
    std::vector<std::vector<std::string>> rows;
    
    // Ordenar itens por leverage score
    std::vector<std::pair<int, double>> itemsByLeverage;
    for (int itemId : aux.allItems) {
        itemsByLeverage.push_back({itemId, aux.weights.itemLeverageScore[itemId]});
    }
    
    std::sort(itemsByLeverage.begin(), itemsByLeverage.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    int maxRows = std::min(20, (int)itemsByLeverage.size());
    for (int i = 0; i < maxRows; i++) {
        int itemId = itemsByLeverage[i].first;
        
        std::vector<std::string> row;
        row.push_back(std::to_string(itemId));
        row.push_back(std::to_string(aux.weights.itemFrequency[itemId]));
        
        std::stringstream leverageSS;
        leverageSS << std::fixed << std::setprecision(3) << aux.weights.itemLeverageScore[itemId];
        row.push_back(leverageSS.str());
        
        std::stringstream scarcitySS;
        scarcitySS << std::fixed << std::setprecision(2) << aux.weights.itemScarcityScore[itemId];
        row.push_back(scarcitySS.str());
        
        rows.push_back(row);
    }
    
    ss << "        <h3>Top Itens por Importância Estratégica</h3>\n";
    ss << createTable(headers, rows, "topItemsTable");
    
    // Gráfico de escassez dos itens
    std::vector<std::string> labels;
    std::vector<double> values;
    
    std::vector<std::pair<int, double>> itemsByScarcity;
    for (int itemId : aux.allItems) {
        itemsByScarcity.push_back({itemId, aux.weights.itemScarcityScore[itemId]});
    }
    
    std::sort(itemsByScarcity.begin(), itemsByScarcity.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    maxRows = std::min(15, (int)itemsByScarcity.size());
    for (int i = 0; i < maxRows; i++) {
        int itemId = itemsByScarcity[i].first;
        labels.push_back("Item " + std::to_string(itemId));
        values.push_back(aux.weights.itemScarcityScore[itemId]);
    }
    
    ss << "        <h3>Itens Mais Escassos (Demanda/Oferta)</h3>\n";
    ss << "        <div class=\"chart-container\">\n";
    ss << "            <canvas id=\"itemScarcityChart\"></canvas>\n";
    ss << "        </div>\n";
    ss << createBarChart("itemScarcityChart", "Escassez dos Itens", labels, values);
    
    ss << "    </div>\n";
    
    return ss.str();
}

std::string ReportGenerator::generateCorridorsSection(const Warehouse& warehouse, const AuxiliaryStructures& aux) {
    std::stringstream ss;
    
    ss << "    <div class=\"section\">\n"
       << "        <h2>Análise de Corredores</h2>\n";
    
    // Em uma implementação real, adicionaríamos informações sobre corredores aqui
    // Por exemplo, uma tabela mostrando quais corredores contêm mais itens importantes
    
    ss << "        <p>Esta seção mostraria análises detalhadas sobre os corredores, como:</p>\n"
       << "        <ul>\n"
       << "            <li>Corredores mais utilizados por pedidos eficientes</li>\n"
       << "            <li>Densidade de itens por corredor</li>\n"
       << "            <li>Visualização da matriz de cobertura corredor-item</li>\n"
       << "        </ul>\n";
    
    // Poderíamos criar um heatmap para visualizar a matriz de cobertura
    // mas isso seria mais complexo e requer mais implementação
    
    ss << "    </div>\n";
    
    return ss.str();
}

std::string ReportGenerator::generateMetricsSection(const AuxiliaryStructures& aux) {
    std::stringstream ss;
    
    ss << "    <div class=\"section\">\n"
       << "        <h2>Métricas e Estatísticas</h2>\n";
    
    // Aqui poderíamos adicionar mais métricas e estatísticas gerais
    // Por exemplo, a distribuição dos pedidos por tamanho, etc.
    
    ss << "        <p>Esta seção incluiria métricas adicionais e estatísticas globais da instância.</p>\n";
    
    ss << "    </div>\n";
    
    return ss.str();
}

std::string ReportGenerator::generateHtmlFooter() {
    std::stringstream ss;
    
    ss << "    <footer style=\"margin-top: 30px; text-align: center; color: #777;\">\n"
       << "        <p>Gerado pelo Otimizador de Wave - Mercado Livre</p>\n"
       << "    </footer>\n"
       << "</body>\n"
       << "</html>";
    
    return ss.str();
}

std::string ReportGenerator::createTable(const std::vector<std::string>& headers, 
                                       const std::vector<std::vector<std::string>>& rows,
                                       const std::string& tableId) {
    std::stringstream ss;
    
    ss << "        <table id=\"" << tableId << "\">\n"
       << "            <thead>\n"
       << "                <tr>\n";
    
    for (const auto& header : headers) {
        ss << "                    <th>" << header << "</th>\n";
    }
    
    ss << "                </tr>\n"
       << "            </thead>\n"
       << "            <tbody>\n";
    
    for (const auto& row : rows) {
        ss << "                <tr>\n";
        for (const auto& cell : row) {
            ss << "                    <td>" << cell << "</td>\n";
        }
        ss << "                </tr>\n";
    }
    
    ss << "            </tbody>\n"
       << "        </table>\n";
    
    return ss.str();
}

std::string ReportGenerator::createBarChart(const std::string& chartId, 
                                         const std::string& title,
                                         const std::vector<std::string>& labels, 
                                         const std::vector<double>& values) {
    std::stringstream ss;
    
    ss << "<script>\n"
       << "    document.addEventListener('DOMContentLoaded', function() {\n"
       << "        const ctx = document.getElementById('" << chartId << "').getContext('2d');\n"
       << "        new Chart(ctx, {\n"
       << "            type: 'bar',\n"
       << "            data: {\n"
       << "                labels: [";
    
    for (size_t i = 0; i < labels.size(); i++) {
        ss << "'" << labels[i] << "'";
        if (i < labels.size() - 1) ss << ", ";
    }
    
    ss << "],\n"
       << "                datasets: [{\n"
       << "                    label: '" << title << "',\n"
       << "                    data: [";
    
    for (size_t i = 0; i < values.size(); i++) {
        ss << values[i];
        if (i < values.size() - 1) ss << ", ";
    }
    
    ss << "],\n"
       << "                    backgroundColor: 'rgba(54, 162, 235, 0.5)',\n"
       << "                    borderColor: 'rgba(54, 162, 235, 1)',\n"
       << "                    borderWidth: 1\n"
       << "                }]\n"
       << "            },\n"
       << "            options: {\n"
       << "                responsive: true,\n"
       << "                maintainAspectRatio: false,\n"
       << "                scales: {\n"
       << "                    y: {\n"
       << "                        beginAtZero: true\n"
       << "                    }\n"
       << "                },\n"
       << "                plugins: {\n"
       << "                    title: {\n"
       << "                        display: true,\n"
       << "                        text: '" << title << "'\n"
       << "                    }\n"
       << "                }\n"
       << "            }\n"
       << "        });\n"
       << "    });\n"
       << "</script>";
    
    return ss.str();
}

std::string ReportGenerator::generateStatisticalSummary(
    const AuxiliaryStructures::OrderStatistics& orderStats,
    const AuxiliaryStructures::ItemStatistics& itemStats) {
    
    std::stringstream ss;
    
    ss << "    <div class=\"section\">\n"
       << "        <h2>Resumo Estatístico</h2>\n"
       
       << "        <div class=\"subsection\">\n"
       << "            <h3>Estatísticas de Pedidos</h3>\n"
       << "            <div class=\"stats-container\">\n"
       
       // Métricas básicas em caixas
       << "                <div class=\"metric-box\">\n"
       << "                    <div class=\"label\">Eficiência Média</div>\n"
       << "                    <div class=\"value\">" << std::fixed << std::setprecision(2) << orderStats.meanEfficiency << "</div>\n"
       << "                </div>\n"
       
       << "                <div class=\"metric-box\">\n"
       << "                    <div class=\"label\">Mediana</div>\n"
       << "                    <div class=\"value\">" << std::fixed << std::setprecision(2) << orderStats.medianEfficiency << "</div>\n"
       << "                </div>\n"
       
       << "                <div class=\"metric-box\">\n"
       << "                    <div class=\"label\">Desvio Padrão</div>\n"
       << "                    <div class=\"value\">" << std::fixed << std::setprecision(2) << orderStats.stdDevEfficiency << "</div>\n"
       << "                </div>\n"
       
       << "                <div class=\"metric-box\">\n"
       << "                    <div class=\"label\">Coef. de Variação</div>\n"
       << "                    <div class=\"value\">" << std::fixed << std::setprecision(2) << orderStats.coefficientOfVariation << "</div>\n"
       << "                </div>\n"
       << "            </div>\n"
       
       // Quartis em tabela
       << "            <h4>Quartis de Eficiência</h4>\n"
       << "            <table class=\"quartile-table\">\n"
       << "                <tr>\n"
       << "                    <th>Q1 (25%)</th>\n"
       << "                    <th>Q2 (50% - Mediana)</th>\n"
       << "                    <th>Q3 (75%)</th>\n"
       << "                </tr>\n"
       << "                <tr>\n";
    
    if (!orderStats.efficiencyQuantiles.empty() && orderStats.efficiencyQuantiles.size() >= 3) {
        ss << "                    <td>" << std::fixed << std::setprecision(2) << orderStats.efficiencyQuantiles[0] << "</td>\n"
           << "                    <td>" << std::fixed << std::setprecision(2) << orderStats.efficiencyQuantiles[1] << "</td>\n"
           << "                    <td>" << std::fixed << std::setprecision(2) << orderStats.efficiencyQuantiles[2] << "</td>\n";
    }
    
    ss << "                </tr>\n"
       << "            </table>\n"
       
       // Histograma de eficiência
       << "            <h4>Distribuição de Eficiência</h4>\n"
       << "            <div class=\"chart-container\">\n"
       << "                <canvas id=\"efficiencyHistogram\"></canvas>\n"
       << "            </div>\n";
    
    // Gerar dados para o histograma
    std::vector<std::string> histLabels;
    std::vector<double> histValues;
    
    for (size_t i = 0; i < orderStats.efficiencyBins.size() - 1; i++) {
        std::stringstream binLabel;
        binLabel << std::fixed << std::setprecision(2) 
                 << orderStats.efficiencyBins[i] << " - " 
                 << orderStats.efficiencyBins[i + 1];
        histLabels.push_back(binLabel.str());
        
        if (i < orderStats.efficiencyDistribution.size()) {
            histValues.push_back(orderStats.efficiencyDistribution[i]);
        } else {
            histValues.push_back(0);
        }
    }
    
    ss << createBarChart("efficiencyHistogram", "Distribuição de Eficiência dos Pedidos", histLabels, histValues)
       << "        </div>\n" // Fim da subsection de pedidos
       
       // Incluiria estatísticas de itens aqui
       << "    </div>\n"; // Fim da section
    
    return ss.str();
}
