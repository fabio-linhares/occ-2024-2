# Projeto de Otimização: Picking (Inspirado no Desafio SBPO 2025)

Este projeto foi desenvolvido como parte da disciplina de **Otimização Contínua e Combinatória** do Mestrado em Informática da Universidade Federal de Alagoas (UFAL). O objetivo é modelar e resolver um problema de otimização realista, aplicando técnicas avançadas estudadas ao longo do curso. Escolhi abordar o problema proposto pelo Mercado Livre no Desafio SBPO 2025, não como participante da competição, mas como estudo de caso para fins acadêmicos.

---

## 1. Introdução ao Problema

O problema em questão envolve a seleção de um subconjunto de pedidos (denominado *wave*) a ser coletado em um armazém, visando maximizar a eficiência da operação de picking. A eficiência é medida pela razão entre o total de unidades coletadas e o número de corredores percorridos.

A jornada de um pacote no Mercado Livre começa muito antes do transporte. Após um pedido ser feito por um cliente, ele entra em um conjunto de pedidos pendentes, chamado **backlog**. O desafio central é agrupar esses pedidos em subconjuntos, denominados **waves**. A coleta de **itens** para um único pedido é menos eficiente do que agrupar pedidos em waves, pois permite aproveitar sinergias nos locais de coleta.

Uma maneira de aumentar a eficiência é concentrar os itens da wave na menor quantidade possível de **corredores**. Um corredor é o espaço livre entre fileiras de estantes onde os itens são armazenados.

### Definições:

* **Backlog (O):** Conjunto de todos os pedidos pendentes que ainda não foram atendidos.
* **Wave (O'):** Subconjunto de pedidos selecionados para coleta em uma determinada rodada.
* **Corredores (A):** Caminhos no armazém onde os itens estão localizados.
* **Corredores visitados (A'):** Subconjunto de corredores efetivamente percorridos.

**Objetivo:** Maximizar a produtividade do picking, ou seja, o total de unidades coletadas por corredor:

$$
\max \frac{\sum_{o \in O'} \sum_{i \in I_o} u_{oi}}{|A'|}
$$

Onde:

* $u_{oi}$: Quantidade do item $i$ no pedido $o$.
* $I_o$: Conjunto de itens no pedido $o$.

---

## 2. Formulação Matemática

### 2.1 Variáveis de Decisão

* $x_o \in \{0,1\}$: Indica se o pedido $o$ está na wave.
* $y_a \in \{0,1\}$: Indica se o corredor $a$ é visitado.
* $z \geq 0$: Variável auxiliar representando $1/|A'|$.
* $w_o \geq 0$: Variável auxiliar para linearizar o produto $z \cdot x_o$.

### 2.2 Restrições

1. **Limite Inferior (LB):**

    $$
    \sum_{o \in O} \sum_{i \in I_o} u_{oi} \cdot x_o \geq LB
    $$

2. **Limite Superior (UB):**

    $$
    \sum_{o \in O} \sum_{i \in I_o} u_{oi} \cdot x_o \leq UB
    $$

3. **Cobertura de Itens:** Para cada item $i$:

    $$
    \sum_{o \in O} u_{oi} \cdot x_o \leq \sum_{a \in A} u_{ai} \cdot y_a
    $$

    Onde $u_{ai}$ representa a quantidade do item $i$ disponível no corredor $a$.

4. **Definição de $z$:**

    $$
    \sum_{a \in A} y_a \cdot z = 1
    $$

    Esta equação define $z$ como o inverso do número de corredores visitados.

5. **Linearização de $w_o = z \cdot x_o$:**

    Utilizamos técnicas de linearização, como as propostas por Fortet e Glover, para modelar o produto de variáveis.

### 2.3 Função Objetivo Linearizada

Com a introdução das variáveis auxiliares, a função objetivo original é transformada em:

$$
\max \sum_{o \in O} \sum_{i \in I_o} u_{oi} \cdot w_o
$$

### 2.4 Técnicas de Linearização Avançadas

Para linearizar a função objetivo fracionária, introduzimos:

1. **Variável $z$ como inverso de $|A'|$**: 
    $$z = 1 / \sum_{a \in A} y_a$$

2. **Variáveis auxiliares $w_o = z \cdot x_o$** com as seguintes restrições:
    $$w_o \leq x_o, \forall o \in O$$
    $$w_o \leq M \cdot z, \forall o \in O$$
    $$w_o \geq x_o + M \cdot (z - 1), \forall o \in O$$

Onde $M$ é uma constante suficientemente grande.

Para a restrição não-linear $\sum_{a \in A} y_a \cdot z = 1$, utilizamos uma linearização por partes (piecewise linearization) que aproxima a relação entre $z$ e $\sum_{a \in A} y_a$.

---

## 3. Métodos de Solução

### 3.1 Abordagem Exata (ILP)

Para instâncias de tamanho pequeno a médio, o problema é modelado como um Programa Inteiro Linear (ILP) e resolvido utilizando ferramentas como PuLP ou OR-Tools, com suporte opcional ao solver CPLEX para maior eficiência.

### 3.2 Heurísticas

Para instâncias maiores, onde a abordagem exata se torna computacionalmente inviável, são aplicadas heurísticas:

* **Construtivas:** Seleção gulosa de pedidos com base na razão unidades/corredores impactados.
* **Busca Local:** Aplicação de técnicas como 2-opt para melhorar soluções iniciais.

### 3.3 Lagrangeano Aumentado (ALM)

Implementação do método de Lagrangeano Aumentado que combina penalidades quadráticas com atualizações de multiplicadores de Lagrange. Este método oferece vantagens significativas:

1. **Formulação com Penalidades:** Transformamos as restrições em termos de penalidade na função objetivo:
    
    $$
    \max \sum_{o \in O} \sum_{i \in I_o} u_{oi} \cdot w_o + \sum_{j=1}^{3} \lambda_j g_j(x) - \sum_{j=1}^{3} \rho_j \sum_{k=0}^{K_j} (t_k^{(j)})^2 \alpha_k^{(j)}
    $$

    Onde:
    - $\lambda_j$ são multiplicadores de Lagrange
    - $g_j(x)$ representam as restrições do problema
    - $\rho_j$ são parâmetros de penalidade
    - Os termos com $\alpha_k^{(j)}$ representam a linearização por partes das penalidades quadráticas

2. **Atualização Iterativa:** Os multiplicadores são atualizados de acordo com:
    
    $$
    \lambda_j^{(t+1)} = \lambda_j^{(t)} + \gamma g_j(x)
    $$

Esta abordagem garante convergência a soluções viáveis de alta qualidade sem necessidade de aumentar excessivamente os parâmetros de penalidade.

---

## 4. Estrutura do Código

```
mercado-livre-challenge/
├── main.py               # Entrada/saída de instâncias
├── challenge.py          # Parser de dados
├── challenge_solver.py   # Modelagem ILP, heurísticas e ALM
├── challenge_solution.py # Serialização de O', A'
├── run_challenge.py      # Benchmark em lote (limite 10 min)
├── checker.py            # Validação de saída
└── requirements.txt      # PuLP, OR-Tools, numpy, pytest
```

### Fluxo de Execução:

1. **Leitura da Instância:** `main.py` utiliza `challenge.py` para interpretar os dados de entrada.
2. **Modelagem e Solução:** `challenge_solver.py` aplica a abordagem escolhida (ILP, heurística ou ALM).
3. **Verificação de Viabilidade:** `checker.py` valida a solução gerada.
4. **Monitoramento de Tempo:** `run_challenge.py` gerencia a execução dentro do limite de tempo.
5. **Escrita da Saída:** `challenge_solution.py` formata e grava a solução final.

---

## 5. Formato de Arquivos

### 5.1 Entrada

O arquivo de entrada segue um formato específico:

```
o i a                # Número de pedidos, itens, corredores
# Próximas o linhas: k pares (item, unidades)
# Próximas a linhas: l pares (item, unidades)
LB UB                # Limites inferior e superior
```

**Exemplo de Arquivo de Entrada:**

```
5 5 5       # o=5 pedidos, i=5 itens, a=5 corredores
2 0 3 2 1   # Pedido 0: 2 itens. Item 0 com 3 unidades, Item 2 com 1 unidade
2 1 1 3 1   # Pedido 1: 2 itens. Item 1 com 1 unidade, Item 3 com 1 unidade
2 2 1 4 2   # Pedido 2: 2 itens. Item 2 com 1 unidade, Item 4 com 2 unidades
4 0 1 2 2 3 1 4 1 # Pedido 3: 4 itens. Item 0 com 1, Item 2 com 2, Item 3 com 1, Item 4 com 1
1 1 1       # Pedido 4: 1 item. Item 1 com 1 unidade
4 0 2 1 1 2 1 4 1 # Corredor 0: 4 itens. Item 0 com 2, Item 1 com 1, Item 2 com 1, Item 4 com 1
4 0 2 1 1 2 2 4 1 # Corredor 1: 4 itens. Item 0 com 2, Item 1 com 1, Item 2 com 2, Item 4 com 1
3 1 2 3 1 4 2   # Corredor 2: 3 itens. Item 1 com 2, Item 3 com 1, Item 4 com 2
4 0 2 1 1 3 1 4 1 # Corredor 3: 4 itens. Item 0 com 2, Item 1 com 1, Item 3 com 1, Item 4 com 1
4 1 1 2 2 3 1 4 2 # Corredor 4: 4 itens. Item 1 com 1, Item 2 com 2, Item 3 com 1, Item 4 com 2
5 12        # LB=5, UB=12 para o tamanho da wave (em unidades)
```

### 5.2 Saída

```
n                   # Número de pedidos em O'
(indices de pedidos)
m                   # Número de corredores em A'
(indices de corredores)
```

**Exemplo de Arquivo de Saída:**

```
4           # n=4 pedidos na wave
0           # Pedido 0
1           # Pedido 1
2           # Pedido 2
4           # Pedido 4
2           # m=2 corredores visitados
1           # Corredor 1
3           # Corredor 3
```

Para este exemplo, a wave inclui os pedidos 0, 1, 2 e 4, com 10 unidades totais coletadas em 2 corredores (1 e 3), resultando em um valor objetivo de 5.0 (10/2).

---

## 6. Configuração do Ambiente

Para garantir a compatibilidade e o desempenho adequado, recomenda-se:

```bash
conda create -n sbpo2025 python=3.11
conda activate sbpo2025
pip install -r requirements.txt
```

**Arquivo `requirements.txt` sugerido:**

```
pulp==2.7.0
ortools==9.11.5705
numpy==1.26.4
pytest==7.4.3
```

Opcionalmente, configurar o CPLEX 22.11 via `.condarc` ou variáveis de ambiente.

---

## 7. Considerações Finais

Este projeto demonstra a aplicação prática de técnicas avançadas de otimização combinatória em um problema realista de logística. A estrutura modular do código, aliada à documentação detalhada, facilita a compreensão e a reprodução dos resultados, servindo como base para estudos futuros e aprimoramentos na área.

A eficiente gestão do backlog de pedidos não apenas otimiza as rotas de coleta, mas também contribui para a redução de custos operacionais e melhor utilização dos recursos do armazém. O balanceamento entre complexidade algorítmica e tempo de execução (limitado a 10 minutos por instância) representa um desafio adicional, incentivando o desenvolvimento de heurísticas eficientes e técnicas de linearização avançadas.

---

## Referências

1. Patterson, D. A. & Hennessy, J. L. "Arquitetura de Computadores – Uma Abordagem Quantitativa".
2. Teixeira, M. "Iterated Local Search para Stable Matching".
3. Repositório SBPO 2025: [https://github.com/mercadolibre/challenge-sbpo-2025](https://github.com/mercadolibre/challenge-sbpo-2025)
4. Fortet, R. "Applications de l'algèbre de Boole en recherche opérationnelle".
5. Glover, F. "Improved Linear Integer Programming Formulations of Nonlinear Integer Problems".

---

Se desejar, posso fornecer exemplos de entrada e saída ou auxiliar na implementação de testes automatizados para validar as soluções geradas.
