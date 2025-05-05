#include "verificador_disponibilidade.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>
#include "localizador_itens.h" // Certifique-se de incluir este arquivo

// Build the total stock count per item
void VerificadorDisponibilidade::construir(const Deposito& deposito) {
    std::fill(estoqueTotal.begin(), estoqueTotal.end(), 0); // Reset stock
    for (int c = 0; c < deposito.numCorredores; ++c) {
        for (const auto& [itemId, quantidade] : deposito.corredor[c]) {
            if (itemId >= 0 && itemId < estoqueTotal.size()) {
                estoqueTotal[itemId] += quantidade;
            } else {
                // Handle invalid item ID if necessary
                std::cerr << "Aviso: Item ID " << itemId << " invalido no corredor " << c << std::endl;
            }
        }
    }
}

// Check if a single order is fulfillable based on total stock
bool VerificadorDisponibilidade::verificarDisponibilidade(const std::map<int, int>& pedido) const {
    for (const auto& [itemId, quantidade] : pedido) {
        if (itemId < 0 || itemId >= estoqueTotal.size()) {
            // std::cerr << "Erro: Pedido contem item ID invalido " << itemId << std::endl;
            return false; // Item doesn't exist
        }
        if (estoqueTotal[itemId] < quantidade) {
            // std::cout << "Debug: Item " << itemId << " indisponivel. Necessario: " << quantidade << ", Disponivel: " << estoqueTotal[itemId] << std::endl;
            return false; // Not enough stock
        }
    }
    return true;
}

// Check if a set of orders is simultaneously fulfillable based on total stock
bool VerificadorDisponibilidade::verificarDisponibilidadeConjunto(
    const std::vector<int>& pedidosIds,
    const Backlog& backlog) const {

    std::unordered_map<int, int> consumoTotalItem;

    // Aggregate consumption for all orders in the set
    for (int pedidoId : pedidosIds) {
         if (pedidoId < 0 || pedidoId >= backlog.numPedidos) {
              std::cerr << "Erro: ID de pedido invalido " << pedidoId << " em verificarDisponibilidadeConjunto" << std::endl;
              return false; // Invalid order ID
         }
        for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
            if (itemId < 0 || itemId >= estoqueTotal.size()) {
                 std::cerr << "Erro: Pedido " << pedidoId << " contem item ID invalido " << itemId << std::endl;
                 return false; // Invalid item ID
            }
            consumoTotalItem[itemId] += quantidade;
        }
    }

    // Check aggregated consumption against total stock
    for (const auto& [itemId, consumoTotal] : consumoTotalItem) {
        if (estoqueTotal[itemId] < consumoTotal) {
            // std::cout << "Debug: Conjunto inviavel. Item " << itemId << " necessario: " << consumoTotal << ", Disponivel: " << estoqueTotal[itemId] << std::endl;
            return false; // Not enough stock for the combined orders
        }
    }

    return true; // Adicionado return true que estava faltando
}


// --- Optional Functions (Implement if needed, otherwise remove) ---

// Attempts to repair an infeasible solution (e.g., due to stock or LB/UB)
std::vector<int> VerificadorDisponibilidade::repararSolucao(
    const std::vector<int>& pedidosWave,
    int LB,
    int UB,
    const Backlog& backlog, // Added backlog
    const LocalizadorItens& localizador // Added localizador
) const {
    // This is a complex heuristic and needs careful implementation.
    // Basic idea:
    // 1. Check current solution viability (stock, LB, UB).
    // 2. If stock infeasible: Iteratively remove orders causing conflicts (e.g., lowest BOV contribution) until feasible.
    // 3. If units < LB: Greedily add available orders (highest BOV contribution first) until LB is met or UB is hit, checking stock at each step.
    // 4. If units > UB: Greedily remove orders (lowest BOV contribution first) until UB is met.

    std::vector<int> pedidosAtuais = pedidosWave;
    int iteracoesReparo = 0;
    const int MAX_ITERACOES_REPARO = 5; // Limit repair attempts

    while (iteracoesReparo < MAX_ITERACOES_REPARO) {
        iteracoesReparo++;
        bool precisaReparo = false;
        int totalUnidades = 0;
        for(int pid : pedidosAtuais) {
            for(const auto& item : backlog.pedido[pid]) totalUnidades += item.second;
        }

        // Check 1: Stock
        if (!verificarDisponibilidadeConjunto(pedidosAtuais, backlog)) {
            precisaReparo = true;
            // TODO: Implement removal strategy for stock conflicts
            std::cerr << "Aviso: Reparo de conflito de estoque nao implementado." << std::endl;
            if (!pedidosAtuais.empty()) pedidosAtuais.pop_back(); // Simplistic removal
            continue; // Re-check after modification
        }

        // Check 2: LB
        if (totalUnidades < LB) {
            precisaReparo = true;
            // TODO: Implement adding strategy to meet LB
             std::cerr << "Aviso: Reparo para atingir LB nao implementado." << std::endl;
             // Try adding one random available order? Very basic.
             break; // Stop repair attempt for now
        }

        // Check 3: UB
        if (totalUnidades > UB) {
            precisaReparo = true;
            // TODO: Implement removal strategy to meet UB
             std::cerr << "Aviso: Reparo para respeitar UB nao implementado." << std::endl;
             if (!pedidosAtuais.empty()) pedidosAtuais.pop_back(); // Simplistic removal
             continue; // Re-check after modification
        }

        if (!precisaReparo) break; // Solution is now feasible
    }


    // Final check after repair attempts
    int finalUnidades = 0;
    for(int pid : pedidosAtuais) {
        for(const auto& item : backlog.pedido[pid]) finalUnidades += item.second;
    }
    if (finalUnidades < LB || finalUnidades > UB || !verificarDisponibilidadeConjunto(pedidosAtuais, backlog)) {
         std::cerr << "Erro: Reparo falhou em produzir solucao viavel." << std::endl;
         return {}; // Return empty if repair failed
    }


    std::cout << "Solucao reparada: " << pedidosAtuais.size() << " pedidos, "
              << finalUnidades << " unidades." << std::endl;

    return pedidosAtuais;
}


int VerificadorDisponibilidade::calcularNumCorredoresUnicos(
    const std::vector<int>& pedidosIds,
    const Backlog& backlog,
    const LocalizadorItens& localizador // Added localizador
) const {
    std::unordered_set<int> corredoresUnicos;
    for (int pedidoId : pedidosIds) {
        if (pedidoId >= 0 && pedidoId < backlog.numPedidos) {
            for (const auto& [itemId, _] : backlog.pedido[pedidoId]) {
                for (const auto& [corredorId, _] : localizador.getCorredoresComItem(itemId)) {
                    corredoresUnicos.insert(corredorId);
                }
            }
        }
    }
    return corredoresUnicos.size();
}

bool VerificadorDisponibilidade::verificarLimites(
    const std::vector<int>& pedidosIds,
    const Backlog& backlog, // Added backlog
    int LB,
    int UB) const {

    int totalUnidades = 0;
    for (int pedidoId : pedidosIds) {
         if (pedidoId >= 0 && pedidoId < backlog.numPedidos) {
             for (const auto& [itemId, quantidade] : backlog.pedido[pedidoId]) {
                 totalUnidades += quantidade;
             }
         } else {
             return false; // Invalid order ID
         }
    }

    return (totalUnidades >= LB && totalUnidades <= UB);
}