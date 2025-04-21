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

# Adicionar o diretório raiz ao path para permitir importações absolutas
sys.path.append("/home/zerocopia/Projetos/occ-2024-2/b1")

# Importar as páginas usando caminho absoluto
from app.pages import about, main_app, apresentacao

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
        
    with tabs_main[1]:  # Aba Heurísticas
        st.title("Heurísticas para Bin Packing")
        # Sub-abas dentro da aba Heurísticas
        subtabs_heuristica = st.tabs(["Apresentação", "Solução"])
        
        with subtabs_heuristica[0]:  # Sub-aba Apresentação
            st.markdown("""
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
                st.markdown("""
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
                st.markdown("""
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
            
            st.markdown("""
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
                st.markdown("""
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
                st.markdown("""
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
            
            st.markdown("""
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

            Solution = List[List[Tuple[int, float]]]

            def load_instance(file) -> List[float]:
                items = []
                for line in file:
                    try:
                        size = float(line.decode('utf-8').strip())
                        if 0 <= size <= 1:
                            items.append(size)
                    except ValueError:
                        continue
                return items

            def initial_solution(items: List[float], max_bins: int = None) -> Solution:
                solution = []
                items_with_index = list(enumerate(items))
                items_with_index.sort(key=lambda x: x[1], reverse=True)
                
                for idx, size in items_with_index:
                    placed = False
                    for bin_idx, bin_items in enumerate(solution):
                        bin_total = sum(item[1] for item in bin_items)
                        if bin_total + size <= 1.0:
                            solution[bin_idx].append((idx, size))
                            placed = True
                            break
                    
                    if not placed:
                        if max_bins is not None and len(solution) >= max_bins:
                            solution[-1].append((idx, size))
                        else:
                            solution.append([(idx, size)])
                
                return solution

            def evaluate_solution(solution: Solution) -> Tuple[int, float, float]:
                num_bins = len(solution)
                bin_usages = [sum(size for _, size in bin_items) for bin_items in solution]
                
                # Calcular utilização média
                avg_utilization = sum(bin_usages) / num_bins if num_bins > 0 else 0
                
                # Calcular variância de utilização (imbalance)
                if num_bins > 1:
                    imbalance = np.var(bin_usages)
                else:
                    imbalance = 0
                
                return num_bins, imbalance, avg_utilization

            def get_neighbors(solution: Solution, max_bins: int = None) -> List[Tuple[Solution, str]]:
                neighbors = []
                
                # Vizinhança 1: Mover um item de um bin para outro
                for bin_idx1, bin1 in enumerate(solution):
                    for item_idx1, (item1, size1) in enumerate(bin1):
                        for bin_idx2, bin2 in enumerate(solution):
                            if bin_idx1 == bin_idx2:
                                continue
                            
                            bin2_total = sum(size for _, size in bin2)
                            if bin2_total + size1 <= 1.0:
                                new_solution = [bin.copy() for bin in solution]
                                item = new_solution[bin_idx1].pop(item_idx1)
                                new_solution[bin_idx2].append(item)
                                
                                if max_bins is None or len([b for b in new_solution if b]) <= max_bins:
                                    new_solution = [bin for bin in new_solution if bin]
                                    neighbors.append((new_solution, f"Move item {item1} do bin {bin_idx1} para bin {bin_idx2}"))
                
                # Vizinhança 2: Trocar dois itens entre bins diferentes
                for bin_idx1, bin1 in enumerate(solution):
                    for item_idx1, (item1, size1) in enumerate(bin1):
                        for bin_idx2, bin2 in enumerate(solution):
                            if bin_idx1 == bin_idx2:
                                continue
                            
                            for item_idx2, (item2, size2) in enumerate(bin2):
                                # Verificar se a troca é viável em termos de capacidade
                                bin1_total = sum(size for _, size in bin1) - size1
                                bin2_total = sum(size for _, size in bin2) - size2
                                
                                if bin1_total + size2 <= 1.0 and bin2_total + size1 <= 1.0:
                                    new_solution = [bin.copy() for bin in solution]
                                    # Troca os itens
                                    new_solution[bin_idx1][item_idx1], new_solution[bin_idx2][item_idx2] = \
                                        new_solution[bin_idx2][item_idx2], new_solution[bin_idx1][item_idx1]
                                    
                                    neighbors.append((new_solution, f"Troca item {item1} do bin {bin_idx1} com item {item2} do bin {bin_idx2}"))
                
                return neighbors

            def perturb_solution(solution: Solution, perturbation_strength: float = 0.3, temperature: float = 1.0) -> Solution:
                """
                Perturba a solução atual para escapar de ótimos locais.
                Combina técnicas de ILS e Simulated Annealing.
                
                Args:
                    solution: Solução atual
                    perturbation_strength: Intensidade da perturbação (0.0 a 1.0)
                    temperature: Temperatura atual (influencia a intensidade da perturbação)
                    
                Returns:
                    Nova solução perturbada
                """
                if not solution:
                    return solution
                
                new_solution = [bin.copy() for bin in solution]
                n_bins = len(new_solution)
                
                if n_bins <= 1:
                    return new_solution
                
                # Calcular número de perturbações baseado na força e temperatura
                n_perturbations = max(1, int(perturbation_strength * temperature * n_bins))
                
                for _ in range(n_perturbations):
                    # Escolher tipo de perturbação
                    perturbation_type = random.choices(
                        ["move", "swap", "redistribute", "merge_split"],
                        weights=[0.4, 0.3, 0.2, 0.1],
                        k=1
                    )[0]
                    
                    if perturbation_type == "move" and n_bins > 1:
                        # Mover um item aleatório de um bin para outro
                        source_bin_idx = random.randint(0, n_bins - 1)
                        if not new_solution[source_bin_idx]:
                            continue
                            
                        target_bin_idx = random.randint(0, n_bins - 1)
                        while target_bin_idx == source_bin_idx:
                            target_bin_idx = random.randint(0, n_bins - 1)
                            
                        item_idx = random.randint(0, len(new_solution[source_bin_idx]) - 1)
                        item = new_solution[source_bin_idx].pop(item_idx)
                        
                        # Verificar capacidade
                        target_bin_total = sum(size for _, size in new_solution[target_bin_idx])
                        if target_bin_total + item[1] <= 1.0:
                            new_solution[target_bin_idx].append(item)
                        else:
                            # Se não couber, criar um novo bin
                            new_solution.append([item])
                    
                    elif perturbation_type == "swap" and n_bins > 1:
                        # Trocar itens entre dois bins aleatórios
                        bin1_idx = random.randint(0, n_bins - 1)
                        bin2_idx = random.randint(0, n_bins - 1)
                        
                        if not new_solution[bin1_idx] or not new_solution[bin2_idx] or bin1_idx == bin2_idx:
                            continue
                            
                        item1_idx = random.randint(0, len(new_solution[bin1_idx]) - 1)
                        item2_idx = random.randint(0, len(new_solution[bin2_idx]) - 1)
                        
                        # Verificar viabilidade da troca
                        item1 = new_solution[bin1_idx][item1_idx]
                        item2 = new_solution[bin2_idx][item2_idx]
                        
                        bin1_total = sum(size for _, size in new_solution[bin1_idx]) - item1[1]
                        bin2_total = sum(size for _, size in new_solution[bin2_idx]) - item2[1]
                        
                        if bin1_total + item2[1] <= 1.0 and bin2_total + item1[1] <= 1.0:
                            new_solution[bin1_idx][item1_idx], new_solution[bin2_idx][item2_idx] = item2, item1
                    
                    elif perturbation_type == "redistribute" and n_bins > 1:
                        # Redistribuir todos os itens de um bin
                        bin_idx = random.randint(0, n_bins - 1)
                        
                        if not new_solution[bin_idx]:
                            continue
                            
                        items_to_redistribute = new_solution[bin_idx].copy()
                        new_solution[bin_idx] = []
                        
                        # Tentar redistribuir em bins existentes
                        for item in items_to_redistribute:
                            placed = False
                            # Tentar bins existentes em ordem aleatória
                            bin_indices = list(range(n_bins))
                            random.shuffle(bin_indices)
                            
                            for target_bin_idx in bin_indices:
                                if target_bin_idx == bin_idx or not new_solution[target_bin_idx]:
                                    continue
                                    
                                bin_total = sum(size for _, size in new_solution[target_bin_idx])
                                if bin_total + item[1] <= 1.0:
                                    new_solution[target_bin_idx].append(item)
                                    placed = True
                                    break
                            
                            if not placed:
                                # Se não couber em nenhum bin existente, criar um novo
                                if not new_solution[bin_idx]:
                                    new_solution[bin_idx].append(item)
                                else:
                                    new_solution.append([item])
                    
                    elif perturbation_type == "merge_split" and n_bins > 1:
                        # Tentar mesclar dois bins e depois dividir em dois novamente
                        bin1_idx = random.randint(0, n_bins - 1)
                        bin2_idx = random.randint(0, n_bins - 1)
                        
                        if bin1_idx == bin2_idx:
                            continue
                            
                        # Juntar todos os itens dos dois bins
                        combined_items = new_solution[bin1_idx] + new_solution[bin2_idx]
                        
                        # Limpar os bins originais
                        new_solution[bin1_idx] = []
                        new_solution[bin2_idx] = []
                        
                        # Realocar os itens combinados nos dois bins
                        # (usando FFD - First Fit Decreasing)
                        combined_items.sort(key=lambda x: x[1], reverse=True)
                        
                        for item in combined_items:
                            # Tentar bin1 primeiro
                            bin1_total = sum(size for _, size in new_solution[bin1_idx])
                            if bin1_total + item[1] <= 1.0:
                                new_solution[bin1_idx].append(item)
                            else:
                                # Senão, tentar bin2
                                bin2_total = sum(size for _, size in new_solution[bin2_idx])
                                if bin2_total + item[1] <= 1.0:
                                    new_solution[bin2_idx].append(item)
                                else:
                                    # Se não couber em nenhum, criar um novo bin
                                    new_solution.append([item])
                
                # Remover bins vazios
                new_solution = [bin for bin in new_solution if bin]
                
                return new_solution

            def accept_solution(current_value, new_value, temperature):
                """
                Decide se aceita uma nova solução baseado na diferença de valor e temperatura.
                Implementa o critério de Metropolis do Simulated Annealing.
                """
                if new_value <= current_value:  # Solução melhor (menos bins)
                    return True
                
                # Probabilidade de aceitar solução pior
                delta = new_value - current_value
                probability = np.exp(-delta / temperature)
                return random.random() < probability

            def binpack_hybrid_metaheuristic(items: List[float], time_limit: int, max_bins: int = None, 
                                            improvement: str = "best", use_sa: bool = True) -> Tuple[Solution, int, List[int], dict]:
                """
                Meta-heurística híbrida para Bin Packing combinando Busca Local com 
                elementos de ILS (Iterated Local Search) e SA (Simulated Annealing).
                
                Args:
                    items: Lista de tamanhos dos itens
                    time_limit: Tempo limite em segundos
                    max_bins: Número máximo de bins permitidos
                    improvement: Estratégia de melhoria ('best' ou 'first')
                    use_sa: Se deve usar Simulated Annealing para aceitar soluções piores
                    
                Returns:
                    Tuple contendo a melhor solução, número de iterações, histórico e estatísticas
                """
                start_time = time.time()
                
                # Parâmetros de SA
                initial_temp = 10.0
                cooling_rate = 0.95
                temp_min = 0.01
                
                # Parâmetros de ILS
                perturbation_strength_min = 0.1
                perturbation_strength_max = 0.5
                
                current_solution = initial_solution(items, max_bins)
                best_solution = current_solution
                current_value, current_imbalance, current_util = evaluate_solution(current_solution)
                best_value = current_value
                
                iterations = 0
                non_improving_iterations = 0
                temperature = initial_temp
                history = [current_value]
                
                # Estatísticas
                stats = {
                    "iterations": 0,
                    "restarts": 0,
                    "perturbations": 0,
                    "improvements": 0,
                    "time_to_best": 0,
                    "acceptance_rate": 0,
                    "solutions_evaluated": 0,
                    "sa_acceptances": 0
                }
                
                solutions_tried = 0
                sa_acceptances = 0
                
                # Critério de parada: tempo limite
                while time.time() - start_time < time_limit:
                    iterations += 1
                    stats["iterations"] = iterations
                    improved = False
                    
                    # Busca Local
                    neighbors = get_neighbors(current_solution, max_bins)
                    stats["solutions_evaluated"] += len(neighbors)
                    solutions_tried += len(neighbors)
                    
                    if not neighbors:
                        # Se não houver vizinhos, perturbar a solução atual
                        perturbation_strength = perturbation_strength_min + (perturbation_strength_max - perturbation_strength_min) * (non_improving_iterations / 50)
                        perturbation_strength = min(perturbation_strength_max, perturbation_strength)
                        
                        current_solution = perturb_solution(current_solution, perturbation_strength, temperature)
                        current_value, current_imbalance, current_util = evaluate_solution(current_solution)
                        stats["perturbations"] += 1
                        continue
                    
                    if improvement == "best":
                        # Best improvement: avaliar todos os vizinhos e selecionar o melhor
                        best_neighbor = None
                        best_neighbor_value = float('inf')
                        
                        for neighbor, move_desc in neighbors:
                            neighbor_value, _, _ = evaluate_solution(neighbor)
                            
                            # Se o número de bins for menor, ou igual com melhor balanceamento
                            if (neighbor_value < best_neighbor_value or 
                                (neighbor_value == best_neighbor_value and 
                                 evaluate_solution(neighbor)[1] < evaluate_solution(best_neighbor)[1] if best_neighbor else float('inf'))):
                                best_neighbor = neighbor
                                best_neighbor_value = neighbor_value
                        
                        if best_neighbor_value < current_value:
                            current_solution = best_neighbor
                            current_value = best_neighbor_value
                            improved = True
                            stats["improvements"] += 1
                            non_improving_iterations = 0
                            
                            if current_value < best_value:
                                best_solution = current_solution
                                best_value = current_value
                                stats["time_to_best"] = time.time() - start_time
                        elif use_sa and accept_solution(current_value, best_neighbor_value, temperature):
                            # Aceitar solução pior baseado em critério SA
                            current_solution = best_neighbor
                            current_value = best_neighbor_value
                            stats["sa_acceptances"] += 1
                            sa_acceptances += 1
                            non_improving_iterations += 1
                        else:
                            non_improving_iterations += 1
                    else:  # First improvement
                        # First improvement: selecionar o primeiro vizinho que melhora a solução
                        for neighbor, move_desc in neighbors:
                            neighbor_value, _, _ = evaluate_solution(neighbor)
                            
                            if neighbor_value < current_value:
                                current_solution = neighbor
                                current_value = neighbor_value
                                improved = True
                                stats["improvements"] += 1
                                non_improving_iterations = 0
                                
                                if current_value < best_value:
                                    best_solution = current_solution
                                    best_value = current_value
                                    stats["time_to_best"] = time.time() - start_time
                                
                                break
                            elif use_sa and accept_solution(current_value, neighbor_value, temperature):
                                # Aceitar solução pior baseado em critério SA
                                current_solution = neighbor
                                current_value = neighbor_value
                                stats["sa_acceptances"] += 1
                                sa_acceptances += 1
                                non_improving_iterations += 1
                                break
                        
                        if not improved:
                            non_improving_iterations += 1
                    
                    history.append(current_value)
                    
                    # Resfriamento (SA) - reduzir temperatura
                    temperature = max(temp_min, temperature * cooling_rate)
                    
                    # Estratégia de reinício: quando muitas iterações sem melhoria
                    if non_improving_iterations > 50:
                        # Reiniciar de uma solução inicial diferente
                        current_solution = initial_solution(items, max_bins)
                        current_value, current_imbalance, current_util = evaluate_solution(current_solution)
                        temperature = initial_temp  # Reset temperatura
                        non_improving_iterations = 0
                        stats["restarts"] += 1
                
                # Calcular taxa de aceitação do SA
                if solutions_tried > 0:
                    stats["acceptance_rate"] = (sa_acceptances / solutions_tried) * 100
                
                return best_solution, iterations, history, stats

            def generate_color_palette(num_items):
                """Gera uma paleta de cores única para cada item"""
                import matplotlib.pyplot as plt
                import matplotlib.colors as mcolors
                
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

            def create_bins_visualization(solution, items, color_palette):
                """Cria visualização detalhada dos bins"""
                # Dados para a visualização
                bin_data = []
                items_data = []
                
                for bin_idx, bin_items in enumerate(solution):
                    bin_usage = sum(size for _, size in bin_items)
                    bin_free = 1.0 - bin_usage
                    
                    # Adicionar bin ao dataset
                    bin_data.append({
                        "Bin": f"Bin {bin_idx+1}",
                        "Utilização": bin_usage * 100,
                        "Espaço Livre": bin_free * 100,
                        "Itens": len(bin_items)
                    })
                    
                    # Posição atual no bin (para empilhamento)
                    position = 0
                    
                    # Adicionar cada item individualmente
                    for (item_idx, size) in bin_items:
                        items_data.append({
                            "Bin": f"Bin {bin_idx+1}",
                            "Item": f"Item {item_idx+1}",
                            "Tamanho": size,
                            "Tamanho (%)": size * 100,
                            "Posição": position,
                            "Posição Final": position + size,
                            "Color": color_palette[item_idx % len(color_palette)]
                        })
                        position += size
                
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
                            items = load_instance(uploaded_file)
                            
                            if not items:
                                st.error("Arquivo vazio ou formato inválido. Verifique se o arquivo contém números entre 0 e 1.")
                            else:
                                st.write(f"Carregados {len(items)} itens.")
                                
                                # Gerar paleta de cores para os itens
                                color_palette = generate_color_palette(len(items))
                                
                                improvement_strategy = "best" if strategy == "Best Improvement" else "first"
                                solution, iterations, history, stats = binpack_hybrid_metaheuristic(
                                    items, time_limit, max_bins, improvement_strategy, use_sa
                                )
                                
                                is_valid = verify_solution(solution, items)
                                
                                if is_valid:
                                    st.success(f"Solução válida encontrada com {len(solution)} bins em {iterations} iterações!")
                                    
                                    # Métricas e estatísticas
                                    col1, col2, col3, col4 = st.columns(4)
                                    col1.metric("Bins utilizados", f"{len(solution)}")
                                    
                                    # Calcular utilização média
                                    bin_usages = [sum(size for _, size in bin_items) for bin_items in solution]
                                    avg_utilization = sum(bin_usages) / len(solution) if solution else 0
                                    col2.metric("Utilização média", f"{avg_utilization:.2%}")
                                    
                                    # Variância da utilização
                                    utilization_var = np.var(bin_usages) if len(solution) > 1 else 0
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
                                        bin_df, items_df = create_bins_visualization(solution, items, color_palette)
                                        
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
                                        # Visualização simplificada (original)
                                        bin_data = []
                                        for i, bin_items in enumerate(solution):
                                            bin_usage = sum(size for _, size in bin_items)
                                            bin_data.append({
                                                "Bin": f"Bin {i+1}",
                                                "Utilização": bin_usage * 100,
                                                "Itens": len(bin_items)
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
                                        for i, bin_items in enumerate(solution):
                                            bin_usage = sum(size for _, size in bin_items)
                                            st.write(f"Bin {i+1}: {len(bin_items)} itens, utilização: {bin_usage*100:.2f}%")
                                            
                                            # Criar lista de itens formatada com cores
                                            items_html = ""
                                            for idx, size in bin_items:
                                                color_hex = color_palette[idx % len(color_palette)]
                                                items_html += f'<span style="background-color:{color_hex}; padding:2px 8px; margin:2px; border-radius:3px; display:inline-block; color:white;">Item {idx+1} ({size:.2f})</span>'
                                            
                                            st.write(f"  Itens: {items_html}", unsafe_allow_html=True)
                                else:
                                    # Verificar se o problema pode estar no número máximo de bins
                                    total_item_size = sum(items)
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
            st.header("Solução com PLI")
            st.markdown("""
            ## Solução para o Professor Rian
            
            Esta seção contém a implementação da meta-heurística para o problema de Bin Packing
            para a disciplina de Otimização Combinatória Computacional.
            
            *Conteúdo em desenvolvimento.*
            """)
    
    with tabs_main[3]:  # Aba Sobre
        about.app()
    
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