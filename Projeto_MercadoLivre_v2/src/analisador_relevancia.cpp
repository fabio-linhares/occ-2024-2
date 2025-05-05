#include "analisador_relevancia.h"
#include <algorithm> // Incluir <algorithm> para std::sort e std::max
#include <vector>    // Incluir <vector> para std::vector
#include <numeric>   // Incluir <numeric> para std::iota (se usado futuramente)
#include <execution> // Necessário para políticas de execução como std::execution::par
#include <iostream>  // Necessário para std::cerr

AnalisadorRelevancia::AnalisadorRelevancia(int numPedidos) : infoPedidos(numPedidos) {
    // Inicializa o vetor infoPedidos com valores padrão
    for (int i = 0; i < numPedidos; i++) {
        infoPedidos[i].pedidoId = i;
        infoPedidos[i].numItens = 0;
        infoPedidos[i].numUnidades = 0;
        infoPedidos[i].numCorredoresMinimo = 0;
        infoPedidos[i].pontuacaoRelevancia = 0.0; // Inicializa pontuação como 0
    }
}

void AnalisadorRelevancia::calcularRelevancia(int pedidoId, const Backlog& backlog, const LocalizadorItens& localizador) {
    // Verifica se o pedidoId é válido
    if (pedidoId < 0 || pedidoId >= infoPedidos.size() || pedidoId >= backlog.pedido.size()) {
        // Considerar lançar uma exceção ou logar um erro
        return;
    }

    InfoPedido& info = infoPedidos[pedidoId];
    info.pedidoId = pedidoId; // Garante que o ID está correto
    info.numItens = backlog.pedido[pedidoId].size();
    info.numUnidades = 0;
    info.corredoresNecessarios.clear(); // Limpa o set antes de preencher

    // Calcula o número total de unidades e os corredores necessários
    for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
        info.numUnidades += quantidade;
        // Verifica se itemId é válido antes de acessar localizador
        if (itemId >= 0 && itemId < localizador.itemParaCorredor.size()) {
            for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                info.corredoresNecessarios.insert(corredorId);
            }
        } else {
            // Logar um aviso ou erro sobre itemId inválido no pedido
        }
    }

    info.numCorredoresMinimo = info.corredoresNecessarios.size();

    // Calcula a pontuação de relevância (eficiência: unidades / corredores)
    // Evita divisão por zero se não houver corredores (embora raro se houver itens)
    if (info.numCorredoresMinimo > 0) {
        info.pontuacaoRelevancia = static_cast<double>(info.numUnidades) / info.numCorredoresMinimo;
    } else {
        info.pontuacaoRelevancia = 0.0; // Ou um valor muito alto se unidades > 0 e corredores = 0 for desejável
    }
}

std::vector<int> AnalisadorRelevancia::ordenarPorRelevancia() const {
    std::vector<int> pedidosOrdenados(infoPedidos.size());
    // Preenche o vetor com os IDs dos pedidos (0 a N-1)
    std::iota(pedidosOrdenados.begin(), pedidosOrdenados.end(), 0);

    // Ordena os IDs com base na pontuacaoRelevancia armazenada em infoPedidos
    // Usar std::execution::par requer C++17 e link com biblioteca de threads (como TBB).
    // Se não disponível, use std::sort sem o primeiro argumento.
    try {
        std::sort(
            // std::execution::par, // Remova ou comente se C++17/TBB não estiver configurado
             pedidosOrdenados.begin(), pedidosOrdenados.end(),
             [this](int a, int b) {
                 // Verifica limites para evitar acesso fora do vetor
                 if (a < 0 || a >= infoPedidos.size() || b < 0 || b >= infoPedidos.size()) {
                     return false; // Ou trate o erro de forma apropriada
                 }
                 // Ordena por relevância decrescente
                 return infoPedidos[a].pontuacaoRelevancia > infoPedidos[b].pontuacaoRelevancia;
             });
    } catch (const std::exception& e) {
        // Captura exceções que std::sort com política de execução pode lançar
        std::cerr << "Erro durante a ordenação paralela: " << e.what() << std::endl;
        // Fallback para ordenação sequencial
        std::sort(pedidosOrdenados.begin(), pedidosOrdenados.end(),
             [this](int a, int b) {
                 if (a < 0 || a >= infoPedidos.size() || b < 0 || b >= infoPedidos.size()) {
                     return false;
                 }
                 return infoPedidos[a].pontuacaoRelevancia > infoPedidos[b].pontuacaoRelevancia;
             });
    }

    return pedidosOrdenados;
}