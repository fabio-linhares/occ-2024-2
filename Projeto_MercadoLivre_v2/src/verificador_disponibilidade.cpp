#include "verificador_disponibilidade.h"

void VerificadorDisponibilidade::construir(const Deposito& deposito) {
    for (int corredorId = 0; corredorId < deposito.numCorredores; corredorId++) {
        for (const auto& [itemId, quantidade] : deposito.corredor[corredorId]) {
            estoqueTotal[itemId] += quantidade;
        }
    }
}

bool VerificadorDisponibilidade::verificarDisponibilidade(const std::map<int, int>& pedido) const {
    for (const auto& [itemId, quantidadeSolicitada] : pedido) {
        if (estoqueTotal[itemId] < quantidadeSolicitada) {
            return false;
        }
    }
    return true;
}