# Projeto de Otimização Contínua e Combinatória  
**Implementação de Algoritmos para o Problema da Seleção de Pedidos Ótima (SPO)**

Este repositório contém a implementação de algoritmos para resolver o **Problema da Seleção de Pedidos Ótima (SPO)** proposto no **Primeiro Desafio Mercado Livre de Otimização** em colaboração com a Sociedade Brasileira de Pesquisa Operacional (SOBRAPO) para o LVII SBPO 2025.

---

## Visão Geral

O foco deste projeto é desenvolver algoritmos eficientes para:

1. **Resolver o Problema da Seleção de Pedidos Ótima:**  
   O projeto visa implementar e otimizar algoritmos capazes de selecionar subconjuntos de pedidos (waves) que maximizem a produtividade em operações de logística de e-commerce, respeitando as restrições operacionais.

2. **Otimização e Análise de Desempenho:**  
   Além da implementação base, buscamos explorar diferentes abordagens algorítmicas e técnicas de otimização para melhorar a qualidade das soluções e o tempo de execução, comparando os resultados obtidos nas instâncias de teste fornecidas.

---

## Módulos Especializados

Este projeto contém dois módulos específicos para abordar aspectos complementares de otimização:

### 1. Bin Packing (Prof. Bruno)

O módulo de Bin Packing implementa soluções para o problema clássico de empacotamento, onde o objetivo é distribuir um conjunto de itens com diferentes tamanhos em recipientes (bins) de capacidade fixa, minimizando o número total de recipientes utilizados.

**Características principais:**
- Implementação de algoritmos heurísticos como First Fit Decreasing (FFD)
- Meta-heurística baseada em busca local com movimentos adaptativos
- Interface interativa para visualização e análise de resultados
- Suporte para instâncias reais e geração de instâncias aleatórias

**Aplicações práticas:**
- Otimização de carregamento de containers
- Alocação de recursos em ambientes virtualizados
- Planejamento de cortes em materiais industriais

### 2. Programação Linear Inteira (Prof. Rian)

O módulo de Programação Linear Inteira implementa soluções para problemas de otimização combinatória, utilizando formulações matemáticas que permitem encontrar soluções ótimas ou de alta qualidade para problemas complexos.

**Características principais:**
- Formulação de modelos matemáticos precisos usando o CPLEX
- Implementação de restrições para garantir a integralidade das variáveis
- Otimização de funções objetivo lineares sob restrições
- Técnicas para lidar com a complexidade computacional de problemas NP-difíceis

**Aplicações práticas:**
- Otimização de cadeias de suprimentos
- Escalonamento de produção
- Roteamento de veículos
- Problemas de localização de instalações

---

## Detalhes do Problema SPO

O Problema da Seleção de Pedidos Ótima é um desafio de otimização com origem em aplicações práticas da logística de e-commerce, definido formalmente como:

> Selecionar um subconjunto dos pedidos de clientes (wave) de forma a maximizar a razão entre o total de itens presentes nos pedidos e o número de corredores necessários para coletar esses itens, respeitando limites mínimos e máximos do total de itens decorrentes de restrições operacionais.

**Principais Características do Problema:**

- **Estrutura do Armazém:**  
  O armazém é organizado em corredores, cada um contendo determinados itens em estoque.

- **Objetivo:**  
  Maximizar a razão (total de itens na wave / número de corredores necessários).

- **Restrições:**  
  - Limites mínimos e máximos para o total de itens na wave.
  - Cada pedido é indivisível (deve ser incluído completamente ou excluído).

- **Representação e Modelagem:**  
  - Os pedidos podem ser representados como conjuntos de itens.
  - Os corredores podem ser mapeados para os itens que contêm.
  - A solução pode ser modelada como um problema de otimização combinatória.

---

## Contexto Científico e Objetivos

Este trabalho tem forte caráter científico e visa:

- **Desenvolver Algoritmos Eficientes:**  
  Implementar soluções algorítmicas para o problema SPO, explorando diferentes abordagens e técnicas de otimização.

- **Otimizar a Execução:**  
  Identificar e implementar melhorias que acelerem o processamento dos algoritmos, potencialmente superando a performance esperada – tanto em tempo de execução quanto na qualidade das soluções obtidas.

- **Contribuição para a Literatura:**  
  Além de ser utilizado para avaliação da disciplina, o projeto tem potencial para publicação em revista científica, fornecendo uma análise comparativa detalhada e propostas de otimização para o problema SPO.

---

## Sobre o Desafio Mercado Livre

O **Primeiro Desafio Mercado Livre de Otimização** é uma iniciativa do Shipping Optimization Team (SOT) do Mercado Livre em colaboração com a Sociedade Brasileira de Pesquisa Operacional (SOBRAPO).

### Datas Importantes
- **Inscrições:** Até 15 de agosto de 2025
- **Submissão de Soluções:** 30 de setembro de 2025
- **Divulgação dos Finalistas:** 15 de setembro de 2025
- **Apresentação no SBPO:** 6 a 9 de outubro de 2025 (Gramado, Brasil)

### Elegibilidade
- Equipes de até 3 estudantes
- Participantes devem estar matriculados em universidades de países onde o Mercado Livre opera na América Latina

### Avaliação
As soluções serão avaliadas por integrantes do Shipping Optimization Team do Mercado Livre, com base na qualidade das waves geradas para um conjunto de instâncias de teste fornecidas pelos organizadores.

### Premiação
As três equipes finalistas serão convidadas a apresentar seu trabalho durante uma sessão especial no LVII SBPO, que será realizado em Gramado, Brasil.

---

## Estrutura do Projeto

```plaintext
├── src/
│   ├── DataGenerator.cpp    # Gera instâncias sintéticas conforme as especificações do desafio
│   ├── spo_solver.cpp       # Implementação dos algoritmos para o problema SPO
│
├── scripts/
│   ├── run.sh               # Orquestra a execução dos algoritmos em todas as instâncias
│   ├── aggregate.py         # Processa os logs e gera análises estatísticas e visualizações
│
├── results/                 # Armazena os resultados experimentais
│
├── Dockerfile               # Define o ambiente Docker (Ubuntu 22.04, dependências e otimizações)
├── README.md                # Este arquivo
```

---

## Implementação e Ambiente Experimental

Para garantir a reprodutibilidade e comparabilidade dos experimentos, utilizamos uma infraestrutura baseada em Docker:

- **Dockerfile:**  
  - Base Ubuntu 22.04 LTS, com instalação de todas as dependências (g++, Python3, Pandas, NumPy, SciPy, etc.).
  - Compilação dos códigos com otimizações agressivas (`-O3 -march=native`).
  - Configuração do ambiente para gerar as instâncias de teste e executar os algoritmos.

- **Execução Automatizada:**  
  O script `run.sh` orquestra a execução dos algoritmos para todas as instâncias e armazena os resultados no diretório `results/` em formato CSV e gráficos.

---

## Como Executar

### 1. Clonar o Repositório

```bash
git clone https://github.com/fabio-linhares/spo-2025.git
cd spo-2025
```

### 2. Construir a Imagem Docker

```bash
docker build -t spo-cpp . 
```

### 3. Executar o Container

```bash
docker run --cpuset-cpus="0-5" --memory="16g" spo-cpp
```

*Observação:*  
Após a execução, os resultados estarão no diretório `results/` dentro do container. Para copiá-los para o sistema host, utilize:

```bash
docker cp <container-id>:/app/results ./results
```

---

## Resultados e Análise

- **Arquivos Gerados:**  
  - `results/summary.csv` – Resumo estatístico dos experimentos.
  - `results/graphs/` – Visualizações dos desempenhos dos algoritmos.

- **Análises:**  
  O script `aggregate.py` calcula métricas como melhor/pior custo, média e desvio padrão, além de realizar testes estatísticos (por exemplo, ANOVA) para comparar o desempenho dos algoritmos.

---

## Nossa Abordagem

Nossa estratégia para resolver o Problema da Seleção de Pedidos Ótima (SPO) combina:

1. **Modelagem Matemática:** Formalização do problema como um problema de otimização combinatória com restrições.

2. **Algoritmos Implementados:**
   - Heurística construtiva gulosa baseada na densidade de itens por corredor
   - Algoritmo de busca local com movimentos de inserção e remoção
   - Meta-heurística baseada em GRASP (Greedy Randomized Adaptive Search Procedure)
   - Abordagem híbrida combinando programação linear inteira e heurísticas

3. **Otimizações Algorítmicas:**
   - Estruturas de dados eficientes para representação de pedidos e corredores
   - Pré-processamento para identificação de pedidos promissores
   - Paralelização de execuções independentes para explorar diferentes pontos iniciais

---

## Limitações e Próximos Passos

### Limitações Conhecidas
- **Viés na Geração de Instâncias:** A ausência de instâncias reais pode limitar comparações diretas com aplicações práticas.
- **Diferenças de Hardware:** A virtualização via Docker introduz overhead e variações de desempenho em relação ao hardware utilizado em ambientes reais.

### Próximos Passos
- **Validação Cruzada:** Contatar os organizadores do desafio para obter instâncias reais e confirmar as propriedades estatísticas.
- **Otimização:** Implementar melhorias nos algoritmos (novas heurísticas, otimizações no uso de estruturas de dados, ajustes na estratégia de busca) para reduzir o tempo de execução.
- **Testes Unitários:** Desenvolver testes para componentes críticos a fim de garantir a robustez das implementações.
- **Análise Estatística Avançada:** Aplicar testes mais robustos para validar a significância dos ganhos obtidos com as otimizações.

---

## Conclusão

Este projeto estabelece uma base robusta para a implementação de algoritmos para o **Problema da Seleção de Pedidos Ótima (SPO)** e propõe melhorias para otimizar a execução dos algoritmos. Com a execução controlada via Docker, garantimos a comparabilidade dos resultados, permitindo que futuras otimizações sejam avaliadas de forma justa. A expectativa é que, ao final do processo, nossa implementação não só resolva como supere as expectativas do desafio.

---

## Referências

- **Desafio Base:**  
  Primeiro Desafio Mercado Livre de Otimização, em colaboração com a SOBRAPO para o LVII SBPO 2025.

- **Outras Referências:**  
  - Trabalhos sobre heurísticas e otimizações em problemas de logística.  
  - Estudos de modelagem e estratégias de busca local.

---

## 👨‍💻 Sobre o Autor

Este projeto foi desenvolvido por **zerocopia** (Fabio Linhares).
Para saber mais sobre minha trajetória, clique [aqui](aboutme.md).

- [LinkedIn](https://www.linkedin.com/in/fabio-linhares/)
- [Portfólio](https://fabio-linhares.github.io/)

