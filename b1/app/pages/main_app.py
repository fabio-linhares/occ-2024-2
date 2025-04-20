# Standard library
import datetime
import os
import time

# Third‑party libraries
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import streamlit as st

# Local application imports
from bin_packing import *


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
    st.markdown("""
    ### Formulação Matemática
    
    O Bin Packing pode ser formulado como um problema de programação linear inteira:
    """)
    
    # Cards para variáveis de decisão, função objetivo e restrições
    col1, col2 = st.columns(2)
    
    with col1:
        st.markdown("""
        <div style="border-radius: 10px; padding: 15px; background-color: rgba(109, 40, 217, 0.2);">
            <h4 style="font-weight: bold; margin-top: 0;">Variáveis de decisão:</h4>
            <ul style="margin-bottom: 0;">
                <li>$y_j$: 1 se o bin $j$ é utilizado, 0 caso contrário</li>
                <li>$x_{ij}$: 1 se o item $i$ é alocado ao bin $j$, 0 caso contrário</li>
            </ul>
        </div>
        """, unsafe_allow_html=True)
    
    with col2:
        st.markdown("""
        <div style="border-radius: 10px; padding: 15px; background-color: rgba(59, 130, 246, 0.2);">
            <h4 style="font-weight: bold; margin-top: 0;">Função objetivo:</h4>
            <div style="text-align: center; font-size: 1.2em; padding: 10px;">
            $$\\min \\sum_{j \\in J} y_j$$
            </div>
            <p style="font-style: italic; margin-top: 5px; text-align: center; font-size: 0.9em;">
            Minimizar o número total de bins utilizados
            </p>
        </div>
        """, unsafe_allow_html=True)
    
    st.markdown("""
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
    
    st.markdown("""
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


 
    # Seção para carregar instâncias
    st.markdown("<h2 style='color: #E5E7EB; margin-top: 2rem;'>🔢 Configurar Instância</h2>", unsafe_allow_html=True)
    
    # Caminhos para os diretórios relevantes
    app_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    instances_dir = os.path.join(app_dir, "data", "instances")
    os.makedirs(instances_dir, exist_ok=True)
    
    # Modo de instância (arquivo ou geração aleatória)
    instance_mode = st.radio(
        "Selecione o modo de instância:",
        ["Carregar de arquivo", "Gerar aleatoriamente"]
    )
    
    items = []
    instance_name = ""
    
    if instance_mode == "Carregar de arquivo":
        # Lista de instâncias disponíveis
        instance_files = [f for f in os.listdir(instances_dir) if f.endswith('.txt')] if os.path.exists(instances_dir) else []
        
        if not instance_files:
            st.warning("Nenhuma instância encontrada no diretório 'data/instances'. Por favor, adicione arquivos .txt para proceder.")
        else:
            # Mostrar as instâncias disponíveis
            selected_instance = st.selectbox("Selecione uma instância:", instance_files)
            instance_name = os.path.splitext(selected_instance)[0]
            
            # Botão para carregar a instância
            if st.button("📂 Carregar Instância", key="load_instance"):
                with st.spinner("Carregando instância..."):
                    instance_path = os.path.join(instances_dir, selected_instance)
                    try:
                        items = read_instance(instance_path)
                        st.success(f"Instância '{selected_instance}' carregada com {len(items)} itens.")
                        st.session_state.items = items
                        st.session_state.instance_name = instance_name
                    except Exception as e:
                        st.error(f"Erro ao carregar instância: {str(e)}")
    else:
        col1, col2 = st.columns(2)
        
        with col1:
            n_items = st.slider("Número de itens:", 10, 500, 50)
        
        with col2:
            min_size = st.slider("Tamanho mínimo (0-1):", 0.0, 0.5, 0.1, 0.01)
            max_size = st.slider("Tamanho máximo (0-1):", min_size, 1.0, 0.7, 0.01)
        
        instance_name = f"random_{n_items}_{min_size:.2f}_{max_size:.2f}"
        
        if st.button("🎲 Gerar Instância Aleatória", key="generate_instance"):
            with st.spinner("Gerando instância aleatória..."):
                try:
                    items = generate_random_instance(n_items, min_size, max_size)
                    st.success(f"Instância aleatória gerada com {len(items)} itens.")
                    st.session_state.items = items
                    st.session_state.instance_name = instance_name
                except Exception as e:
                    st.error(f"Erro ao gerar instância: {str(e)}")
    
    # Usar instância salva na session_state se disponível
    if not items and 'items' in st.session_state:
        items = st.session_state.items
        instance_name = st.session_state.instance_name if 'instance_name' in st.session_state else instance_name
    
    # Se temos itens, mostrar informações e opções da meta-heurística
    if items:
        # Visualização dos dados
        st.markdown("<h2 style='color: #E5E7EB; margin-top: 2rem;'>📊 Dados da Instância</h2>", unsafe_allow_html=True)
        
        # Transformar os itens em um DataFrame para visualização
        items_df = pd.DataFrame({
            "ID": [item.id for item in items],
            "Tamanho": [item.size for item in items]
        })
        
        col1, col2 = st.columns([2, 3])
        
        with col1:
            # Estatísticas básicas
            total_size = sum(item.size for item in items)
            avg_size = np.mean([item.size for item in items])
            min_bins = int(np.ceil(total_size))
            
            st.markdown("""
            <div class="info-box blue">
                <h3>Estatísticas da Instância</h3>
                <ul>
                    <li><strong>Total de itens:</strong> {}</li>
                    <li><strong>Tamanho médio:</strong> {:.4f}</li>
                    <li><strong>Soma dos tamanhos:</strong> {:.4f}</li>
                    <li><strong>Bins necessários (limite inferior teórico):</strong> {}</li>
                </ul>
            </div>
            """.format(len(items), avg_size, total_size, min_bins), unsafe_allow_html=True)
            
            # Amostra dos dados
            st.markdown("### Amostra dos dados:")
            st.dataframe(items_df.head(10), height=300)
        
        with col2:
            # Histograma dos tamanhos
            fig, ax = plt.subplots(figsize=(10, 6))
            
            bins_hist = np.linspace(0, 1, 21)  # 20 bins de 0 a 1
            
            ax.hist([item.size for item in items], bins=bins_hist, color='#6D28D9', alpha=0.7, edgecolor='black')
            ax.set_xlabel('Tamanho do Item', fontsize=12)
            ax.set_ylabel('Frequência', fontsize=12)
            ax.set_title('Distribuição dos Tamanhos dos Itens', fontsize=14)
            ax.grid(alpha=0.3)
            
            # Adicionar estatísticas ao gráfico
            props = dict(boxstyle='round', facecolor='#1F2937', alpha=0.5)
            textstr = f"Total: {len(items)}\nMédia: {avg_size:.4f}\nMín: {min([item.size for item in items]):.4f}\nMáx: {max([item.size for item in items]):.4f}"
            ax.text(0.05, 0.95, textstr, transform=ax.transAxes, fontsize=10,
                   verticalalignment='top', bbox=props)
            
            # Melhorar a aparência
            fig.tight_layout()
            st.pyplot(fig)
        
        # Parâmetros da meta-heurística
        st.markdown("<h2 style='color: #E5E7EB; margin-top: 2rem;'>⚙️ Parâmetros da Meta-heurística</h2>", unsafe_allow_html=True)
        
        col1, col2 = st.columns(2)
        
        with col1:
            time_limit = st.slider("Tempo limite (segundos):", 1, 300, 60)
        
        with col2:
            alpha = st.slider("Parâmetro α (balanceamento):", 0.0, 1.0, 0.1, 0.01)
        
        # Executar meta-heurística
        st.markdown("<h2 style='color: #E5E7EB; margin-top: 2rem;'>🚀 Execução</h2>", unsafe_allow_html=True)
        st.write("Clique no botão abaixo para executar a meta-heurística de busca local.")
        
        # Botão centralizado
        col_button = st.columns(3)
        with col_button[1]:
            run_meta = st.button("🚀 Executar Meta-heurística", use_container_width=True, type="primary")
        
        # Se o botão foi clicado, executar a meta-heurística
        if run_meta:
            with st.spinner("Executando meta-heurística..."):
                # Iniciar temporizador
                start_time = time.time()
                
                # Mostrar barra de progresso
                progress = st.progress(0)
                status_text = st.empty()
                
                # Construir solução inicial com FFD
                status_text.text("Construindo solução inicial com First Fit Decreasing...")
                progress.progress(20)
                initial_solution = first_fit_decreasing(items)
                
                # Atualizar progresso
                time.sleep(0.5)  # Pequena pausa para visualização da barra
                progress.progress(40)
                status_text.text("Solução inicial construída. Iniciando busca local...")
                
                # Instanciar busca local com tempo limite e alpha
                local_search = LocalSearch(time_limit, alpha)
                
                # Executar busca local
                best_solution, history = local_search.run(initial_solution)
                
                # Atualizar progresso
                progress.progress(90)
                status_text.text("Busca local concluída. Gerando visualizações...")
                
                # Calcular tempo total
                end_time = time.time()
                total_time = end_time - start_time
                
                # Completar a barra de progresso
                progress.progress(100)
                status_text.empty()
                
                st.success(f"Meta-heurística executada com sucesso! Tempo total: {total_time:.2f} segundos.")
                
                # Salvar resultados na sessão
                st.session_state.initial_solution = initial_solution
                st.session_state.best_solution = best_solution
                st.session_state.solution_history = history
                
                # Exibir resultados
                st.markdown("""
                <div style="border-top: 1px solid rgba(49, 51, 63, 0.2); margin: 1em 0;"></div>
                <h2 style="text-align: center; margin-bottom: 1em; color: #E5E7EB;">🔍 Resultados</h2>
                """, unsafe_allow_html=True)
                
                # Criar tabs para solução inicial e final
                tab1, tab2 = st.tabs(["Solução Inicial (First Fit Decreasing)", "Solução Final (Busca Local)"])
                
                with tab1:
                    display_solution(initial_solution)
                
                with tab2:
                    display_solution(best_solution, history)
                
                # Comparação das soluções
                st.markdown("<h2 style='color: #E5E7EB; margin-top: 2rem;'>📈 Comparação das Soluções</h2>", unsafe_allow_html=True)
                
                col1, col2 = st.columns(2)
                
                initial_bins = len(initial_solution.bins)
                final_bins = len(best_solution.bins)
                initial_eval = initial_solution.evaluate()
                final_eval = best_solution.evaluate()
                
                with col1:
                    # Gráfico comparativo de número de bins
                    fig, ax = plt.subplots(figsize=(8, 5))
                    
                    solutions = ['Inicial (FFD)', 'Final (Busca Local)']
                    bin_counts = [initial_bins, final_bins]
                    
                    ax.bar(solutions, bin_counts, color=['#3B82F6', '#6D28D9'])
                    ax.set_ylabel('Número de Bins', fontsize=12)
                    ax.set_title('Comparação do Número de Bins', fontsize=14)
                    
                    for i, count in enumerate(bin_counts):
                        ax.text(i, count + 0.1, str(count), ha='center', fontsize=12)
                    
                    improvement = ((initial_bins - final_bins) / initial_bins) * 100 if initial_bins > 0 and initial_bins > final_bins else 0
                    plt.figtext(0.5, 0.01, f"Melhoria: {improvement:.2f}%", ha="center", fontsize=12)
                    
                    st.pyplot(fig)
                
                with col2:
                    # Gráfico comparativo de avaliação
                    fig, ax = plt.subplots(figsize=(8, 5))
                    
                    evals = [initial_eval, final_eval]
                    
                    ax.bar(solutions, evals, color=['#3B82F6', '#6D28D9'])
                    ax.set_ylabel('Valor da Função de Avaliação', fontsize=12)
                    ax.set_title('Comparação da Função Objetivo', fontsize=14)
                    
                    for i, eval_val in enumerate(evals):
                        ax.text(i, eval_val + 0.01, f"{eval_val:.4f}", ha='center', fontsize=12)
                    
                    eval_improvement = ((initial_eval - final_eval) / initial_eval) * 100 if initial_eval > 0 and initial_eval > final_eval else 0
                    plt.figtext(0.5, 0.01, f"Melhoria: {eval_improvement:.2f}%", ha="center", fontsize=12)
                    
                    st.pyplot(fig)
                
                # Salvar solução
                st.markdown("<h2 style='color: #E5E7EB; margin-top: 2rem;'>💾 Salvar Resultados</h2>", unsafe_allow_html=True)
                
                # Criar diretório de resultados se não existir
                results_dir = os.path.join(app_dir, "results")
                os.makedirs(results_dir, exist_ok=True)
                
                timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                solution_filename = f"{instance_name}_{time_limit}s_{timestamp}.txt"
                solution_path = os.path.join(results_dir, solution_filename)
                
                save_solution(best_solution, instance_name, time_limit, solution_path)
                
                st.success(f"Solução salva em: {solution_filename}")
                
                # Exibir detalhes do arquivo salvo como expander
                with st.expander("📄 Detalhes do arquivo salvo", expanded=False):
                    try:
                        with open(solution_path, "r") as f:
                            solution_content = f.read()
                        
                        st.code(solution_content, language="text")
                    except Exception as e:
                        st.error(f"Erro ao ler o arquivo salvo: {str(e)}")
    
    # Se não há itens carregados, exibir instruções
    else:
        st.info("Selecione ou gere uma instância para começar.")
    
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