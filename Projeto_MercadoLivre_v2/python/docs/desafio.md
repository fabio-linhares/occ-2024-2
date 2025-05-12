# Desafio SBPO 2025 do Mercado Livre

## Introdução

O Desafio SBPO 2025, promovido pelo Mercado Livre, é um exemplo paradigmático de problema de otimização combinatória aplicado à logística de armazenagem. A proposta central consiste em selecionar, de um conjunto de pedidos pendentes (backlog), um subconjunto (wave) que maximize a eficiência da coleta (picking) ao mesmo tempo em que minimiza o número de corredores percorridos no depósito.

Esse trade-off entre produtividade e deslocamento físico torna o problema intrinsecamente desafiador do ponto de vista teórico e implementacional.

## Contextualização e Motivação

Ao posicionar-se dentro do LVII Simpósio Brasileiro de Pesquisa Operacional (SBPO 2025), o Mercado Livre expõe um cenário real, onde a eficiência operacional impacta diretamente nos custos e na satisfação do cliente. O objetivo de agrupar pedidos em waves e concentrar itens em poucos corredores reflete necessidades cotidianas em grandes centros de distribuição.

A relevância prática é reforçada pela crescente demanda do e-commerce e pela pressão por entregas rápidas. Em armazéns de larga escala, o picking pode responder por até 60% do custo operacional de separação de mercadorias. Assim, soluções que aumentem a densidade de coleta por trajeto reduzem tempo de mão de obra e uso de equipamentos.

Além dos impactos econômicos, o desafio oferece um rico laboratório para testar:

- Modelagem rigorosa de restrições de capacidade e limites de onda
- Aplicação de técnicas de programação inteira (ILP) e heurísticas avançadas
- Integração de scripts e frameworks (Python, PuLP)