# Meta-heurística para o Problema de Bin Packing

Este projeto implementa uma meta-heurística híbrida para resolver o problema de Bin Packing de forma eficiente. A aplicação inclui uma interface web interativa para visualização e experimentação com diferentes instâncias do problema.

## O Problema de Bin Packing

O problema de Bin Packing consiste em empacotar um conjunto de itens com dimensões variadas em recipientes (bins) de capacidade fixa, minimizando o número total de recipientes utilizados. Este é um problema NP-difícil clássico com aplicações em diversas áreas como logística, manufatura e alocação de recursos.

## Nossa Abordagem

Implementamos uma meta-heurística híbrida que combina:
- Busca Local (Best/First Improvement)
- Simulated Annealing
- Iterated Local Search com perturbações adaptativas
- Estratégias de reinício

## Características

- Inicialização com First-Fit Decreasing
- Múltiplos tipos de perturbação para escapar de ótimos locais
- Visualização colorida dos itens em cada bin
- Métricas detalhadas sobre a qualidade da solução
- Interface interativa para carregamento de instâncias e configuração do algoritmo

## Como Executar

### Requisitos

- Python 3.8+
- Streamlit
- Pandas
- NumPy
- Altair
- Matplotlib
- PIL (Pillow)

### Instalação

```bash
# Clone o repositório
git clone https://github.com/fabio-linhares/occ-2024-2.git

# Crie um ambiente conda (opcional)
conda create -n occ2024-2 python=3.12
conda activate occ2024-2

# Instale as dependências
pip install streamlit pandas numpy altair matplotlib pillow
```

### Executando a Aplicação

Para iniciar a aplicação Streamlit, execute o seguinte comando no terminal:

```bash
streamlit run b1/app/main.py
```

O navegador será aberto automaticamente com a aplicação em execução. Se isso não acontecer, acesse:
- Local: http://localhost:8501
- Rede: http://seu-ip:8501

### Utilizando a Aplicação

1. Navegue até a aba "Heurísticas" → "Solução"
2. Carregue um arquivo contendo uma instância do problema
   - Cada linha do arquivo deve conter um número entre 0 e 1 (tamanho do item)
3. Configure o tempo limite de execução e o número máximo de bins
4. Clique em "Executar Meta-heurística"
5. Analise os resultados e visualizações

## Estrutura do Projeto



```
b1/
├── app/                   # Código da aplicação
│   ├── main.py            # Arquivo principal da aplicação
│   ├── styles/            # Arquivos CSS para estilização
│   └── pages/             # Páginas da aplicação
├── data/                  # Dados e exemplos
│   ├── instances/         # Instâncias do problema
│   └── logos/             # Logos institucionais
└── README.md              # Este arquivo
```

## Exemplos de Instâncias

O diretório `data/instances/` contém exemplos de instâncias do problema que podem ser utilizadas para testar a aplicação.

## Desenvolvido por

**Fábio Linhares**  
Universidade Federal de Alagoas (UFAL)  
Programa de Pós-Graduação em Informática (PPGI)

[Repositório GitHub](https://github.com/fabio-linhares/occ-2024-2/tree/main/b1)