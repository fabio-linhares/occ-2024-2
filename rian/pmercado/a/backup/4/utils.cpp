#include "utils.h"
#include <iostream>
#include <fstream>
#include <iomanip> // Para formatação da saída

void gerarRelatorioDetalhado(const std::vector<ResultadoInstancia>& resultados, long long tempo_total_ms) {
    std::ofstream arquivo_relatorio("relatorio_detalhado.txt"); // Nome do arquivo de relatório

    if (!arquivo_relatorio.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de relatório." << std::endl;
        return;
    }

    arquivo_relatorio << "Relatório Detalhado das Instâncias" << std::endl;
    arquivo_relatorio << "----------------------------------" << std::endl;
    arquivo_relatorio << "Tempo total de execução: " << tempo_total_ms << " ms" << std::endl;
    arquivo_relatorio << std::endl;

    for (const auto& resultado : resultados) {
        // Implementação existente...
    }

    arquivo_relatorio.close();
    std::cout << "Relatório detalhado gerado em relatorio_detalhado.txt" << std::endl;
}

// Removida a implementação duplicada de next_combination