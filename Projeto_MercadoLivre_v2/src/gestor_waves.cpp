#include "gestor_waves.h"

GestorWaves::GestorWaves(const Deposito& dep, const Backlog& back) 
    : deposito(dep), 
      backlog(back),
      localizador(dep.numItens),
      verificador(dep.numItens),
      analisador(back.numPedidos) {
    
    // Inicializar estruturas auxiliares
    localizador.construir(deposito);
    verificador.construir(deposito);
    analisador.construir(backlog, localizador);
}

SeletorWaves::WaveCandidata GestorWaves::selecionarMelhorWave() {
    std::vector<int> pedidosOrdenados = analisador.getPedidosOrdenadosPorRelevancia();
    return seletor.selecionarWaveOtima(backlog, pedidosOrdenados, analisador, localizador);
}

bool GestorWaves::verificarPedido(int pedidoId) {
    return verificador.verificarDisponibilidade(backlog.pedido[pedidoId]);
}

AnalisadorRelevancia::InfoPedido GestorWaves::getInfoPedido(int pedidoId) {
    return analisador.infoPedidos[pedidoId];
}

const std::unordered_map<int, int>& GestorWaves::getCorredoresComItem(int itemId) {
    return localizador.getCorredoresComItem(itemId);
}