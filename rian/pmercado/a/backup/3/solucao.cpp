#include "solucao.h"

void gerarSaida(const std::string& caminho_saida, const Solucao& solucao) {
    std::ofstream arquivo_saida(caminho_saida);

    if (!arquivo_saida.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de saída: " << caminho_saida << std::endl;
        return;
    }

    // Número de pedidos selecionados
    arquivo_saida << solucao.pedidos_atendidos.size() << std::endl;
    
    // Índices dos pedidos selecionados
    for (size_t i = 0; i < solucao.pedidos_atendidos.size(); ++i) {
        arquivo_saida << solucao.pedidos_atendidos[i];
        if (i < solucao.pedidos_atendidos.size() - 1) {
            arquivo_saida << " ";
        }
    }
    arquivo_saida << std::endl;
    
    // Número de corredores visitados e seus índices
    arquivo_saida << solucao.corredores_utilizados.size() << std::endl;
    for (size_t i = 0; i < solucao.corredores_utilizados.size(); ++i) {
        arquivo_saida << solucao.corredores_utilizados[i];
        if (i < solucao.corredores_utilizados.size() - 1) {
            arquivo_saida << " ";
        }
    }
    
    arquivo_saida.close();
}