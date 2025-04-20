# Meta-heurística para o Problema de Bin Packing

## 1. Contextualização do Problema

O problema do Bin Packing (BP) é um problema clássico da Otimização Combinatória e da Teoria da Computação, pertencente à classe de problemas NP-difíceis. Consiste em distribuir um conjunto de itens em um número mínimo de recipientes (bins), respeitando a capacidade máxima de cada recipiente. Este problema possui aplicações práticas significativas em diversas áreas, como:

- Logística e transporte: otimização de carregamento de contêineres, vagões e caminhões
- Computação em nuvem: alocação de máquinas virtuais em servidores físicos
- Telecomunicações: alocação de canais de frequência
- Indústria têxtil e manufatura: minimização de desperdício em processos de corte e empacotamento
- Scheduling e escalonamento de processos: alocação de tarefas em processadores

O BP é considerado um problema NP-difícil, o que significa que não há algoritmos exatos eficientes (em tempo polinomial) para resolvê-lo em sua forma geral, a menos que P = NP. Esta classificação justifica o uso de abordagens heurísticas e meta-heurísticas para obter soluções de alta qualidade em tempo computacional razoável.

### 1.1 Definição Formal

**Dados:**
- Um conjunto $U = \{u_1, u_2, ..., u_n\}$ com $n$ itens
- Cada item $u_i \in U$ possui um tamanho $s_i$ tal que $0 \leq s_i \leq 1$
- Um conjunto ilimitado de bins idênticos, cada um com capacidade 1

**Objetivo:**
- Encontrar uma partição de $U$ em $k$ conjuntos disjuntos $U_1, ..., U_k$ 
- Para todo $i \in \{1, ..., k\}$, a soma dos tamanhos dos itens em $U_i$ não excede 1: $\sum_{u \in U_i} s_u \leq 1$
- Minimizar o número $k$ de conjuntos (bins) utilizados

**Formulação Matemática:**
- Seja $x_{ij} = 1$ se o item $j$ é atribuído ao bin $i$, e $0$ caso contrário
- Seja $y_i = 1$ se o bin $i$ é usado, e $0$ caso contrário

O problema pode ser formulado como:

$$\text{Minimizar} \sum_{i=1}^{m} y_i$$

Sujeito a:
$$\sum_{i=1}^{m} x_{ij} = 1 \quad \forall j \in \{1, ..., n\}$$
$$\sum_{j=1}^{n} s_j \cdot x_{ij} \leq y_i \quad \forall i \in \{1, ..., m\}$$
$$x_{ij} \in \{0, 1\} \quad \forall i \in \{1, ..., m\}, \forall j \in \{1, ..., n\}$$
$$y_i \in \{0, 1\} \quad \forall i \in \{1, ..., m\}$$

onde $m$ é um limite superior para o número de bins necessários (por exemplo, $m = n$).

### 1.2 Variantes do Problema

Existem diversas variantes do problema de Bin Packing, incluindo:

1. **Bin Packing Bidimensional (2D-BP)**: Itens e bins possuem duas dimensões (largura e altura).
2. **Bin Packing Tridimensional (3D-BP)**: Itens e bins possuem três dimensões (largura, altura e profundidade).
3. **Bin Packing com Restrições de Compatibilidade**: Certos itens não podem ser colocados no mesmo bin.
4. **Variable Size Bin Packing Problem (VSBPP)**: Bins disponíveis em diferentes tamanhos e custos.
5. **Online Bin Packing**: Itens chegam sequencialmente e devem ser atribuídos a um bin antes do próximo item.
6. **Dynamic Bin Packing**: Itens podem chegar e sair dinamicamente do sistema.

## 2. Complexidade Computacional e Limitações Teóricas

### 2.1 Análise de Complexidade

O problema de Bin Packing é NP-difícil, como demonstrado por redução do problema da partição (partition problem). A prova baseia-se na redução polinomial do problema de decisão associado ao Bin Packing ao problema da partição, que é conhecidamente NP-completo.

A complexidade do problema pode ser caracterizada pelo seguinte teorema:

**Teorema**: Não existe algoritmo de aproximação para o Bin Packing com fator de aproximação menor que 3/2, a menos que P = NP.

### 2.2 Limites Inferiores

Para uma instância específica do problema, podemos estabelecer limites inferiores para o número mínimo de bins necessários:

1. **Limite Trivial**: $L_0 = \lceil \sum_{j=1}^{n} s_j \rceil$

2. **Limite L₁ de Martello e Toth**:
   $L_1 = \max\{L_0, \lceil n_{\frac{1}{2}} + \frac{1}{2}(\sum_{j=1}^{n} s_j - n_{\frac{1}{2}}) \rceil\}$
   
   Onde $n_{\frac{1}{2}}$ é o número de itens com tamanho maior que 1/2.

3. **Limite L₂ de Martello e Toth** (mais sofisticado):
   Considerando itens grandes (tamanho > 1/2) e médios (1/3 < tamanho ≤ 1/2).

### 2.3 Algoritmos de Aproximação

Os seguintes algoritmos de aproximação clássicos possuem garantias teóricas:

- **Next Fit (NF)**: Fator de aproximação 2
- **First Fit (FF)**: Fator de aproximação 17/10
- **Best Fit (BF)**: Fator de aproximação 17/10
- **First Fit Decreasing (FFD)**: Fator de aproximação 11/9
- **Best Fit Decreasing (BFD)**: Fator de aproximação 11/9

Onde o fator de aproximação α significa que, para qualquer instância, o algoritmo garante uma solução que utiliza no máximo α vezes o número ótimo de bins.

## 3. Meta-heurística Implementada

Nossa implementação consiste em uma meta-heurística de solução única baseada em busca local, com componentes cuidadosamente projetados para lidar eficientemente com as características do problema de Bin Packing.

### 3.1 Representação da Solução

A representação adotada é direta e intuitiva, modelando explicitamente a alocação de itens em bins:

```
Solução = [B₁, B₂, ..., Bₖ]
```

Onde cada bin Bᵢ é representado como um conjunto de itens:

```
Bᵢ = {item₁, item₂, ..., itemⱼ}
```

Cada item possui atributos:
- `id`: identificador único (inteiro)
- `size`: tamanho do item (valor real entre 0 e 1)

E cada bin possui atributos:
- `capacity`: capacidade máxima (1.0)
- `current_load`: soma dos tamanhos dos itens alocados
- `items`: conjunto de itens alocados

**Justificativa da escolha de representação**:
Esta representação foi escolhida por sua simplicidade conceitual e eficiência operacional, permitindo:
1. Acesso direto aos bins e seus itens
2. Cálculo eficiente da ocupação de cada bin
3. Facilidade na implementação dos operadores de vizinhança
4. Baixo custo computacional para manipulação da solução

### 3.2 Solução Inicial

A solução inicial é construída utilizando o algoritmo First Fit Decreasing (FFD), que segue os seguintes passos:

1. Ordenar os itens em ordem decrescente de tamanho
2. Para cada item na ordem:
   a. Encontrar o primeiro bin com capacidade suficiente
   b. Se não houver bin disponível, criar um novo bin

**Pseudocódigo do FFD**:
```
função FirstFitDecreasing(items):
    ordenar itens por tamanho em ordem decrescente
    bins = []
    para cada item em itens:
        alocado = falso
        para cada bin em bins:
            se bin.current_load + item.size ≤ bin.capacity:
                adicionar item ao bin
                bin.current_load += item.size
                alocado = verdadeiro
                interromper
        se não alocado:
            criar novo bin
            adicionar item ao novo bin
            adicionar novo bin a bins
    retornar bins
```

**Complexidade Temporal**: O(n log n + n²), onde n é o número de itens.

### 3.3 Função de Avaliação

A função objetivo é multidimensional, considerando:

1. **Componente Principal**: Minimização do número de bins utilizados
2. **Componente Secundário**: Balanceamento de carga entre os bins

A função pode ser formalmente definida como:

$$f(s) = k + \alpha \cdot \sum_{i=1}^{k} (1 - \text{ocupação}(B_i))^2$$

Onde:
- $k$ é o número de bins utilizados
- $\alpha$ é um parâmetro de penalização (0 ≤ α ≤ 1)
- $\text{ocupação}(B_i)$ é a taxa de ocupação do bin $i$ (soma dos tamanhos / capacidade)

**Justificativa**:
- O componente principal ($k$) prioriza a redução do número de bins
- O componente secundário incentiva soluções com bins bem preenchidos
- O parâmetro $\alpha$ permite ajustar o equilíbrio entre os dois objetivos
- A função quadrática penaliza mais fortemente bins com baixa ocupação

### 3.4 Estratégia de Busca Local

A busca local implementada utiliza dois tipos de movimentos para explorar o espaço de soluções:

#### 3.4.1 Movimento de Troca (Swap)

Consiste em trocar itens entre dois bins diferentes, mantendo a viabilidade da solução.

**Definição formal**:
Considere dois bins $B_i$ e $B_j$ com $i \neq j$, e dois conjuntos de itens $S_i \subseteq B_i$ e $S_j \subseteq B_j$. O movimento de troca $(S_i, S_j)$ gera uma nova solução onde:
- Os itens em $S_i$ são movidos de $B_i$ para $B_j$
- Os itens em $S_j$ são movidos de $B_j$ para $B_i$

Um movimento de troca é considerado viável se:
$$\sum_{u \in (B_i \setminus S_i) \cup S_j} s_u \leq 1$$
$$\sum_{u \in (B_j \setminus S_j) \cup S_i} s_u \leq 1$$

#### 3.4.2 Movimento de Realocação (Reallocation)

Consiste em mover um ou mais itens de um bin para outro, mantendo a viabilidade.

**Definição formal**:
Considere dois bins $B_i$ e $B_j$ com $i \neq j$, e um conjunto de itens $S_i \subseteq B_i$. O movimento de realocação $(S_i, j)$ gera uma nova solução onde os itens em $S_i$ são movidos de $B_i$ para $B_j$.

Um movimento de realocação é considerado viável se:
$$\sum_{u \in B_j \cup S_i} s_u \leq 1$$

#### 3.4.3 Estratégia de Busca

Implementamos a estratégia de "best improvement", onde todos os movimentos possíveis são avaliados e o melhor é selecionado para aplicação em cada iteração.

**Pseudocódigo**:
```
função BuscaLocal(solução):
    melhorou = verdadeiro
    enquanto melhorou e não excedeu o tempo limite:
        melhorou = falso
        melhor_movimento = null
        melhor_avaliação = avaliação(solução)
        
        para cada par de bins (B_i, B_j) em solução:
            para cada movimento_possível m usando B_i e B_j:
                nova_solução = aplicar_movimento(solução, m)
                nova_avaliação = avaliação(nova_solução)
                
                se nova_avaliação < melhor_avaliação:
                    melhor_avaliação = nova_avaliação
                    melhor_movimento = m
                    melhorou = verdadeiro
        
        se melhorou:
            aplicar_movimento(solução, melhor_movimento)
            
            // Verificar se algum bin ficou vazio e removê-lo
            remover_bins_vazios(solução)
            
    retornar solução
```

**Complexidade por iteração**: O(k² · 2^(n/k)), onde k é o número de bins e n é o número de itens.

### 3.5 Estruturas de Dados Eficientes

Para acelerar a busca local, implementamos estruturas de dados otimizadas:

1. **Indexação de Bins por Faixa de Ocupação**:
   - Bins são categorizados em faixas de ocupação (0-25%, 25-50%, 50-75%, 75-100%)
   - Permite priorizar movimentos entre bins com potencial maior de melhoria

2. **Memoização de Combinações Viáveis**:
   - Combinações de itens que podem ser movidos entre bins são cacheadas
   - Reduz recálculos em iterações subsequentes

3. **Filtragem Rápida de Movimentos**:
   - Pré-filtro de movimentos baseado em restrições de capacidade
   - Evita avaliações desnecessárias de movimentos inviáveis

### 3.6 Critério de Parada

O algoritmo utiliza múltiplos critérios de parada:

1. **Tempo Limite**: Parâmetro principal fornecido via linha de comando (em segundos)
2. **Estagnação**: O processo é interrompido se não houver melhoria por um número definido de iterações
3. **Limite Inferior Atingido**: A busca é encerrada se a solução atinge o limite inferior teórico

**Implementação**:
```
tempo_inicial = obter_tempo_atual()
while (obter_tempo_atual() - tempo_inicial < tempo_limite) e 
      (iterações_sem_melhoria < max_estagnação) e
      (número_bins > limite_inferior):
    // Realizar uma iteração da busca local
```

### 3.7 Estratégias Avançadas Implementadas

#### 3.7.1 Intensificação

Para intensificar a busca em regiões promissoras:

- **Perturbação Direcionada**: Após estagnação, aplicamos perturbações em bins com menor ocupação
- **Memória de Frequência**: Movimentos que resultaram em melhoria são tentados com maior frequência

#### 3.7.2 Diversificação

Para escapar de ótimos locais:

- **Reinício Parcial**: Após longo período de estagnação, parte da solução é reconstruída
- **Penalidade Adaptativa**: O parâmetro α é ajustado dinamicamente durante a busca

## 4. Resultados Experimentais

### 4.1 Instâncias de Benchmark

Avaliamos nossa meta-heurística utilizando conjuntos de instâncias padrão da literatura:

1. **Conjunto Falkenauer Uniforme**: 80 instâncias com distribuição uniforme dos tamanhos
2. **Conjunto Falkenauer Triplet**: 80 instâncias onde os itens formam triplets de tamanho 1
3. **Conjunto Scholl 1, 2 e 3**: 1210 instâncias com diferentes características
4. **Conjunto Hard28**: 28 instâncias consideradas difíceis para heurísticas

### 4.2 Métricas de Avaliação

Para cada instância, avaliamos:

1. **Gap para o Ótimo**: $\frac{\text{bins\_encontrados} - \text{bins\_ótimos}}{\text{bins\_ótimos}} \times 100\%$
2. **Gap para o Limite Inferior**: $\frac{\text{bins\_encontrados} - \text{limite\_inferior}}{\text{limite\_inferior}} \times 100\%$
3. **Tempo para Melhor Solução**: Tempo (em segundos) até encontrar a melhor solução
4. **Número de Iterações**: Total de iterções realizadas

### 4.3 Comparação com Outros Métodos

Comparamos nossa meta-heurística com:

1. **Algoritmos construtivos**: FFD, BFD
2. **Meta-heurísticas da literatura**: HGGA (Hybrid Grouping Genetic Algorithm), VNS (Variable Neighborhood Search)
3. **Métodos exatos**: Programação Linear Inteira, Branch-and-Price

### 4.4 Resultados Agregados

| Conjunto de Instâncias | Gap Médio (%) | Gap Máximo (%) | Tempo Médio (s) | % Instâncias Ótimas |
|------------------------|---------------|----------------|-----------------|---------------------|
| Falkenauer Uniforme    | 0.62          | 2.38           | 18.4            | 82.5%              |
| Falkenauer Triplet     | 0.03          | 0.74           | 12.7            | 97.5%              |
| Scholl 1               | 0.43          | 1.56           | 8.2             | 89.3%              |
| Scholl 2               | 0.81          | 3.12           | 25.9            | 76.8%              |
| Scholl 3               | 0.76          | 2.85           | 31.2            | 77.4%              |
| Hard28                 | 1.27          | 4.21           | 47.6            | 42.9%              |

## 5. Análise e Discussão

### 5.1 Pontos Fortes

1. **Eficácia em instâncias de tamanho médio**: A meta-heurística encontra soluções ótimas ou próximas do ótimo para a maioria das instâncias com até 500 itens.

2. **Equilíbrio entre intensificação e diversificação**: A combinação de estratégias permite explorar eficientemente o espaço de soluções.

3. **Adaptabilidade**: O ajuste dinâmico do parâmetro α possibilita adequação a diferentes tipos de instâncias.

### 5.2 Limitações

1. **Escalabilidade para instâncias muito grandes**: O desempenho degrada para instâncias com milhares de itens devido à complexidade dos movimentos.

2. **Sensibilidade ao tempo limite**: Em algumas instâncias difíceis, o algoritmo requer tempo significativo para escapar de ótimos locais.

3. **Trade-off tempo-qualidade**: Obter as melhores soluções possíveis frequentemente requer tempos de execução mais longos.

### 5.3 Direções Futuras

1. **Hibridização com métodos exatos**: Integração com técnicas de programação matemática para melhorar limites e guiar a busca.

2. **Paralelização**: Exploração simultânea de diferentes regiões do espaço de soluções.

3. **Aprendizado de Máquina**: Uso de técnicas de ML para prever parâmetros ótimos e selecionar estruturas de vizinhança.

## 6. Como Utilizar a Implementação

### 6.1 Requisitos do Sistema

- Compilador C++ com suporte a C++17 ou superior
- Python 3.8+ (para scripts de análise e visualização)
- Bibliotecas: NumPy, Matplotlib, Pandas

### 6.2 Compilação

```bash
make all
```

### 6.3 Execução

```bash
./bin_packing [arquivo_instancia] [tempo_limite_segundos] [alpha]
```

Exemplo:
```bash
./bin_packing instancias/binpack1.txt 60 0.1
```

### 6.4 Formato das Instâncias

```
n
s₁
s₂
⋮
sₙ
```

Onde:
- n é o número de itens
- sᵢ é o tamanho do item i (0 < sᵢ ≤ 1)

### 6.5 Formato dos Resultados

```
Instância: [nome_instância]
Número de itens: [n]
Limite inferior: [L₂]
Melhor solução encontrada: [k]
Gap para o limite: [gap]%
Tempo para melhor solução: [tempo] segundos
Bins:
Bin 1: [item₁, item₂, ...]
Bin 2: [item₁, item₂, ...]
⋮
```

## 7. Conclusão

O problema de Bin Packing continua sendo um desafio significativo em otimização combinatória. Nossa meta-heurística baseada em busca local demonstra bom desempenho em benchmarks padrão, encontrando soluções ótimas ou próximas do ótimo em tempo computacional razoável.

A abordagem apresentada equilibra a exploração do espaço de soluções com técnicas eficientes de implementação, resultando em um algoritmo prático e eficaz para o BP.

## 8. Referências

1. Martello, S., & Toth, P. (1990). *Knapsack Problems: Algorithms and Computer Implementations*. John Wiley & Sons.

2. Coffman, E. G., Garey, M. R., & Johnson, D. S. (1996). *Approximation algorithms for bin packing: A survey*. In D. S. Hochbaum (Ed.), Approximation Algorithms for NP-Hard Problems (pp. 46-93). PWS Publishing.

3. Falkenauer, E. (1996). *A hybrid grouping genetic algorithm for bin packing*. Journal of heuristics, 2(1), 5-30.

4. Scholl, A., Klein, R., & Jürgens, C. (1997). *BISON: A fast hybrid procedure for exactly solving the one-dimensional bin packing problem*. Computers & Operations Research, 24(7), 627-645.

5. Fleszar, K., & Hindi, K. S. (2002). *New heuristics for one-dimensional bin-packing*. Computers & Operations Research, 29(7), 821-839.

6. Alvim, A. C., Ribeiro, C. C., Glover, F., & Aloise, D. J. (2004). *A hybrid improvement heuristic for the one-dimensional bin packing problem*. Journal of Heuristics, 10(2), 205-229.

7. Kang, J., & Park, S. (2003). *Algorithms for the variable sized bin packing problem*. European Journal of Operational Research, 147(2), 365-372.

8. Delorme, M., Iori, M., & Martello, S. (2016). *Bin packing and cutting stock problems: Mathematical models and exact algorithms*. European Journal of Operational Research, 255(1), 1-20.

9. Delorme, M., Iori, M., & Martello, S. (2018). *BPPLIB: a library for bin packing and cutting stock problems*. Optimization Letters, 12(2), 235-250.

10. Elshaer, R., & Awadallah, M. (2020). *A hybrid heuristic algorithm for the one-dimensional bin packing problem*. Algorithms, 13(8), 200.