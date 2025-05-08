#include "core/solution.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <set>

// Implementação do construtor (se ainda não existir):
Solution::Solution() : totalItems(0), objectiveValue(0.0), feasible(false) {
    // Corpo do construtor (se houver lógica adicional)
}

void Solution::addOrder(int orderId, const Warehouse& warehouse) {
    if (!isOrderSelected(orderId)) {
        selectedOrders.push_back(orderId);
        
        // Atualizar total de itens
        for (const auto& item : warehouse.orders[orderId]) {
            totalItems += item.second;
        }
        
        // Não atualiza os corredores aqui para evitar recálculos constantes
        // Os corredores serão atualizados explicitamente quando necessário
    }
}

// Método para remover pedido (necessário para a estratégia de emergência)
void Solution::removeOrder(int order_id) {
    orders.erase(order_id);
}

void Solution::removeOrder(int orderId, const Warehouse& warehouse) {
    auto it = std::find(selectedOrders.begin(), selectedOrders.end(), orderId);
    if (it != selectedOrders.end()) {
        // Remover o pedido da lista
        selectedOrders.erase(it);

        // Atualizar total de itens (subtrair os itens do pedido removido)
        for (const auto& item : warehouse.orders[orderId]) {
            totalItems -= item.second;
        }

        // Atualizar corredores e valor objetivo (pode ser feito explicitamente depois ou aqui)
        // Para manter a consistência, é melhor recalcular
        updateCorridors(warehouse);
        calculateObjectiveValue(warehouse); // Recalcula o valor objetivo
    }
}

// Método para adicionar corredor à solução
void Solution::addVisitedCorridor(int corridor_id) {
    // Validação para evitar IDs negativos
    if (corridor_id < 0) {
        std::cerr << "AVISO: Tentativa de adicionar corredor com ID negativo: " 
                  << corridor_id << " (ignorando)" << std::endl;
        return;
    }
    
    corridors.insert(corridor_id);
}

void Solution::updateCorridors(const Warehouse& warehouse) {
    // Limpar corredores existentes
    visitedCorridors.clear();
    
    // Usar conjunto para evitar duplicatas
    std::set<int> corridorsSet;
    
    // Para cada pedido selecionado
    for (int orderId : selectedOrders) {
        // Para cada item no pedido
        for (const auto& item : warehouse.orders[orderId]) {
            int itemId = item.first;
            
            // Encontrar todos os corredores que contêm este item
            for (int c = 0; c < warehouse.numCorridors; c++) {
                for (const auto& corridorItem : warehouse.corridors[c]) {
                    if (corridorItem.first == itemId) {
                        corridorsSet.insert(c);
                        break;  // Encontrou um corredor com este item, não precisa continuar
                    }
                }
            }
        }
    }
    
    // Converter conjunto para vetor
    visitedCorridors.assign(corridorsSet.begin(), corridorsSet.end());
    
    std::cout << "    Corredores atualizados: " << visitedCorridors.size() << " corredores necessários." << std::endl;
}

double Solution::calculateObjectiveValue(const Warehouse& warehouse) {
    // Atualizar corredores se necessário
    if (visitedCorridors.empty() && !selectedOrders.empty()) {
        updateCorridors(warehouse);
    }
    
    // Calcular a razão (função objetivo)
    if (visitedCorridors.empty()) {
        objectiveValue = 0.0;  // Evitar divisão por zero
    } else {
        objectiveValue = static_cast<double>(totalItems) / visitedCorridors.size();
    }
    
    return objectiveValue;
}

bool Solution::isOrderSelected(int orderId) const {
    return std::find(selectedOrders.begin(), selectedOrders.end(), orderId) != selectedOrders.end();
}

void Solution::clear() {
    selectedOrders.clear();
    visitedCorridors.clear();
    totalItems = 0;
    objectiveValue = 0.0;
    // Não limpa auxiliaryDataMap para preservar estruturas auxiliares
}

bool Solution::isValid(const Warehouse& warehouse) const {
    // Verificar limite inferior
    if (totalItems < warehouse.LB) {
        std::cout << "Validação: total de itens (" << totalItems 
                  << ") abaixo do limite inferior (" << warehouse.LB << ")" << std::endl;
        return false;
    }
    
    // Verificar limite superior
    if (totalItems > warehouse.UB) {
        std::cout << "Validação: total de itens (" << totalItems 
                  << ") acima do limite superior (" << warehouse.UB << ")" << std::endl;
        return false;
    }
    
    // Verificar disponibilidade de estoque
    std::vector<int> estoqueUsado(warehouse.numItems, 0);
    for (int orderId : selectedOrders) {
        for (const auto& item : warehouse.orders[orderId]) {
            estoqueUsado[item.first] += item.second;
        }
    }
    
    // Calcular estoque disponível
    std::vector<int> estoqueDisponivel(warehouse.numItems, 0);
    for (int corridorId : visitedCorridors) {
        for (const auto& item : warehouse.corridors[corridorId]) {
            estoqueDisponivel[item.first] += item.second;
        }
    }
    
    // Verificar se o estoque é suficiente
    for (size_t i = 0; i < estoqueUsado.size(); i++) {
        if (estoqueUsado[i] > estoqueDisponivel[i]) {
            std::cout << "Validação: estoque insuficiente para o item " << i 
                      << " (necessário: " << estoqueUsado[i] 
                      << ", disponível: " << estoqueDisponivel[i] << ")" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool Solution::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo para gravação: " << filename << std::endl;
        return false;
    }
    
    // Eliminar duplicatas antes de salvar
    std::set<int> pedidosUnicos(selectedOrders.begin(), selectedOrders.end());
    std::vector<int> pedidosValidados(pedidosUnicos.begin(), pedidosUnicos.end());
    
    // Primeira linha: número de pedidos
    file << pedidosValidados.size() << std::endl;
    
    // Segunda linha: IDs dos pedidos separados por espaço
    for (size_t i = 0; i < pedidosValidados.size(); i++) {
        file << pedidosValidados[i];
        if (i < pedidosValidados.size() - 1) {
            file << " ";
        }
    }
    file << std::endl;
    
    file.close();
    return true;
}

bool Solution::loadFromFile(const std::string& filename, const Warehouse& warehouse) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo para leitura: " << filename << std::endl;
        return false;
    }
    
    clear();  // Limpar a solução atual
    
    // Primeira linha: número de pedidos
    int numPedidos;
    file >> numPedidos;
    
    // Segunda linha: IDs dos pedidos
    for (int i = 0; i < numPedidos; i++) {
        int orderId;
        if (file >> orderId) {
            // Verificar se ID é válido antes de adicionar
            if (orderId >= 0 && orderId < warehouse.numOrders) {
                addOrder(orderId, warehouse);
            } else {
                std::cerr << "ID de pedido inválido no arquivo: " << orderId << std::endl;
            }
        }
    }
    
    file.close();
    
    // Atualizar corredores e calcular função objetivo
    updateCorridors(warehouse);
    calculateObjectiveValue(warehouse);
    
    // Verificar se a solução é viável
    setFeasible(isValid(warehouse));
    
    return true;
}