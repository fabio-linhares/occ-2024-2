#pragma once

#include <string>
#include <map>
#include <vector>
#include <cmath>  // Adicione este include
#include "problema.h"

// Estrutura para armazenar configurações dinâmicas
struct RestricoesConfig {
    // Configurações de tempo
    int limite_tempo_total_ms = 600000; // 10 minutos padrão
    int limite_tempo_instancia_ms = 120000; // 2 minutos padrão
    int margem_seguranca_ms = 30000; // 30 segundos padrão
    
    // Configurações de limites
    double lb_multiplicador = 1.0; // Não altera o LB por padrão
    double ub_multiplicador = 1.0; // Não altera o UB por padrão
    
    // Configurações de algoritmo
    bool verificar_disponibilidade = true; 
    int max_iteracoes_busca_local = 100;
    double intensidade_perturbacao = 0.3;
    int max_iteracoes_perturbacao = 20;
    
    // Thresholds para seleção de algoritmo baseado no tamanho
    int threshold_problemas_pequenos = 15; 
    int threshold_problemas_medios = 50;
    
    // Fração de tempo para diferentes algoritmos
    double fracao_tempo_busca_local = 0.3;
    
    // Modificar o problema com base nas configurações
    void aplicarAoProblema(Problema& problema) const {
        // Aplicar multiplicadores a LB e UB
        if (lb_multiplicador != 1.0 && problema.LB > 0) {
            int lb_original = problema.LB;
            problema.LB = static_cast<int>(std::ceil(lb_original * lb_multiplicador));
            std::cout << "LB ajustado: " << lb_original << " -> " << problema.LB << std::endl;
        }
        
        if (ub_multiplicador != 1.0 && problema.UB > 0) {
            int ub_original = problema.UB;
            problema.UB = static_cast<int>(std::floor(ub_original * ub_multiplicador));
            std::cout << "UB ajustado: " << ub_original << " -> " << problema.UB << std::endl;
        }
    }
};

// Em restricoes.h
RestricoesConfig carregarRestricoesConfig(const std::string& caminho = "restricoes.txt");