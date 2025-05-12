# Informações das Instâncias

## Conjunto de Dados

O desafio inclui dois conjuntos principais de instâncias:

1. **Conjunto A**: Instâncias de tamanho pequeno/médio, adequadas para métodos exatos
2. **Conjunto B**: Instâncias de tamanho grande/regular, que podem requerer heurísticas

## Formato do Arquivo de Entrada

Cada instância é definida em um arquivo de texto com o seguinte formato:

- **Primeira linha**: Três números inteiros:
  - o: número total de pedidos no backlog
  - i: número total de tipos de itens diferentes
  - a: número total de corredores no armazém

- **Próximas o linhas**: Cada linha descreve um pedido:
  - Inicia com um inteiro k: número de tipos de itens diferentes solicitados
  - Seguido por k pares (item_index, quantity): índice do item e quantidade

- **Próximas a linhas**: Cada linha descreve a disponibilidade em um corredor:
  - Inicia com um inteiro l: número de tipos de itens disponíveis
  - Seguido por l pares (item_index, quantity): índice do item e quantidade disponível

- **Última linha**: Dois inteiros:
  - LB: Limite inferior para o total de unidades na wave
  - UB: Limite superior para o total de unidades na wave

### Exemplo de Entrada
```
5 5 5       # o=5 pedidos, i=5 itens, a=5 corredores
2 0 3 2 1   # Pedido 0: 2 itens. Item 0 (3 un), Item 2 (1 un)
2 1 1 3 1   # Pedido 1: 2 itens. Item 1 (1 un), Item 3 (1 un)
2 2 1 4 2   # Pedido 2: 2 itens. Item 2 (1 un), Item 4 (2 un)
4 0 1 2 2 3 1 4 1 # Pedido 3: 4 itens
1 1 1       # Pedido 4: 1 item. Item 1 (1 un)
4 0 2 1 1 2 1 4 1 # Corredor 0: 4 itens disponíveis
4 0 2 1 1 2 2 4 1 # Corredor 1: 4 itens disponíveis
3 1 2 3 1 4 2     # Corredor 2: 3 itens disponíveis
4 0 2 1 1 3 1 4 1 # Corredor 3: 4 itens disponíveis
4 1 1 2 2 3 1 4 2 # Corredor 4: 4 itens disponíveis
5 12        # LB=5, UB=12 para o tamanho da wave
```