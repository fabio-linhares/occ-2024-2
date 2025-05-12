# Descrição do Problema de Wave Order Picking

## Visão Geral

O problema de Wave Order Picking do Mercado Livre consiste em selecionar um subconjunto de pedidos (wave) do backlog e um subconjunto de corredores a serem visitados, de forma a maximizar a eficiência da coleta.

## Terminologia

- **Backlog**: Conjunto completo de pedidos pendentes (O)
- **Wave**: Subconjunto de pedidos selecionados para processamento (O')
- **Corredores**: Locais do armazém onde os itens estão armazenados (A)
- **Itens**: Produtos solicitados nos pedidos (I)
- **Picking**: Processo de coleta de itens nos corredores

## Definições Formais

Sejam:
- O = {0, …, o−1}: o backlog de pedidos
- A = {0, …, a−1}: o conjunto de corredores
- I = {0, …, i−1}: o conjunto de todos os itens
- I_o ⊆ I: itens solicitados pelo pedido o
- u_{oi}: unidades de item i pedidas por o
- u_{ai}: unidades de item i disponíveis no corredor a
- LB e UB: limites inferior e superior do total de unidades na wave

O problema consiste em selecionar:
1. Um subconjunto de pedidos O' ⊆ O
2. Um subconjunto de corredores A' ⊆ A

De forma a maximizar a razão entre o número total de unidades coletadas e o número de corredores visitados.