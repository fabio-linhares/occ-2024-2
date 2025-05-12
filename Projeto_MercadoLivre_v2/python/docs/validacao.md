# Validação de Soluções

## Formato do Arquivo de Saída

Uma solução válida deve ser escrita em um arquivo de texto com o seguinte formato:

- **Primeira linha**: Um inteiro n, o número de pedidos selecionados para a wave (|O'|)
- **Próximas n linhas**: Cada linha contém um inteiro representando o índice de um pedido selecionado
- **Linha seguinte**: Um inteiro m, o número de corredores selecionados (|A'|)
- **Próximas m linhas**: Cada linha contém um inteiro representando o índice de um corredor selecionado

### Exemplo de Saída
```
4           # 4 pedidos na wave
0           # Pedido 0
1           # Pedido 1
2           # Pedido 2
4           # Pedido 4
2           # 2 corredores visitados
1           # Corredor 1
3           # Corredor 3
```

## Verificação de Viabilidade

Uma solução é considerada viável se satisfizer as três restrições principais:

1. **Limite Inferior (LB)**: A soma total de unidades dos pedidos selecionados deve ser ≥ LB
2. **Limite Superior (UB)**: A soma total de unidades dos pedidos selecionados deve ser ≤ UB
3. **Capacidade dos Corredores**: Para cada item requisitado nos pedidos selecionados, a disponibilidade total nos corredores selecionados deve ser suficiente

## Ferramenta de Validação

O script `checker.py` pode ser usado para verificar a viabilidade e calcular o valor objetivo de uma solução:

```
python checker.py <arquivo_entrada> <arquivo_solucao>
```

O valor objetivo é calculado como:
```
Total de unidades coletadas / Número de corredores visitados
```

Para a solução de exemplo com 10 unidades coletadas e 2 corredores visitados, o valor objetivo é 5.0.