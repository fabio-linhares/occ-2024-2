# Função Objetivo e Restrições

## Função Objetivo

O objetivo é maximizar a relação entre o número total de unidades de itens coletadas para os pedidos na wave e o número de corredores visitados:

```
maximizar ( ∑_{o∈O'} ∑_{i∈Io} uoi ) / |A'|
```

Onde:
- O' é o subconjunto de pedidos selecionados (a wave)
- A' é o subconjunto de corredores selecionados
- Io é o subconjunto de itens solicitados pelo pedido o
- uoi é o número de unidades do item i solicitado pelo pedido o
- |A'| é o número de corredores no subconjunto A'

## Restrições

Uma solução viável deve satisfazer três famílias de restrições:

### 1. Limite Inferior de Unidades (LB)

O total de unidades de todos os itens em todos os pedidos da wave (O') deve ser maior ou igual a um limite inferior (LB):

```
∑_{o∈O'} ∑_{i∈Io} uoi ≥ LB
```

### 2. Limite Superior de Unidades (UB)

O total de unidades de todos os itens em todos os pedidos da wave (O') deve ser menor ou igual a um limite superior (UB):

```
∑_{o∈O'} ∑_{i∈Io} uoi ≤ UB
```

### 3. Restrição de Capacidade/Estoque

Para cada item (i) que é pedido por qualquer pedido na wave (O'), a soma das unidades desse item disponíveis nos corredores selecionados (A') deve ser suficiente para atender à soma total das unidades desse item solicitadas por todos os pedidos na wave (O'):

```
∑_{o∈O'} uoi ≤ ∑_{a∈A'} uai, ∀i ∈ Io com o ∈ O'
```

## Linearização do Modelo (PuLP)

Para resolver o problema usando PuLP, introduzimos:

- Variáveis binárias:
  - x_o ∈ {0,1}: pedido o está em O'?
  - y_a ∈ {0,1}: corredor a está em A'?

- Variáveis auxiliares:
  - z ≥ 0: estimativa de 1/|A'|
  - w_o ≥ 0: lineariza o produto z·x_o

A função objetivo linearizada é:
```
max ∑_{o∈O} ∑_{i∈Io} uoi·w_o
```

Com restrições adicionais para garantir a equivalência com o modelo original.