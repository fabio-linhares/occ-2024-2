# Standard library
import datetime
import os
import time
import math
import sys

# Adicionando o caminho base ao sys.path para importações absolutas
sys.path.append("/home/zerocopia/Projetos/occ-2024-2/b1")

# Third‑party libraries
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import streamlit as st

# Local application imports - usando caminho absoluto
from app.bin_packing import executar_heuristica, verificar_viabilidade_inicial


def app():
    # Título e subtítulo centralizados com espaçamento reduzido
    st.markdown("""
    <div style="text-align: center;">
        <h3 style="margin-bottom: 0.5rem; font-weight: normal; color: #A78BFA;">Otimização de empacotamento de itens utilizando busca local</h3>
    </div>
    """, unsafe_allow_html=True)
    
    # Dashboard no topo da página
    st.markdown("""
    <div style="margin-bottom: 1.5rem; animation: fadeIn 0.5s ease forwards;">
        <p>Esta aplicação resolve o problema do Bin Packing utilizando uma meta-heurística baseada em busca local.
        O objetivo é encontrar a melhor distribuição dos itens em recipientes, minimizando o número total de recipientes
        utilizados. A partir de uma solução inicial gerada pelo algoritmo First Fit Decreasing, a busca local
        tenta melhorar a solução através de movimentos de troca e realocação de itens.</p>
    </div>
    """, unsafe_allow_html=True)
    
    st.markdown("""
    ## Problema de Bin Packing
    
    O problema de Bin Packing é um problema clássico de otimização combinatória que consiste em empacotar um conjunto de itens em recipientes (bins) de capacidade fixa, de forma a minimizar o número de recipientes utilizados.
    
    ### Aplicações
    
    Este problema possui diversas aplicações práticas, como:
    
    - Alocação de contêineres em navios
    - Armazenamento de dados em discos
    - Corte de materiais para minimizar desperdício
    - Balanceamento de carga em servidores
    - Empacotamento de produtos para transporte
    """)
    
    # Formulação Matemática com layout aprimorado e fórmulas em estilo LaTeX para artigos científicos
    st.markdown(r"""
    ### Formulação Matemática
    
    O Bin Packing pode ser formulado como um problema de programação linear inteira:
    """)
    
    # Cards para variáveis de decisão, função objetivo e restrições
    col1, col2 = st.columns(2)
    
    with col1:
        st.markdown(r"""
        <div style="border-radius: 10px; padding: 15px; background-color: rgba(109, 40, 217, 0.2);">
            <h4 style="font-weight: bold; margin-top: 0;">Variáveis de decisão:</h4>
            <ul style="margin-bottom: 0;">
                <li>$y_j$: 1 se o bin $j$ é utilizado, 0 caso contrário</li>
                <li>$x_{ij}$: 1 se o item $i$ é alocado ao bin $j$, 0 caso contrário</li>
            </ul>
        </div>
        """, unsafe_allow_html=True)
    
    with col2:
        st.markdown(r"""
        <div style="border-radius: 10px; padding: 15px; background-color: rgba(59, 130, 246, 0.2);">
            <h4 style="font-weight: bold; margin-top: 0;">Função objetivo:</h4>
            <div style="text-align: center; font-size: 1.2em; padding: 10px;">
            $$\min \sum_{j \in J} y_j$$
            </div>
            <p style="font-style: italic; margin-top: 5px; text-align: center; font-size: 0.9em;">
            Minimizar o número total de bins utilizados
            </p>
        </div>
        """, unsafe_allow_html=True)
    
    st.markdown(r"""
    <div style="border-radius: 10px; padding: 15px; background-color: rgba(16, 185, 129, 0.2); margin-top: 15px;">
        <h4 style="font-weight: bold; margin-top: 0;">Restrições:</h4>
        <ol>
            <li>
                <strong>Cada item deve ser atribuído a exatamente um bin:</strong>
                <div style="text-align: center; padding: 10px;">
                    $\displaystyle\sum_{j \in J} x_{ij} = 1, \quad \forall i \in I$
                </div>
            </li>
            <li>
                <strong>A capacidade dos bins não pode ser excedida:</strong>
                <div style="text-align: center; padding: 10px;">
                    $\displaystyle\sum_{i \in I} w_i \cdot x_{ij} \leq C \cdot y_j, \quad \forall j \in J$
                </div>
            </li>
            <li>
                <strong>Restrições de integralidade:</strong>
                <div style="text-align: center; padding: 10px;">
                    $x_{ij} \in \{0,1\}, \quad \forall i \in I, j \in J \quad \text{e} \quad y_j \in \{0,1\}, \quad \forall j \in J$
                </div>
            </li>
        </ol>
    </div>
    """, unsafe_allow_html=True)
    
    st.markdown(r"""
    <div style="margin-top: 15px;">
        <strong>Onde:</strong>
        <ul>
            <li>$I$: conjunto de itens</li>
            <li>$J$: conjunto de bins (no pior caso, um bin para cada item)</li>
            <li>$w_i$: tamanho (peso) do item $i$</li>
            <li>$C$: capacidade de cada bin</li>
        </ul>
    </div>
    """, unsafe_allow_html=True)
    
    # Seção sobre as heurísticas implementadas
    st.markdown("""
    ## Heurísticas Implementadas
    
    ### First Fit Decreasing (FFD)
    
    O algoritmo First Fit Decreasing é uma heurística construtiva que funciona da seguinte forma:
    
    1. Ordena os itens em ordem decrescente de tamanho
    2. Para cada item, tenta colocá-lo no primeiro bin onde ele couber
    3. Se não couber em nenhum bin existente, cria um novo bin
    
    **Pseudocódigo:**
    ```
    Função FirstFitDecreasing(itens):
        Ordena itens por tamanho (decrescente)
        bins = []
        
        Para cada item em itens:
            colocado = falso
            
            Para cada bin em bins:
                Se bin.capacidade_restante >= item.tamanho:
                    Adiciona item ao bin
                    colocado = verdadeiro
                    Break
            
            Se não colocado:
                Cria novo bin
                Adiciona item ao novo bin
                Adiciona novo bin à lista de bins
        
        Retorna bins
    ```
    
    ### Busca Local
    
    A meta-heurística de busca local implementada explora a vizinhança de uma solução inicial (gerada pelo FFD) através de dois tipos de movimentos:
    
    1. **Movimento de Realocação**: move um item de um bin para outro
    2. **Movimento de Troca**: troca itens entre dois bins
    
    A função de avaliação considera tanto o número de bins utilizados (componente principal) quanto o balanceamento de carga dos bins (componente secundário).
    
    **Processo de busca:**
    1. Começa com a solução inicial gerada pelo FFD
    2. Explora todos os movimentos possíveis da vizinhança
    3. Seleciona o melhor movimento (que reduz o valor da função de avaliação)
    4. Aplica o movimento, gerando uma nova solução
    5. Repete o processo até não encontrar mais movimentos de melhoria ou atingir o limite de tempo
    
    ### Resultados Esperados
    
    A combinação do algoritmo FFD com a busca local permite encontrar soluções de boa qualidade para o problema de Bin Packing em tempo computacional razoável.
    
    - O FFD fornece uma solução inicial rápida e de qualidade razoável
    - A busca local refina essa solução, tentando reduzir o número de bins utilizados
    
    Na aba "Bin Packing" é possível testar esses algoritmos em instâncias predefinidas ou geradas aleatoriamente.
    """)


    
    # Footer
    st.markdown("---")
    st.markdown("© 2025 Fábio Linhares | [GitHub](https://github.com/fabio-linhares/occ-2024-2/tree/main/b1) | Meta-heurística para o Problema de Bin Packing | v1.0")


# Função para visualizar uma solução
def display_solution(solution, history=None):
    # Criação de métricas em cards
    col1, col2, col3 = st.columns(3)
    
    with col1:
        st.markdown("""
        <div class="dashboard-card card-purple">
            <div class="card-title">Número de Bins</div>
            <div class="card-value">{}</div>
            <div class="card-subtitle">Total de recipientes utilizados</div>
        </div>
        """.format(len(solution.bins)), unsafe_allow_html=True)
    
    with col2:
        avg_occupation = sum(bin.get_occupation() for bin in solution.bins) / len(solution.bins) if solution.bins else 0
        st.markdown("""
        <div class="dashboard-card card-green">
            <div class="card-title">Ocupação Média</div>
            <div class="card-value">{:.2f}%</div>
            <div class="card-subtitle">Utilização média dos bins</div>
        </div>
        """.format(avg_occupation * 100), unsafe_allow_html=True)
    
    with col3:
        st.markdown("""
        <div class="dashboard-card card-blue">
            <div class="card-title">Avaliação da Solução</div>
            <div class="card-value">{:.4f}</div>
            <div class="card-subtitle">Valor da função objetivo</div>
        </div>
        """.format(solution.evaluate()), unsafe_allow_html=True)
    
    # Visualização dos bins
    st.markdown("### Distribuição dos Itens nos Bins")
    
    # Usando Plotly para visualizar os bins
    fig = make_subplots(rows=1, cols=1)
    
    # Criar dados para o gráfico
    bin_ids = [f"Bin {i}" for i in range(len(solution.bins))]
    bin_occupations = [bin.get_occupation() * 100 for bin in solution.bins]
    bin_capacities = [100 for _ in solution.bins]  # 100% de capacidade
    
    # Adicionar barras para ocupação
    fig.add_trace(
        go.Bar(
            x=bin_ids,
            y=bin_occupations,
            name='Ocupação',
            marker_color='rgba(109, 40, 217, 0.8)',
            text=[f"{occ:.1f}%" for occ in bin_occupations],
            textposition='auto'
        )
    )
    
    # Adicionar linha para capacidade
    fig.add_trace(
        go.Scatter(
            x=bin_ids,
            y=bin_capacities,
            mode='lines',
            name='Capacidade',
            line=dict(color='rgba(239, 68, 68, 0.8)', width=2, dash='dash')
        )
    )
    
    # Atualizar layout
    fig.update_layout(
        title='Ocupação dos Bins',
        xaxis_title='Bins',
        yaxis_title='Ocupação (%)',
        template='plotly_dark',
        plot_bgcolor='rgba(31, 41, 55, 0.5)',
        paper_bgcolor='rgba(31, 41, 55, 0.0)',
        font=dict(color='white'),
        height=400,
        yaxis=dict(range=[0, 105])  # Ajusta o eixo y para mostrar um pouco mais que 100%
    )
    
    st.plotly_chart(fig, use_container_width=True)
    
    # Detalhes de cada bin
    st.markdown("### Detalhes dos Bins")
    
    # Exibe os bins em tabelas usando expansores para não ocupar muito espaço
    num_cols = 3
    cols = st.columns(num_cols)
    
    for i, bin in enumerate(solution.bins):
        col_idx = i % num_cols
        with cols[col_idx]:
            with st.expander(f"Bin {i} - Ocupação: {bin.current_load:.2f}/{bin.capacity:.2f}"):
                # Criar tabela com os itens do bin
                items_data = [{
                    "ID": item.id,
                    "Tamanho": f"{item.size:.2f}"
                } for item in sorted(bin.items, key=lambda x: x.id)]
                
                if items_data:
                    st.table(pd.DataFrame(items_data))
                else:
                    st.info("Bin vazio")
    
    # Se houver histórico, exibir gráfico de evolução
    if history:
        st.markdown("### Evolução da Busca Local")
        
        # Dados para o gráfico
        iterations = [h[0] for h in history]
        bin_counts = [h[1] for h in history]
        evals = [h[2] for h in history]
        
        # Criar gráfico com Plotly
        fig = make_subplots(specs=[[{"secondary_y": True}]])
        
        # Número de bins
        fig.add_trace(
            go.Scatter(
                x=iterations,
                y=bin_counts,
                name='Número de Bins',
                line=dict(color='rgba(109, 40, 217, 0.8)', width=3)
            ),
            secondary_y=False
        )
        
        # Valor da função de avaliação
        fig.add_trace(
            go.Scatter(
                x=iterations,
                y=evals,
                name='Avaliação',
                line=dict(color='rgba(16, 185, 129, 0.8)', width=2, dash='dot')
            ),
            secondary_y=True
        )
        
        # Atualizar layout
        fig.update_layout(
            title='Evolução da Busca Local',
            xaxis_title='Iterações',
            template='plotly_dark',
            plot_bgcolor='rgba(31, 41, 55, 0.5)',
            paper_bgcolor='rgba(31, 41, 55, 0.0)',
            font=dict(color='white'),
            legend=dict(
                orientation="h",
                yanchor="bottom",
                y=1.02,
                xanchor="center",
                x=0.5
            ),
            height=400
        )
        
        # Atualizar títulos dos eixos y
        fig.update_yaxes(title_text="Número de Bins", secondary_y=False)
        fig.update_yaxes(title_text="Valor da Função de Avaliação", secondary_y=True)
        
        st.plotly_chart(fig, use_container_width=True)