#include "controle.h"
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <ctime>
#include <sstream>

// Função auxiliar para gerar a data atual como string
std::string obterDataAtual() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
    return oss.str();
}

// Calcular o total de itens em uma solução
int calcularTotalItens(const Solucao& solucao, const Problema& problema) {
    int total = 0;
    for (int pedido_id : solucao.pedidos_atendidos) {
        for (const auto& item_info : problema.pedidos[pedido_id].itens) {
            total += item_info.second;
        }
    }
    return total;
}

// Registrar desempenho atual e salvar no histórico
void registrarDesempenho(const std::vector<ResultadoInstancia>& resultados, const std::vector<Problema>& problemas) {
    if (resultados.size() != problemas.size()) {
        std::cerr << "Erro: Número de resultados e problemas diferentes" << std::endl;
        return;
    }
    
    std::vector<MetricasDesempenho> metricas;
    std::string data = obterDataAtual();
    
    for (size_t i = 0; i < resultados.size(); ++i) {
        const auto& resultado = resultados[i];
        const auto& problema = problemas[i];
        
        int total_itens = calcularTotalItens(resultado.solucao, problema);
        double razao = resultado.solucao.corredores_utilizados.empty() ? 0 : 
                      static_cast<double>(total_itens) / resultado.solucao.corredores_utilizados.size();
        
        MetricasDesempenho metrica = {
            resultado.nome_instancia,
            static_cast<int>(resultado.solucao.pedidos_atendidos.size()),
            static_cast<int>(resultado.solucao.corredores_utilizados.size()),
            total_itens,
            razao,
            resultado.tempo_execucao_ms,
            "GRASP",  // Ajustar conforme o algoritmo utilizado
            data
        };
        
        metricas.push_back(metrica);
    }
    
    // Salvar histórico
    salvarHistoricoDesempenho(metricas);
    
    // Gerar relatório para esta execução
    gerarRelatorioExecucao(metricas, data);
}

// Gerar estatísticas consolidadas
EstatisticasConsolidadas calcularEstatisticas(const std::vector<MetricasDesempenho>& metricas) {
    if (metricas.empty()) {
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }
    
    std::vector<double> razoes;
    std::vector<long long> tempos;
    std::vector<int> pedidos;
    std::vector<int> corredores;
    
    for (const auto& m : metricas) {
        razoes.push_back(m.razao_itens_corredor);
        tempos.push_back(m.tempo_execucao_ms);
        pedidos.push_back(m.num_pedidos_atendidos);
        corredores.push_back(m.num_corredores_utilizados);
    }
    
    auto razao_min_max = std::minmax_element(razoes.begin(), razoes.end());
    auto tempo_min_max = std::minmax_element(tempos.begin(), tempos.end());
    auto pedidos_min_max = std::minmax_element(pedidos.begin(), pedidos.end());
    auto corredores_min_max = std::minmax_element(corredores.begin(), corredores.end());
    
    double razao_media = std::accumulate(razoes.begin(), razoes.end(), 0.0) / razoes.size();
    double tempo_medio = std::accumulate(tempos.begin(), tempos.end(), 0.0) / tempos.size();
    double pedidos_medio = std::accumulate(pedidos.begin(), pedidos.end(), 0.0) / pedidos.size();
    double corredores_medio = std::accumulate(corredores.begin(), corredores.end(), 0.0) / corredores.size();
    
    return {
        *razao_min_max.first,
        *razao_min_max.second,
        razao_media,
        static_cast<double>(*tempo_min_max.first),
        static_cast<double>(*tempo_min_max.second),
        tempo_medio,
        *pedidos_min_max.first,
        *pedidos_min_max.second,
        pedidos_medio,
        *corredores_min_max.first,
        *corredores_min_max.second,
        corredores_medio
    };
}

// Exibir estatísticas no terminal
void exibirEstatisticasTerminal(const std::vector<ResultadoInstancia>& resultados, const std::vector<Problema>& problemas) {
    std::vector<MetricasDesempenho> metricas;
    std::string data = obterDataAtual();
    
    for (size_t i = 0; i < resultados.size(); ++i) {
        const auto& resultado = resultados[i];
        const auto& problema = problemas[i];
        
        int total_itens = calcularTotalItens(resultado.solucao, problema);
        double razao = resultado.solucao.corredores_utilizados.empty() ? 0 : 
                      static_cast<double>(total_itens) / resultado.solucao.corredores_utilizados.size();
        
        MetricasDesempenho metrica = {
            resultado.nome_instancia,
            static_cast<int>(resultado.solucao.pedidos_atendidos.size()),
            static_cast<int>(resultado.solucao.corredores_utilizados.size()),
            total_itens,
            razao,
            resultado.tempo_execucao_ms,
            "GRASP",  // Ajustar conforme o algoritmo utilizado
            data
        };
        
        metricas.push_back(metrica);
    }
    
    auto stats = calcularEstatisticas(metricas);
    
    std::cout << "\n========== ESTATÍSTICAS DA EXECUÇÃO ==========\n";
    std::cout << "Data: " << data << "\n\n";
    
    std::cout << "RAZÃO ITENS/CORREDORES:\n";
    std::cout << "  Mínima: " << std::fixed << std::setprecision(2) << stats.razao_min << "\n";
    std::cout << "  Máxima: " << std::fixed << std::setprecision(2) << stats.razao_max << "\n";
    std::cout << "  Média:  " << std::fixed << std::setprecision(2) << stats.razao_media << "\n\n";
    
    std::cout << "TEMPO DE EXECUÇÃO (ms):\n";
    std::cout << "  Mínimo: " << std::fixed << std::setprecision(2) << stats.tempo_min << "\n";
    std::cout << "  Máximo: " << std::fixed << std::setprecision(2) << stats.tempo_max << "\n";
    std::cout << "  Médio:  " << std::fixed << std::setprecision(2) << stats.tempo_medio << "\n\n";
    
    std::cout << "PEDIDOS ATENDIDOS:\n";
    std::cout << "  Mínimo: " << stats.pedidos_min << "\n";
    std::cout << "  Máximo: " << stats.pedidos_max << "\n";
    std::cout << "  Médio:  " << std::fixed << std::setprecision(2) << stats.pedidos_medio << "\n\n";
    
    std::cout << "CORREDORES UTILIZADOS:\n";
    std::cout << "  Mínimo: " << stats.corredores_min << "\n";
    std::cout << "  Máximo: " << stats.corredores_max << "\n";
    std::cout << "  Médio:  " << std::fixed << std::setprecision(2) << stats.corredores_medio << "\n";
    
    std::cout << "============================================\n\n";
    
    // Exibir detalhes por instância
    std::cout << "DETALHES POR INSTÂNCIA:\n";
    std::cout << std::left << std::setw(15) << "Instância" 
              << std::setw(10) << "Pedidos" 
              << std::setw(12) << "Corredores" 
              << std::setw(10) << "Itens" 
              << std::setw(10) << "Razão" 
              << std::setw(10) << "Tempo(ms)" << "\n";
    std::cout << std::string(67, '-') << "\n";
    
    for (const auto& m : metricas) {
        std::cout << std::left << std::setw(15) << m.nome_instancia 
                  << std::setw(10) << m.num_pedidos_atendidos 
                  << std::setw(12) << m.num_corredores_utilizados 
                  << std::setw(10) << m.total_itens 
                  << std::setw(10) << std::fixed << std::setprecision(2) << m.razao_itens_corredor 
                  << std::setw(10) << m.tempo_execucao_ms << "\n";
    }
}

// Salvar histórico de desempenho em arquivo CSV
void salvarHistoricoDesempenho(const std::vector<MetricasDesempenho>& metricas) {
    std::string arquivo = "historico_desempenho.csv";
    bool arquivo_existe = std::ifstream(arquivo).good();
    
    std::ofstream out(arquivo, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Erro ao abrir arquivo de histórico: " << arquivo << std::endl;
        return;
    }
    
    // Escrever cabeçalho se for um novo arquivo
    if (!arquivo_existe) {
        out << "Data,Instância,Pedidos,Corredores,Itens,Razão,Tempo(ms),Algoritmo\n";
    }
    
    // Escrever dados
    for (const auto& m : metricas) {
        out << m.data_execucao << ","
            << m.nome_instancia << ","
            << m.num_pedidos_atendidos << ","
            << m.num_corredores_utilizados << ","
            << m.total_itens << ","
            << std::fixed << std::setprecision(2) << m.razao_itens_corredor << ","
            << m.tempo_execucao_ms << ","
            << m.algoritmo_utilizado << "\n";
    }
    
    out.close();
}

// Gerar relatório completo para esta execução
void gerarRelatorioCompleto(const std::vector<ResultadoInstancia>& resultados, const std::vector<Problema>& problemas) {
    std::vector<MetricasDesempenho> metricas;
    std::string data = obterDataAtual();
    
    for (size_t i = 0; i < resultados.size(); ++i) {
        const auto& resultado = resultados[i];
        const auto& problema = problemas[i];
        
        int total_itens = calcularTotalItens(resultado.solucao, problema);
        double razao = resultado.solucao.corredores_utilizados.empty() ? 0 : 
                      static_cast<double>(total_itens) / resultado.solucao.corredores_utilizados.size();
        
        MetricasDesempenho metrica = {
            resultado.nome_instancia,
            static_cast<int>(resultado.solucao.pedidos_atendidos.size()),
            static_cast<int>(resultado.solucao.corredores_utilizados.size()),
            total_itens,
            razao,
            resultado.tempo_execucao_ms,
            "GRASP",  // Ajustar conforme o algoritmo utilizado
            data
        };
        
        metricas.push_back(metrica);
    }
    
    auto stats = calcularEstatisticas(metricas);
    
    std::string arquivo = "relatorio_" + data + ".txt";
    std::replace(arquivo.begin(), arquivo.end(), ':', '-');
    std::replace(arquivo.begin(), arquivo.end(), ' ', '_');
    
    std::ofstream out(arquivo);
    if (!out.is_open()) {
        std::cerr << "Erro ao criar arquivo de relatório: " << arquivo << std::endl;
        return;
    }
    
    out << "RELATÓRIO DE DESEMPENHO - " << data << "\n";
    out << std::string(50, '=') << "\n\n";
    
    out << "ESTATÍSTICAS CONSOLIDADAS:\n";
    out << std::string(25, '-') << "\n";
    
    out << "RAZÃO ITENS/CORREDORES:\n";
    out << "  Mínima: " << std::fixed << std::setprecision(2) << stats.razao_min << "\n";
    out << "  Máxima: " << std::fixed << std::setprecision(2) << stats.razao_max << "\n";
    out << "  Média:  " << std::fixed << std::setprecision(2) << stats.razao_media << "\n\n";
    
    out << "TEMPO DE EXECUÇÃO (ms):\n";
    out << "  Mínimo: " << std::fixed << std::setprecision(2) << stats.tempo_min << "\n";
    out << "  Máximo: " << std::fixed << std::setprecision(2) << stats.tempo_max << "\n";
    out << "  Médio:  " << std::fixed << std::setprecision(2) << stats.tempo_medio << "\n\n";
    
    out << "PEDIDOS ATENDIDOS:\n";
    out << "  Mínimo: " << stats.pedidos_min << "\n";
    out << "  Máximo: " << stats.pedidos_max << "\n";
    out << "  Médio:  " << std::fixed << std::setprecision(2) << stats.pedidos_medio << "\n\n";
    
    out << "CORREDORES UTILIZADOS:\n";
    out << "  Mínimo: " << stats.corredores_min << "\n";
    out << "  Máximo: " << stats.corredores_max << "\n";
    out << "  Médio:  " << std::fixed << std::setprecision(2) << stats.corredores_medio << "\n\n";
    
    out << "DETALHES POR INSTÂNCIA:\n";
    out << std::string(25, '-') << "\n";
    out << std::left << std::setw(15) << "Instância" 
              << std::setw(10) << "Pedidos" 
              << std::setw(12) << "Corredores" 
              << std::setw(10) << "Itens" 
              << std::setw(10) << "Razão" 
              << std::setw(10) << "Tempo(ms)" << "\n";
    out << std::string(67, '-') << "\n";
    
    for (const auto& m : metricas) {
        out << std::left << std::setw(15) << m.nome_instancia 
                  << std::setw(10) << m.num_pedidos_atendidos 
                  << std::setw(12) << m.num_corredores_utilizados 
                  << std::setw(10) << m.total_itens 
                  << std::setw(10) << std::fixed << std::setprecision(2) << m.razao_itens_corredor 
                  << std::setw(10) << m.tempo_execucao_ms << "\n";
    }
    
    out.close();
    std::cout << "Relatório detalhado gerado em " << arquivo << std::endl;
}

// Gerar relatório para esta execução
void gerarRelatorioExecucao(const std::vector<MetricasDesempenho>& metricas, const std::string& data) {
    auto stats = calcularEstatisticas(metricas);
    
    std::string arquivo = "relatorio_" + data + ".txt";
    std::replace(arquivo.begin(), arquivo.end(), ':', '-');
    std::replace(arquivo.begin(), arquivo.end(), ' ', '_');
    
    std::ofstream out(arquivo);
    if (!out.is_open()) {
        std::cerr << "Erro ao criar arquivo de relatório: " << arquivo << std::endl;
        return;
    }
    
    out << "RELATÓRIO DE DESEMPENHO - " << data << "\n";
    out << std::string(50, '=') << "\n\n";
    
    out << "ESTATÍSTICAS CONSOLIDADAS:\n";
    out << std::string(25, '-') << "\n";
    
    out << "RAZÃO ITENS/CORREDORES:\n";
    out << "  Mínima: " << std::fixed << std::setprecision(2) << stats.razao_min << "\n";
    out << "  Máxima: " << std::fixed << std::setprecision(2) << stats.razao_max << "\n";
    out << "  Média:  " << std::fixed << std::setprecision(2) << stats.razao_media << "\n\n";
    
    out << "TEMPO DE EXECUÇÃO (ms):\n";
    out << "  Mínimo: " << std::fixed << std::setprecision(2) << stats.tempo_min << "\n";
    out << "  Máximo: " << std::fixed << std::setprecision(2) << stats.tempo_max << "\n";
    out << "  Médio:  " << std::fixed << std::setprecision(2) << stats.tempo_medio << "\n\n";
    
    out << "PEDIDOS ATENDIDOS:\n";
    out << "  Mínimo: " << stats.pedidos_min << "\n";
    out << "  Máximo: " << stats.pedidos_max << "\n";
    out << "  Médio:  " << std::fixed << std::setprecision(2) << stats.pedidos_medio << "\n\n";
    
    out << "CORREDORES UTILIZADOS:\n";
    out << "  Mínimo: " << stats.corredores_min << "\n";
    out << "  Máximo: " << stats.corredores_max << "\n";
    out << "  Médio:  " << std::fixed << std::setprecision(2) << stats.corredores_medio << "\n\n";
    
    out << "DETALHES POR INSTÂNCIA:\n";
    out << std::string(25, '-') << "\n";
    out << std::left << std::setw(15) << "Instância" 
              << std::setw(10) << "Pedidos" 
              << std::setw(12) << "Corredores" 
              << std::setw(10) << "Itens" 
              << std::setw(10) << "Razão" 
              << std::setw(10) << "Tempo(ms)" << "\n";
    out << std::string(67, '-') << "\n";
    
    for (const auto& m : metricas) {
        out << std::left << std::setw(15) << m.nome_instancia 
                  << std::setw(10) << m.num_pedidos_atendidos 
                  << std::setw(12) << m.num_corredores_utilizados 
                  << std::setw(10) << m.total_itens 
                  << std::setw(10) << std::fixed << std::setprecision(2) << m.razao_itens_corredor 
                  << std::setw(10) << m.tempo_execucao_ms << "\n";
    }
    
    out.close();
    std::cout << "Relatório detalhado gerado em " << arquivo << std::endl;
}