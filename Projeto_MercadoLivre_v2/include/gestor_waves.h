#pragma once

#include "armazem.h"
#include "localizador_itens.h"
#include "verificador_disponibilidade.h"
#include "analisador_relevancia.h"
#include "seletor_waves.h"

/**
 * @brief Classe para gestão integrada de waves
 */
class GestorWaves {
private:
    Deposito deposito;
    Backlog backlog;
    LocalizadorItens localizador;
    VerificadorDisponibilidade verificador;
    AnalisadorRelevancia analisador;
    SeletorWaves seletor;
    
public:
    /**
     * @brief Construtor
     * @param dep Referência ao objeto Deposito
     * @param back Referência ao objeto Backlog
     */
    GestorWaves(const Deposito& dep, const Backlog& back);
    
    /**
     * @brief Seleciona a melhor wave possível
     * @return Uma WaveCandidata contendo a melhor seleção de pedidos
     */
    SeletorWaves::WaveCandidata selecionarMelhorWave();
    
    /**
     * @brief Verifica se um pedido específico pode ser atendido
     * @param pedidoId ID do pedido a ser verificado
     * @return true se o pedido pode ser atendido, false caso contrário
     */
    bool verificarPedido(int pedidoId);
    
    /**
     * @brief Obtém informações de relevância para um pedido
     * @param pedidoId ID do pedido
     * @return Estrutura InfoPedido com informações de relevância
     */
    AnalisadorRelevancia::InfoPedido getInfoPedido(int pedidoId);
    
    /**
     * @brief Obtém corredores que contêm um item específico
     * @param itemId ID do item
     * @return Mapa de corredores e suas quantidades disponíveis
     */
    const std::unordered_map<int, int>& getCorredoresComItem(int itemId);
};