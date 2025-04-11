# Explicação do Arquivo de Regras e Restrições

Este documento explica cada item do arquivo `rules_and_constraints.txt`, que é usado para configurar a solução do Problema da Seleção de Pedidos Ótima (SPO).

## Caminhos dos diretórios

- `INPUT_DIR`: Especifica o diretório onde estão localizados os arquivos de instância de entrada. Por exemplo, se suas instâncias estiverem na pasta `data/instances`, você deve configurar `INPUT_DIR=data/instances`.
- `OUTPUT_DIR`: Especifica o diretório onde serão salvos os arquivos de solução. As soluções geradas serão armazenadas aqui. Por exemplo, `OUTPUT_DIR=data/solutions` salvará as soluções na pasta `data/solutions`.

## Parâmetros do problema

- `MAX_TIME`: Define o tempo máximo de execução do algoritmo em segundos. Após esse tempo, o programa deve retornar a melhor solução encontrada até o momento. Isso é crucial para garantir que o algoritmo não execute indefinidamente. Por exemplo, `MAX_TIME=300` limita a execução a 300 segundos (5 minutos).

## Restrições

- `MIN_ITEMS`: Define o limite inferior (LB) para o número total de itens que devem ser selecionados em uma solução válida. Isso garante que a solução atenda a um mínimo de itens. Por exemplo, `MIN_ITEMS=500` exige que pelo menos 500 itens sejam selecionados.
- `MAX_ITEMS`: Define o limite superior (UB) para o número total de itens que podem ser selecionados em uma solução válida. Isso impõe um limite máximo para evitar soluções excessivas. Por exemplo, `MAX_ITEMS=2500` limita a seleção a no máximo 2500 itens.

## Função objetivo

- `OBJECTIVE`: Especifica a função objetivo a ser maximizada. Neste caso, `maximize_items_per_aisle` indica que queremos maximizar a razão entre o número de itens coletados e o número de corredores visitados. Outras funções objetivo poderiam ser implementadas, como minimizar o número de corredores visitados, maximizar o número total de itens, ou uma combinação ponderada de ambos.

## Algoritmo

- `ALGORITHM`: Especifica o algoritmo principal a ser usado para resolver o problema. Neste caso, `dinkelbach` refere-se ao método de Dinkelbach para resolver problemas de programação fracionária. Este método é eficiente para encontrar a solução ótima para funções objetivo fracionárias. Outros algoritmos poderiam ser implementados, como busca local, simulated annealing, ou algoritmos genéticos, dependendo da complexidade do problema e dos requisitos de desempenho.

## Parâmetros do algoritmo

- `EPSILON`: Define a precisão desejada para o método de Dinkelbach. O algoritmo irá parar quando a diferença entre duas iterações consecutivas for menor que este valor. Um valor menor de `EPSILON` resulta em maior precisão, mas também pode aumentar o tempo de execução. Por exemplo, `EPSILON=1e-8` aumenta a precisão em comparação com `EPSILON=1e-6`.
- `MAX_ITERATIONS`: Define o número máximo de iterações permitidas para o método de Dinkelbach. Isso evita que o algoritmo entre em um loop infinito caso não consiga convergir. É importante definir um valor adequado para garantir que o algoritmo termine em um tempo razoável. Por exemplo, `MAX_ITERATIONS=200` permite que o algoritmo execute até 200 iterações.

## Flags de validação

- `VALIDATE_ITEM_AVAILABILITY`: Quando verdadeiro, o programa verifica se há disponibilidade suficiente de itens nos corredores para atender aos pedidos selecionados. Isso garante que a solução seja factível. Se definido como `false`, a validação é desativada, o que pode ser útil para testes rápidos ou para instâncias onde a disponibilidade de itens não é uma preocupação.
- `VALIDATE_ORDER_IDS`: Quando verdadeiro, o programa verifica se todos os IDs de pedidos na solução são válidos (existem na instância de entrada). Isso garante que a solução não inclua pedidos inexistentes. Se definido como `false`, a validação é desativada, o que pode ser útil para testes rápidos ou para instâncias onde a validade dos IDs não é uma preocupação.

Este arquivo de configuração permite ajustar facilmente os parâmetros do problema e do algoritmo sem precisar modificar o código-fonte, tornando a solução mais flexível e adaptável a diferentes cenários ou requisitos. Ele facilita a experimentação com diferentes configurações e a otimização do desempenho do algoritmo.