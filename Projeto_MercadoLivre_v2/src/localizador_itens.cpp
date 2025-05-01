#include "localizador_itens.h"

void LocalizadorItens::construir(const Deposito& deposito) {
    for (int corredorId = 0; corredorId < deposito.numCorredores; corredorId++) {
        for (const auto& [itemId, quantidade] : deposito.corredor[corredorId]) {
            itemParaCorredor[itemId][corredorId] = quantidade;
        }
    }
}

const std::unordered_map<int, int>& LocalizadorItens::getCorredoresComItem(int itemId) const {
    return itemParaCorredor[itemId];
}