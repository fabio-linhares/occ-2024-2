import streamlit as st
import os
from PIL import Image
import sys
from typing import List, Tuple
import time
import random
import pandas as pd
import numpy as np
import altair as alt
# Adicionar importações necessárias para a geração da paleta de cores
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

# Importações do módulo bin_packing
from bin_packing import (
    Item, Solution, first_fit_decreasing, verificar_viabilidade_inicial,
    load_instance_from_buffer, HybridMetaheuristic, executar_heuristica
)

# Adicionar o diretório raiz ao path para permitir importações absolutas
sys.path.append("/home/zerocopia/Projetos/occ-2024-2/b1")

# Importar as páginas usando caminho absoluto
from app.pages import about, main_app, apresentacao
# Como ilp.py está no mesmo diretório que main.py, podemos importar diretamente
import ilp

# Configuração da página
st.set_page_config(
    page_title="Meta-heurística para o Problema de Bin Packing",
    page_icon="📦",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Carregando e aplicando o CSS personalizado
def load_css(css_file):
    import os
    # Obter o caminho absoluto do diretório atual do script
    dir_path = os.path.dirname(os.path.realpath(__file__))
    # Construir o caminho completo para o arquivo CSS
    css_path = os.path.join(dir_path, css_file)
    with open(css_path, "r") as f:
        st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

# Função para carregar logos
def load_logos():
    # Obter o caminho absoluto do diretório atual do script
    dir_path = os.path.dirname(os.path.realpath(__file__))
    # Construir o caminho completo para o diretório de logos
    logos_path = os.path.join(dir_path, "..", "data", "logos")
    
    # Dicionário para armazenar logos válidos
    logos_dict = {}
    
    # Verificar se o diretório existe
    if os.path.exists(logos_path):
        # Tentar carregar cada logo
        for logo_file in ["ufal.png", "ic.png", "ppgi.png"]:
            logo_path = os.path.join(logos_path, logo_file)
            if os.path.exists(logo_path):
                try:
                    logo = Image.open(logo_path)
                    # Remover a extensão para usar como chave
                    logo_name = os.path.splitext(logo_file)[0]
                    logos_dict[logo_name] = logo
                except Exception as e:
                    print(f"Erro ao carregar logo {logo_file}: {str(e)}")
    
    # Retorna o dicionário de logos válidos
    return logos_dict

def verify_solution(solution: List[List[Tuple[int, float]]], items: List[float]) -> bool:
    """
    Verifica se a solução é válida, ou seja, se todos os itens estão alocados
    e se a capacidade dos bins não é excedida.
    
    Args:
        solution: A solução a ser verificada
        items: Lista de tamanhos dos itens
        
    Returns:
        True se a solução for válida, False caso contrário
    """
    # Verificar se todos os itens estão na solução
    solution_items = []
    for bin_items in solution:
        for idx, size in bin_items:
            solution_items.append((idx, size))
    
    if len(solution_items) != len(items):
        return False
    
    # Verificar se as capacidades dos bins não são excedidas
    for bin_items in solution:
        bin_total = sum(size for _, size in bin_items)
        if bin_total > 1.0 + 1e-10:  # Tolerância para erros de ponto flutuante
            return False
    
    return True

def main():
    # Carregar CSS
    load_css("styles/custom.css")
    
    # Adicionar suporte para LaTeX com MathJax
    st.markdown(r"""
    <script src="https://polyfill.io/v3/polyfill.min.js?features=es6"></script>
    <script id="MathJax-script" async src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"></script>
    """, unsafe_allow_html=True)
    
    # Esconder as informações não desejadas e aplicar estilos personalizados
    hide_streamlit_style = """
    <style>
    #MainMenu {visibility: hidden;}
    .stTabs [data-baseweb="tab-list"] button [data-testid="stMarkdownContainer"] p {
        font-size:16px;
    }
    /* Remove a barra horizontal no topo */
    header {
        background-color: transparent !important;
        border-bottom: none !important;
    }
    /* Ajusta o título para ficar mais próximo do topo */
    .block-container {
        padding-top: 1rem !important;
    }
    /* Deixa o título com estilo mais destacado */
    h1:first-of-type {
        margin-top: 0 !important;
        padding-top: 0 !important;
        font-size: 2.5rem !important;
        color: var(--primary-color) !important;
        text-shadow: 1px 1px 2px rgba(0,0,0,0.7);
    }
    /* Ocultar seletor de tema na barra superior */
    div[data-testid="stToolbar"] button[aria-label="View theme"] {
        display: none !important;
    }
    /* Remover qualquer barra de cores no topo */
    header::before, 
    body::before,
    .main::before,
    #root::before,
    [data-testid="stAppViewContainer"]::before,
    [data-testid="stHeader"]::before {
        content: none !important;
        background: none !important;
        display: none !important;
    }
    </style>
    """
    st.markdown(hide_streamlit_style, unsafe_allow_html=True)
    
    # Sidebar
    st.sidebar.title("Navegação")
    
    # Logos na sidebar
    logos = load_logos()
    if logos:  # Verifica se há logos válidos antes de tentar exibi-los
        # Primeiro exibimos o logo da UFAL em tamanho completo
        if 'ufal' in logos:
            st.sidebar.image(logos['ufal'], use_container_width=True, caption=None)
        
        # Em seguida, exibimos os logos IC e PPGI lado a lado
        col1, col2 = st.sidebar.columns(2)
        if 'ic' in logos:
            col1.image(logos['ic'], use_container_width=True, caption=None)
        if 'ppgi' in logos:
            col2.image(logos['ppgi'], use_container_width=True, caption=None)
    
    # Usando abas principais em vez de radio buttons
    tabs_main = st.tabs(["Apresentação", "Heurísticas", "PLI", "Sobre"])
    
    with tabs_main[0]:  # Aba Apresentação (era Introdução)
        apresentacao.app()
    
    ###################################
    # Aba Heurísticas - Solução
    with tabs_main[1]:  # Aba Heurísticas
        st.title("Heurísticas para Bin Packing")
        # Sub-abas dentro da aba Heurísticas
        subtabs_heuristica = st.tabs(["Apresentação", "Solução"])
        
        with subtabs_heuristica[0]:  # Sub-aba Apresentação
            st.markdown(r"""
            # Meta-heurísticas para o Problema de Bin Packing

            ## Fundamentação Teórica
            
            O problema de Bin Packing (BPP) é um problema de otimização combinatória classificado como **NP-difícil no sentido forte**. 
            Formalmente, o problema pode ser definido como:

            Dado um conjunto de $n$ itens, cada um com um tamanho $s_i \in (0, 1]$, e bins (recipientes) idênticos de capacidade 1,
            o objetivo é empacotar todos os itens utilizando o menor número possível de bins, garantindo que a soma dos tamanhos 
            dos itens em cada bin não exceda a capacidade.

            ### Formulação Matemática

            O BPP pode ser formulado como um problema de Programação Linear Inteira:

            $$\min \sum_{j=1}^{m} y_j$$

            Sujeito a:
            
            $$\sum_{j=1}^{m} x_{ij} = 1, \quad \forall i \in \{1, 2, ..., n\}$$
            
            $$\sum_{i=1}^{n} s_i x_{ij} \leq y_j, \quad \forall j \in \{1, 2, ..., m\}$$
            
            $$x_{ij} \in \{0, 1\}, \quad \forall i \in \{1, 2, ..., n\}, \forall j \in \{1, 2, ..., m\}$$
            
            $$y_j \in \{0, 1\}, \quad \forall j \in \{1, 2, ..., m\}$$

            Onde:
            - $y_j = 1$ se o bin $j$ for utilizado, 0 caso contrário
            - $x_{ij} = 1$ se o item $i$ for alocado ao bin $j$, 0 caso contrário
            - $s_i$ é o tamanho do item $i$
            - $m$ é um limite superior para o número de bins (geralmente $m = n$)

            ## Estado da Arte em Heurísticas para BPP

            Devido à natureza NP-difícil do problema, várias heurísticas foram desenvolvidas para encontrar 
            soluções aproximadas em tempo computacional razoável.
            """)
            
            # Dividindo em colunas para organizar melhor o conteúdo
            col1, col2 = st.columns(2)
            
            with col1:
                st.markdown(r"""
                ### Heurísticas Construtivas Clássicas
                
                1. **First Fit (FF)**
                   - Coloca cada item no primeiro bin onde ele cabe
                   - Complexidade: $O(n \log n)$
                   
                2. **Best Fit (BF)**
                   - Coloca cada item no bin mais cheio onde ele ainda cabe
                   - Complexidade: $O(n \log n)$
                   
                3. **First Fit Decreasing (FFD)**
                   - Ordena os itens em ordem decrescente de tamanho
                   - Aplica First Fit à lista ordenada
                   - Garantia teórica: $FFD(I) \leq \frac{11}{9} OPT(I) + 1$
                   
                4. **Best Fit Decreasing (BFD)**
                   - Ordena os itens em ordem decrescente de tamanho
                   - Aplica Best Fit à lista ordenada
                   - Desempenho similar ao FFD na prática
                """)
                
            with col2:
                st.markdown(r"""
                ### Meta-heurísticas Avançadas
                
                1. **Busca Local**
                   - Explora o espaço de soluções através de pequenas modificações
                   - Estruturas de vizinhança: movimentos de itens entre bins
                   
                2. **Simulated Annealing (SA)**
                   - Inspirado no processo físico de recozimento de metais
                   - Aceita soluções piores com probabilidade controlada para escapar de ótimos locais
                   - Probabilidade de aceitação: $P = e^{-\Delta/T}$
                   
                3. **Iterated Local Search (ILS)**
                   - Aplica perturbações quando a busca local estagna
                   - Permite explorar diferentes regiões do espaço de busca
                   
                4. **Algoritmos Genéticos e Evolutivos**
                   - Utilizam operadores inspirados na evolução natural
                   - Crossover, mutação e seleção para evolução da população
                """)
            
            st.markdown(r"""
            ## Nossa Abordagem: Meta-heurística Híbrida
            
            Implementamos uma meta-heurística híbrida que combina elementos de diferentes abordagens para obter um algoritmo 
            mais robusto e eficiente:

            ### Componentes da Meta-heurística
            
            1. **Inicialização**
               - Solução inicial gerada com First Fit Decreasing (FFD)
               
            2. **Busca Local**
               - Exploração sistemática da vizinhança através de movimentos de itens entre bins
               - Dois tipos de estratégia: Best Improvement e First Improvement
               
            3. **Mecanismos para Escapar de Ótimos Locais**
               - **Simulated Annealing**: Aceita soluções piores com probabilidade $P = e^{-\Delta/T}$
               - **Perturbações Inteligentes**: Quatro tipos de perturbações com intensidade adaptativa
               - **Estratégia de Reinício**: Quando não há progresso após várias iterações
            """)
            
            col1, col2 = st.columns(2)
            
            with col1:
                st.markdown(r"""
                ### Tipos de Perturbação Implementados
                
                1. **Move**
                   - Move um item aleatório de um bin para outro
                   - Perturbação de baixa intensidade
                   
                2. **Swap**
                   - Troca itens entre dois bins diferentes
                   - Mantém o número de bins constante
                   
                3. **Redistribute**
                   - Redistribui todos os itens de um bin para outros bins
                   - Potencial para reduzir o número total de bins
                   
                4. **Merge-Split**
                   - Combina dois bins e redistribui os itens otimamente
                   - Perturbação de alta intensidade que pode levar a reorganizações significativas
                """)
                
            with col2:
                st.markdown(r"""
                ### Esquema de Resfriamento (SA)
                
                A temperatura do Simulated Annealing é controlada pela fórmula:
                
                $$T_{k+1} = \alpha \times T_k$$
                
                Onde:
                - $T_k$ é a temperatura na iteração $k$
                - $\alpha$ é a taxa de resfriamento (0.95 em nossa implementação)
                - Temperatura inicial $T_0 = 10.0$
                - Temperatura mínima $T_{min} = 0.01$
                
                A probabilidade de aceitar uma solução pior é:
                
                $$P(aceitar) = e^{-\Delta/T}$$
                
                Onde $\Delta$ é a diferença entre a nova solução e a atual.
                """)
                
            st.image("https://www.mecs-press.org/ijieeb/ijieeb-v4-n2/IJIEEB-V4-N2-2_files/image002.jpg", 
                 caption="Exemplo de empacotamento de itens em bins", width=500)
            
            st.markdown(r"""
            ## Comparação Teórica de Desempenho

            Para instâncias com $n$ itens, as complexidades teóricas são:
            
            | Algoritmo | Complexidade | Razão de Aproximação |
            |-----------|-------------|---------------------|
            | First Fit | $O(n \log n)$ | $FF(I) \leq 1.7 \times OPT(I) + 0.7$ |
            | Best Fit | $O(n \log n)$ | $BF(I) \leq 1.7 \times OPT(I) + 0.7$ |
            | First Fit Decreasing | $O(n \log n)$ | $FFD(I) \leq \frac{11}{9} \times OPT(I) + 1$ |
            | Nossa Meta-heurística | Depende do tempo limite | Não há garantia teórica, mas empiricamente obtém soluções próximas do ótimo |

            A abordagem híbrida implementada consegue escapar de ótimos locais e explorar eficientemente o espaço de busca, 
            resultando em soluções de alta qualidade para o problema de Bin Packing.
            
            ## Referências
            
            1. Martello, S., & Toth, P. (1990). Knapsack problems: algorithms and computer implementations. John Wiley & Sons.
            
            2. Coffman Jr, E. G., Csirik, J., Galambos, G., Martello, S., & Vigo, D. (2013). Bin packing approximation algorithms: survey and classification. Handbook of combinatorial optimization, 455-531.
            
            3. Lourenço, H. R., Martin, O. C., & Stützle, T. (2003). Iterated local search. Handbook of metaheuristics, 320-353.
            
            4. Kirkpatrick, S., Gelatt, C. D., & Vecchi, M. P. (1983). Optimization by simulated annealing. Science, 220(4598), 671-680.
            """)
            
            main_app.app()

    with subtabs_heuristica[1]:  # Sub-aba Solução
        st.header("Meta-heurística de Solução Única para Bin Packing")
        st.markdown("""
        Esta implementação resolve o problema de Bin Packing utilizando uma meta-heurística baseada em busca local.
        Você pode carregar um arquivo com instâncias do problema e definir um tempo limite para a execução.

        ### Formato do arquivo de entrada:
        - Cada linha contém um número entre 0 e 1, representando o tamanho de um item.
        - Exemplo: 0.42, 0.25, 0.15, 0.31, 0.12, 0.5, 0.18
        """)

        # Interface para upload de arquivo e configuração
        col1, col2, col3 = st.columns([2, 1, 1])

        with col1:
            uploaded_file = st.file_uploader("Carregar arquivo de instância", type=["txt", "csv"])
            
        with col2:
            time_limit = st.number_input("Tempo limite (segundos)", min_value=1, value=10)
            
        with col3:
            max_bins = st.number_input("Número máximo de bins", min_value=1, value=100, 
                                    help="Define o número máximo de bins disponíveis. O algoritmo tentará usar o mínimo possível.")

        def generate_color_palette(num_items):
            """Gera uma paleta de cores para os itens"""
            # Usar uma combinação de paletas coloridas para ter mais cores distintas
            base_colors = list(plt.cm.tab20.colors) + list(plt.cm.tab20b.colors) + list(plt.cm.tab20c.colors)
            
            if num_items <= len(base_colors):
                colors = base_colors[:num_items]
            else:
                # Se precisar de mais cores, gerar aleatoriamente
                colors = base_colors.copy()
                needed = num_items - len(colors)
                for _ in range(needed):
                    colors.append((random.random(), random.random(), random.random()))
            
            # Converter para formato hexadecimal para uso no Altair
            hex_colors = [mcolors.rgb2hex(color[:3]) for color in colors]
            return hex_colors

        def create_bins_visualization(solution, color_palette):
            """Cria visualização detalhada dos bins"""
            # Dados para a visualização
            bin_data = []
            items_data = []
            
            for i, bin in enumerate(solution.bins):
                bin_usage = bin.current_load
                bin_free = 1.0 - bin_usage
                
                # Adicionar bin ao dataset
                bin_data.append({
                    "Bin": f"Bin {i+1}",
                    "Utilização": bin_usage * 100,
                    "Espaço Livre": bin_free * 100,
                    "Itens": len(bin.items)
                })
                
                # Posição atual no bin (para empilhamento)
                position = 0
                
                # Adicionar cada item individualmente
                for item in sorted(bin.items, key=lambda x: x.id):
                    items_data.append({
                        "Bin": f"Bin {i+1}",
                        "Item": f"Item {item.id+1}",
                        "Tamanho": item.size,
                        "Tamanho (%)": item.size * 100,
                        "Posição": position,
                        "Posição Final": position + item.size,
                        "Color": color_palette[item.id % len(color_palette)]
                    })
                    position += item.size
            
            # Criar DataFrames
            bin_df = pd.DataFrame(bin_data)
            items_df = pd.DataFrame(items_data)
            
            return bin_df, items_df

        if uploaded_file is not None:
            strategy = st.radio("Estratégia de busca local", ["Best Improvement", "First Improvement"])
            
            # Opções avançadas em um expander
            with st.expander("Opções avançadas"):
                use_sa = st.checkbox("Usar Simulated Annealing", value=True, 
                                    help="Permite aceitar soluções piores para escapar de ótimos locais")
                detailed_viz = st.checkbox("Visualização detalhada", value=True,
                                        help="Mostra cada item com cores distintas dentro dos bins")
            
            if st.button("Executar Meta-heurística"):
                try:
                    with st.spinner("Executando meta-heurística..."):
                        # Carregar instância
                        items = load_instance_from_buffer(uploaded_file)
                        
                        if not items:
                            st.error("Arquivo vazio ou formato inválido. Verifique se o arquivo contém números entre 0 e 1.")
                        else:
                            st.write(f"Carregados {len(items)} itens.")
                            
                            # Gerar paleta de cores para os itens
                            color_palette = generate_color_palette(len(items))
                            
                            # Configurar e executar a meta-heurística
                            improvement_strategy = "best" if strategy == "Best Improvement" else "first"
                            metaheuristic = HybridMetaheuristic(
                                time_limit_seconds=time_limit, 
                                use_sa=use_sa,
                                improvement=improvement_strategy
                            )
                            
                            best_solution, stats, history = metaheuristic.run(items, max_bins)
                            
                            # Verificar validade da solução
                            is_valid = best_solution.verify()
                            
                            if is_valid:
                                st.success(f"Solução válida encontrada com {len(best_solution.bins)} bins em {stats['iterations']} iterações!")
                                
                                # Métricas e estatísticas
                                col1, col2, col3, col4 = st.columns(4)
                                col1.metric("Bins utilizados", f"{len(best_solution.bins)}")
                                
                                # Calcular utilização média
                                bin_usages = [bin.current_load for bin in best_solution.bins]
                                avg_utilization = sum(bin_usages) / len(best_solution.bins) if best_solution.bins else 0
                                col2.metric("Utilização média", f"{avg_utilization:.2%}")
                                
                                # Variância da utilização
                                utilization_var = np.var(bin_usages) if len(best_solution.bins) > 1 else 0
                                col3.metric("Variância de utilização", f"{utilization_var:.4f}")
                                
                                # Tempo até melhor solução
                                col4.metric("Tempo até melhor solução", f"{stats['time_to_best']:.2f}s")
                                
                                # Segunda linha de métricas
                                col1, col2, col3, col4 = st.columns(4)
                                col1.metric("Perturbações aplicadas", f"{stats['perturbations']}")
                                col2.metric("Melhorias encontradas", f"{stats['improvements']}")
                                col3.metric("Reinícios executados", f"{stats['restarts']}")
                                col4.metric("Taxa de aceitação SA", f"{stats['acceptance_rate']:.2f}%")
                                
                                # Visualização detalhada dos bins
                                st.subheader("Distribuição dos Itens nos Bins")
                                
                                if detailed_viz:
                                    # Visualização detalhada com cores para cada item
                                    bin_df, items_df = create_bins_visualization(best_solution, color_palette)
                                    
                                    # Gráfico de barras empilhadas para visualizar itens dentro dos bins
                                    chart = alt.Chart(items_df).mark_bar().encode(
                                        x=alt.X('Bin:N', sort=None, title='Bins'),
                                        y=alt.Y('Tamanho (%):Q', title='Utilização (%)'),
                                        color=alt.Color('Item:N', scale=alt.Scale(range=color_palette.copy())),
                                        tooltip=['Bin', 'Item', 'Tamanho']
                                    ).properties(
                                        width=600,
                                        height=400,
                                        title='Distribuição dos Itens nos Bins'
                                    )
                                    
                                    # Linha indicando capacidade máxima
                                    capacity_line = alt.Chart(pd.DataFrame({'y': [100]})).mark_rule(
                                        color='red', strokeDash=[3, 3]
                                    ).encode(y='y')
                                    
                                    st.altair_chart(chart + capacity_line, use_container_width=True)
                                else:
                                    # Visualização simplificada
                                    bin_data = []
                                    for i, bin in enumerate(best_solution.bins):
                                        bin_data.append({
                                            "Bin": f"Bin {i+1}",
                                            "Utilização": bin.current_load * 100,
                                            "Itens": len(bin.items)
                                        })
                                    
                                    # DataFrame para visualização
                                    bin_df = pd.DataFrame(bin_data)
                                    
                                    # Gráfico de barras para utilização dos bins
                                    chart = alt.Chart(bin_df).mark_bar().encode(
                                        x=alt.X('Bin:N', sort=None),
                                        y=alt.Y('Utilização:Q', title='Utilização (%)', scale=alt.Scale(domain=[0, 100])),
                                        color=alt.Color('Utilização:Q', scale=alt.Scale(scheme='viridis'), legend=None),
                                        tooltip=['Bin', 'Utilização', 'Itens']
                                    ).properties(
                                        width=600,
                                        height=400,
                                        title='Utilização dos Bins (%)'
                                    )
                                    
                                    st.altair_chart(chart, use_container_width=True)
                                
                                # Gráfico de convergência
                                st.subheader("Convergência do Algoritmo")
                                history_df = pd.DataFrame({
                                    'Iteração': range(len(history)),
                                    'Número de Bins': history
                                })
                                
                                convergence_chart = alt.Chart(history_df).mark_line().encode(
                                    x='Iteração:Q',
                                    y=alt.Y('Número de Bins:Q', scale=alt.Scale(zero=False)),
                                    tooltip=['Iteração', 'Número de Bins']
                                ).properties(
                                    width=600,
                                    height=300,
                                    title='Progresso da Busca Local'
                                )
                                
                                st.altair_chart(convergence_chart, use_container_width=True)
                                
                                # Detalhes da solução
                                with st.expander("Ver detalhes da solução"):
                                    for i, bin in enumerate(best_solution.bins):
                                        st.write(f"Bin {i+1}: {len(bin.items)} itens, utilização: {bin.current_load*100:.2f}%")
                                        
                                        # Criar lista de itens formatada com cores
                                        items_html = ""
                                        for item in sorted(bin.items, key=lambda x: x.id):
                                            color_hex = color_palette[item.id % len(color_palette)]
                                            items_html += f'<span style="background-color:{color_hex}; padding:2px 8px; margin:2px; border-radius:3px; display:inline-block; color:white;">Item {item.id+1} ({item.size:.2f})</span>'
                                        
                                        st.write(f"  Itens: {items_html}", unsafe_allow_html=True)
                            else:
                                # Verificar se o problema pode estar no número máximo de bins
                                total_item_size = sum(item.size for item in items)
                                min_theoretical_bins = int(total_item_size) + (1 if total_item_size % 1 > 0 else 0)
                                
                                if max_bins < min_theoretical_bins:
                                    st.error(f"A solução é inválida porque o número máximo de bins ({max_bins}) é insuficiente. " 
                                            f"Baseado no tamanho total dos itens ({total_item_size:.2f}), "
                                            f"são necessários pelo menos {min_theoretical_bins} bins teóricos. "
                                            f"Por favor, aumente o valor de 'Número máximo de bins'.")
                                
                                    # Mostrar a explicação do cálculo do limite inferior
                                    st.info(f"**Explicação do limite inferior:**  \n"
                                            f"Soma dos tamanhos de todos os itens = {total_item_size:.2f}  \n"
                                            f"Como cada bin tem capacidade máxima 1.0, precisamos de pelo menos "
                                            f"⌈{total_item_size:.2f}⌉ = {min_theoretical_bins} bins.  \n"
                                            f"Note que este é apenas um limite teórico mínimo. "
                                            f"Devido à natureza combinatória do problema, a solução ótima "
                                            f"geralmente requer mais bins que este limite.")
                                else:
                                    st.error("A solução encontrada é inválida. Isso pode ocorrer devido a limitações do algoritmo "
                                            "ou restrições muito rígidas. Tente aumentar o tempo limite ou o número máximo de bins.")
                except Exception as e:
                    st.error(f"Erro durante a execução: {str(e)}")
                    st.exception(e)  # Mostra stack trace do erro para depuração
        else:
            st.info("Carregue um arquivo de instância para começar.")
    
    ###################################
    # Aba PLI - Programação Linear Inteira    
    with tabs_main[2]:  # Aba PLI
        st.title("Programação Linear Inteira")
        subtabs_pli = st.tabs(["Apresentação", "Solução"])
        
        with subtabs_pli[0]:  # Sub-aba Apresentação
            st.header("Apresentação da PLI")
            st.markdown("""
            ## Programação Linear Inteira para o Problema de Bin Packing
            
            A Programação Linear Inteira (PLI) é uma abordagem matemática exata para resolução de problemas de otimização
            como o Bin Packing. Diferentemente das heurísticas, a PLI garante a obtenção da solução ótima (quando convergente),
            porém com maior custo computacional.
            """)
            
        with subtabs_pli[1]:  # Sub-aba Solução
            ilp.app()
    
    ###################################
    # Aba Sobre 
    with tabs_main[3]:  # Aba Sobre
        about.app()
    

    ###################################
    # Footer  
    st.sidebar.markdown("---")
    st.sidebar.info("""
    **Desenvolvido por:** Fábio Linhares  
    **Universidade:** UFAL  
    **Programa:** PPGI
    
    [Repositório GitHub](https://github.com/fabio-linhares/occ-2024-2/tree/main/b1)
    
    **v1.0** - Abril/2025
    """)

if __name__ == "__main__":
    main()