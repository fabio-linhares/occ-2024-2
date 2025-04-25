#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include "input/input_parser.h"
#include "core/solution.h"
#include "modules/cria_auxiliares.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

class ReportGenerator {
public:
    // Gerar relatório para uma instância específica
    static bool generateReport(const std::string& instanceFile, const std::string& outputPath);
    
    // Gerar relatório a partir de uma warehouse e solution já processadas
    static bool generateReportFromProcessedData(const Warehouse& warehouse, 
                                               const Solution& solution,
                                               const std::string& outputPath);

private:
    // Métodos auxiliares para geração do relatório
    static std::string generateHtmlHeader(const Warehouse& warehouse);
    static std::string generateInstanceSummary(const Warehouse& warehouse);
    static std::string generateOrdersSection(const Warehouse& warehouse, const AuxiliaryStructures& aux);
    static std::string generateItemsSection(const Warehouse& warehouse, const AuxiliaryStructures& aux);
    static std::string generateCorridorsSection(const Warehouse& warehouse, const AuxiliaryStructures& aux);
    static std::string generateMetricsSection(const AuxiliaryStructures& aux);
    static std::string generateHtmlFooter();
    
    // Helpers para formatar os dados
    static std::string createTable(const std::vector<std::string>& headers, 
                                 const std::vector<std::vector<std::string>>& rows,
                                 const std::string& tableId);
    
    static std::string createBarChart(const std::string& chartId, 
                                   const std::string& title,
                                   const std::vector<std::string>& labels, 
                                   const std::vector<double>& values);
    
    static std::string createHeatmap(const std::string& chartId,
                                  const std::string& title,
                                  const std::vector<std::string>& xLabels,
                                  const std::vector<std::string>& yLabels,
                                  const std::vector<std::vector<double>>& data);
};

#endif // REPORT_GENERATOR_H