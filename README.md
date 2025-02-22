# Projeto de Otimização Contínua e Combinatória  
**Reprodução e Otimização dos Algoritmos para o Problema da Intersecção Máxima de k-Subconjuntos**

Este repositório contém a implementação dos algoritmos apresentados no artigo **"Algoritmo ILS-VND para o Problema da Intersecção Máxima de k-Subconjuntos"** (Ferreira & Pinheiro, SBPO 2024) – com o objetivo de reproduzir fielmente os resultados e, a partir deles, otimizar a execução para torná-la mais rápida e eficiente.

---

## Visão Geral

O foco deste projeto é duplo:

1. **Reprodução Fiel em Ambiente Controlado:**  
   Devido às diferenças de hardware (já que não dispomos de uma máquina equivalente à do artigo), a execução idêntica dos algoritmos (ILS, VNS e GRASP-Tabu) é realizada via Docker. Essa abordagem permite atualizar os resultados de referência com base na nossa plataforma, evitando comparações equivocadas.

2. **Otimização e Superação dos Resultados Originais:**  
   A partir dos resultados reproduzidos, nosso objetivo é explorar oportunidades de melhoria na implementação – seja na estrutura dos algoritmos, seja em técnicas de otimização – para que, ao final, nossa implementação apresente desempenho superior ao descrito no artigo. Em outras palavras, queremos reduzir o tempo de execução e/ou obter soluções de melhor qualidade que a implementação original.

---

## Detalhes do Artigo Base

O artigo original, datado de 26 de outubro de 2023, aborda o **Problema da Intersecção Máxima de k-Subconjuntos (kMIS)**, definido formalmente como:

> Dada uma coleção L = {S₁, …, Sₙ} de n subconjuntos de um conjunto finito R = {e₁, …, eₘ} e um inteiro k, o objetivo é encontrar L′ ⊆ L com |L′| = k de forma que a interseção dos subconjuntos em L′ seja máxima.

**Principais Contribuições e Abordagens do Artigo:**

- **Meta-heurística ILS-VND:**  
  O artigo propõe uma estratégia iterated local search (ILS) que utiliza a Variable Neighborhood Descent (VND) para explorar duas estruturas de vizinhança:
  - **swap(1,1):** Remoção de um subconjunto e inserção de outro.
  - **swap*(2,2):** Remoção de dois subconjuntos e inserção de dois novos simultaneamente.

- **Otimização da Busca Local:**  
  A aceleração da busca é conseguida por meio do uso de estruturas de dados auxiliares:
  - **Array µ:** Armazena a interseção resultante da solução atual ao remover cada subconjunto individualmente.
  - **Matriz ϕ:** Guarda a interseção resultante após a remoção de pares de subconjuntos, otimizando a vizinhança swap*(2,2).

- **Perturbação e Representação:**  
  - **Perturbação:** Uma adaptação da estratégia ShakingGrasp é utilizada para diversificar a busca e escapar de ótimos locais, com um parâmetro α que controla o grau de aleatoriedade.
  - **Bitsets:** A utilização de bitsets permite operações AND bit a bit para calcular interseções de forma extremamente eficiente, aproveitando o paralelismo de bits.

- **Resultados Experimentais:**  
  Os experimentos demonstraram que a meta-heurística ILS-VND supera algoritmos concorrentes (como VNS e GRASP-Tabu) em termos de qualidade da solução e tempo de execução, atribuindo grande parte do ganho à otimização na busca local através dos arrays µ e ϕ.

---

## Contexto Científico e Objetivos

Este trabalho tem forte caráter científico e visa:

- **Estabelecer uma Base de Referência:**  
  Reproduzir a execução idêntica dos algoritmos do artigo em um ambiente Docker para atualizar os resultados de referência, compensando a diferença de hardware.

- **Otimizar a Execução:**  
  Identificar e implementar melhorias que acelerem o processamento dos algoritmos, potencialmente superando a performance original – tanto em tempo de execução quanto na qualidade das soluções obtidas.

- **Contribuição para a Literatura:**  
  Além de ser utilizado para avaliação da disciplina, o projeto tem potencial para publicação em revista científica, fornecendo uma análise comparativa detalhada e propostas de otimização para o problema kMIS.

---

## Estrutura do Projeto

```plaintext
├── src/
│   ├── DataGenerator.cpp    # Gera instâncias sintéticas conforme as especificações do artigo
│   ├── ils.cpp              # Implementação do algoritmo ILS-VND
│   ├── vns.cpp              # Implementação do algoritmo VNS
│   ├── grasp_tabu.cpp       # Implementação do algoritmo GRASP com Busca Tabu
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
  O script `run.sh` orquestra a execução dos algoritmos para todas as instâncias (238 instâncias divididas em 9 classes) e armazena os resultados no diretório `results/` em formato CSV e gráficos.

---

## Como Executar

### 1. Clonar o Repositório

```bash
git clone https://github.com/fabio-linhares/occ-2024-2.git
cd occ-2024-2
```

### 2. Construir a Imagem Docker

```bash
docker build -t kmis-cpp .
```

### 3. Executar o Container

```bash
docker run --cpuset-cpus="0-5" --memory="16g" kmis-cpp
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

## Limitações e Próximos Passos

### Limitações Conhecidas
- **Viés na Geração de Instâncias:** A ausência do código original para geração dos dados pode limitar comparações diretas.
- **Diferenças de Hardware:** A virtualização via Docker introduz overhead e variações de desempenho em relação ao hardware utilizado no artigo.

### Próximos Passos
- **Validação Cruzada:** Contatar os autores para obter as instâncias originais e confirmar as propriedades estatísticas.
- **Otimização:** Implementar melhorias nos algoritmos (novas estruturas de vizinhança, otimizações no uso de bitsets, ajustes na estratégia de perturbação) para reduzir o tempo de execução.
- **Testes Unitários:** Desenvolver testes para componentes críticos a fim de garantir a robustez das implementações.
- **Análise Estatística Avançada:** Aplicar testes mais robustos para validar a significância dos ganhos obtidos com as otimizações.

---

## Conclusão

Este projeto estabelece uma base robusta para a reprodução dos resultados do artigo **"Algoritmo ILS-VND para o Problema da Intersecção Máxima de k-Subconjuntos"** e propõe melhorias para otimizar a execução dos algoritmos. Com a execução controlada via Docker, garantimos a comparabilidade dos resultados, permitindo que futuras otimizações sejam avaliadas de forma justa. A expectativa é que, ao final do processo, nossa implementação não só reproduza como supere o desempenho da versão original.

---

## Referências

- **Artigo Base:**  
  FERREIRA, Hélder Silva; PINHEIRO, Rian. *Algoritmo ILS-VND para o Problema da Intersecção Máxima de k-Subconjuntos*. Anais do LVI Simpósio Brasileiro de Pesquisa Operacional, 2024, Fortaleza.  
  [Proceedings SBPO 2024](https://proceedings.science/sbpo/sbpo-2024/trabalhos/algoritmo-ils-vnd-para-o-problema-da-intersecao-maxima-de-k-subconjuntos?lang=pt-br)

- **Outras Referências:**  
  - Trabalhos sobre VNS, GRASP e otimizações via bitsets.  
  - Estudos de perturbação e estratégias de busca local.

---

## 👨‍💻 Sobre o Autor

Este projeto foi desenvolvido por **zerocopia** (Fabio Linhares).
Para saber mais sobre minha trajetória, [clique aqui e leia o aboutme.md](aboutme.md).

- [LinkedIn](https://www.linkedin.com/in/fabio-linhares/)
- [Portfólio](https://fabio-linhares.github.io/)

