#include "gestor_waves.h"

GestorWaves::GestorWaves(const Deposito& dep, const Backlog& back)
    : deposito(dep), backlog(back), 
      localizador(dep.numItens), 
      verificador(dep.numItens), 
      analisador(back.numPedidos) {
    
    // Construir as estruturas auxiliares
    localizador.construir(deposito);
    verificador.construir(deposito);
    
    // Usar o novo método calcularRelevancia para cada pedido em vez de construir
    for (int pedidoId = 0; pedidoId < backlog.numPedidos; pedidoId++) {
        if (verificador.verificarDisponibilidade(backlog.pedido[pedidoId])) {
            analisador.calcularRelevancia(pedidoId, backlog, localizador);
        }
    }
}

SeletorWaves::WaveCandidata GestorWaves::selecionarMelhorWave() {
    // Usar o novo método ordenarPedidos com a estratégia PARALELO
    std::vector<int> pedidosOrdenados = analisador.ordenarPedidos(AnalisadorRelevancia::EstrategiaOrdenacao::PARALELO);
    
    // CORREÇÃO: Usar o construtor sem parâmetros para SeletorWaves
    SeletorWaves seletor;
    
    // CORREÇÃO: Usar o método selecionarWaveOtima que existe na classe SeletorWaves
    return seletor.selecionarWaveOtima(backlog, pedidosOrdenados, analisador, localizador);
}

bool GestorWaves::verificarPedido(int pedidoId) {
    return verificador.verificarDisponibilidade(backlog.pedido[pedidoId]);
}

AnalisadorRelevancia::InfoPedido GestorWaves::getInfoPedido(int pedidoId) {
    return analisador.getInfoPedido(pedidoId);
}

const std::unordered_map<int, int>& GestorWaves::getCorredoresComItem(int itemId) {
    return localizador.getCorredoresComItem(itemId);
}