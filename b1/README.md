# Meta-heurística para o Problema de Bin Packing

## Descrição do Problema

O problema do Bin Packing (BP) é um problema clássico da Otimização Combinatória e da Teoria da Computação, pertencente à classe de problemas NP-difíceis. Consiste em distribuir um conjunto de itens em um número mínimo de recipientes (bins), respeitando a capacidade máxima de cada recipiente (unitária).

### Definição Formal
- **Entrada**: Conjunto U com n itens, em que cada item u ∈ U possui um tamanho s_u tal que 0 ≤ s_u ≤ 1.
- **Objetivo**: Encontrar uma partição de U em k conjuntos disjuntos U₁, ..., Uₖ de forma que para todo i, a soma de todos os itens em cada Uᵢ não exceda 1, e que o valor de k seja minimizado.

## Implementação da Meta-heurística

Esta implementação consiste em uma meta-heurística de solução única baseada em busca local para resolver instâncias do problema de Bin Packing.

### Componentes da Solução

#### 1. Representação da Solução
A solução é representada por um vetor de bins, onde cada bin contém um conjunto de itens. Cada item possui um tamanho associado, e a soma dos tamanhos dos itens em um bin não pode exceder a capacidade unitária.

```
Solução = [Bin₁, Bin₂, ..., Binₖ]
Onde cada Binᵢ = {item₁, item₂, ..., itemⱼ}
```

#### 2. Função de Avaliação
A função objetivo principal é minimizar o número de bins utilizados. Adicionalmente, para guiar a busca local, utilizamos uma função de avaliação que considera:

- Número total de bins utilizados (componente principal)
- Balanceamento de carga dos bins (componente secundário)

A função de avaliação pode ser expressa como:

```
f(s) = k + α * Σ(1 - ocupação(Binᵢ))²
```

Onde:
- k é o número de bins utilizados
- α é um parâmetro de penalização 
- ocupação(Binᵢ) é a soma dos tamanhos dos itens no Binᵢ

#### 3. Estratégia de Busca Local
A busca local é implementada usando dois tipos de movimentos:

- **Movimento de Troca**: Trocar itens entre dois bins diferentes
- **Movimento de Realocação**: Mover um item de um bin para outro

A estratégia implementada é a de "best improvement", onde todos os movimentos possíveis são avaliados e o melhor é selecionado para aplicação.

Pseudocódigo da busca local:
```
função BuscaLocal(solução):
    melhorou = verdadeiro
    enquanto melhorou:
        melhorou = falso
        para cada par de bins (i, j):
            para cada movimento possível m em (i, j):
                nova_solução = aplicar_movimento(solução, m)
                se f(nova_solução) < f(solução):
                    solução = nova_solução
                    melhorou = verdadeiro
                    se movimento reduziu número de bins:
                        continuar
    retornar solução
```

#### 4. Critério de Parada
O algoritmo utiliza como critério de parada um tempo limite em segundos, recebido via linha de comando. Quando o tempo limite é atingido, a melhor solução encontrada até o momento é retornada.

## Modelos de Programação Linear Inteira

Além da meta-heurística, foram implementados os seguintes modelos de Programação Linear Inteira para o problema de Bin Packing:

1. **Modelo de Atribuição de Itens a Bins**
2. **Modelo de Fluxo em Rede**
3. **Modelo de Geração de Colunas**

Cada modelo possui suas vantagens e limitações em termos de eficiência computacional e qualidade das soluções.

## Execução

Para executar a meta-heurística:
```
./bin_packing [arquivo_instancia] [tempo_limite_segundos]
```

Exemplo:
```
./bin_packing instancias/binpack1.txt 60
```

## Estrutura do Projeto

```
bin_packing/
├── src/
│   ├── main.cpp
│   ├── solution.h
│   ├── local_search.cpp
│   ├── local_search.h
│   ├── models/
│   │   ├── ilp_model.cpp
│   │   ├── ilp_model.h
│   │   ├── column_generation.cpp
│   │   └── column_generation.h
├── instances/
│   ├── binpack1.txt
│   ├── binpack2.txt
│   └── ...
├── results/
└── Makefile
```

## Resultados e Análise

Os resultados obtidos pela meta-heurística são comparados com os limites inferiores teóricos e com as soluções ótimas (quando conhecidas) para cada instância. Uma análise do desempenho do algoritmo em termos de qualidade da solução e tempo de execução é apresentada.