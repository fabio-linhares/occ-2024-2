# Roteiro para Implementação do Desafio SBPO 2025 em C++

## 1. Avaliação da Infraestrutura Existente

### 1.1 Análise da Estrutura do Projeto
- Verificar as classes e interfaces existentes em include e src
- Revisar o sistema de build (CMake) e as dependências
- Analisar as estruturas de dados já definidas

### 1.2 Complementação das Estruturas
- Completar implementação ou estender as classes em:
  - core e core (Warehouse, Order, Item, Corridor)
  - algorithm e algorithm (OptimizationAlgorithm)

## 2. Implementação do Algoritmo de Otimização

### 2.1 Definir Classes de Algoritmo
- Implementar interfaces em optimization_algorithm.h
- Criar classes derivadas para algoritmos específicos:
  ```cpp
  // Exemplo para algoritmo guloso
  class GreedyAlgorithm : public OptimizationAlgorithm {
  public:
      Solution solve(const Warehouse& warehouse) override;
  private:
      // Métodos auxiliares específicos
  };
  ```

### 2.2 Algoritmo Guloso (Primeira Abordagem)
- Implementar em greedy_algorithm.cpp:
  - Ordenação de pedidos por densidade (itens/corredores)
  - Seleção incremental de pedidos
  - Verificação contínua das restrições

### 2.3 Função Objetivo
- Implementar avaliação da função objetivo:
  ```cpp
  double evaluateObjective(const Solution& solution, const Warehouse& warehouse);
  ```

### 2.4 Verificação de Restrições
- Implementar validadores para:
  - Limites de tamanho da wave (LB/UB)
  - Disponibilidade de itens nos corredores
  - Outras restrições operacionais

## 3. Classe Solution

### 3.1 Definição da Classe
- Implementar em solution.h:
  ```cpp
  class Solution {
  private:
      std::vector<int> selectedOrders;
      std::vector<int> visitedCorridors;
      double objectiveValue;
      bool feasible;
      
  public:
      // Construtores e métodos
      void addOrder(int orderId);
      void removeOrder(int orderId);
      void updateCorridors(const Warehouse& warehouse);
      // Getters/setters
  };
  ```

### 3.2 Métodos de Manipulação
- Implementar operações para:
  - Adicionar/remover pedidos
  - Calcular corredores necessários
  - Verificar viabilidade
  - Calcular valor da função objetivo

## 4. Integração com a Camada de I/O

### 4.1 Complementação do Parser
- Verificar implementação em input_parser.cpp
- Garantir leitura correta de:
  - Pedidos, itens e suas quantidades
  - Corredores e disponibilidade
  - Limites LB/UB

### 4.2 Geração da Saída
- Implementar em output_writer.cpp:
  - Formatação da solução conforme especificações
  - Escrita em arquivo no formato correto

## 5. Execução e Fluxo Principal

### 5.1 Implementação do main.cpp
- Fluxo principal:
  ```cpp
  int main(int argc, char* argv[]) {
      // Parse da linha de comando
      // Leitura da instância
      // Escolha e execução do algoritmo
      // Escrita da solução
      return 0;
  }
  ```

### 5.2 Parametrização
- Suporte para diferentes modos de execução:
  - Diferentes algoritmos
  - Leitura de parâmetros via linha de comando
  - Configuração a partir de arquivos

## 6. Testes e Validação

### 6.1 Testes Unitários
- Expandir os testes existentes em test
- Implementar testes para novas funcionalidades:
  - Teste da função objetivo
  - Teste da verificação de restrições
  - Teste da solução completa

### 6.2 Benchmark
- Criar testes de desempenho para diferentes algoritmos
- Validar em instâncias de diferentes tamanhos

## 7. Algoritmos Alternativos (Opcionais)

### 7.1 Simulated Annealing
- Implementar em `src/algorithm/simulated_annealing.cpp`:
  - Estrutura de vizinhança
  - Esquema de resfriamento
  - Critérios de aceitação

### 7.2 Algoritmo Genético
- Implementar em `src/algorithm/genetic_algorithm.cpp`:
  - Representação de cromossomos
  - Operadores de cruzamento e mutação
  - Função de fitness

### 7.3 Busca Tabu
- Implementar em `src/algorithm/tabu_search.cpp`:
  - Lista tabu e critérios de aspiração
  - Estruturas de vizinhança
  - Critérios de parada

## 8. Otimização e Refinamento

### 8.1 Perfil de Desempenho
- Identificar gargalos com ferramentas como gprof
- Otimizar seções críticas do código

### 8.2 Validação Cruzada
- Comparar resultados com diferentes algoritmos
- Analisar trade-offs entre qualidade e tempo de execução

Este roteiro reflete a estrutura existente do projeto em C++ e estabelece um caminho claro para implementar a solução completa para o desafio.