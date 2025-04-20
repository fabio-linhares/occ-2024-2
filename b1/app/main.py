import streamlit as st
import os
from PIL import Image
import sys

# Adicionar o diretório atual ao caminho do Python para permitir importações relativas
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

# Importando as páginas da aplicação
from pages import about, main_app, apresentacao

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
            # Conteúdo trocado: agora a sub-aba "Apresentação" contém o conteúdo detalhado da aplicação
            main_app.app()  # Aplicação principal do professor Bruno
            
        with subtabs_heuristica[1]:  # Sub-aba Solução
            # Conteúdo trocado: agora a sub-aba "Solução" contém apenas o texto introdutório
            st.header("Solução com Heurísticas")
            st.markdown("""
            Nesta seção são apresentados os conceitos e fundamentos das heurísticas
            implementadas para resolver o problema de Bin Packing.
            
            As heurísticas implementadas são:
            
            - **First Fit Decreasing (FFD)**: Algoritmo construtivo que ordena os itens em ordem decrescente de tamanho
              e os aloca sequencialmente no primeiro bin onde couberem.
              
            - **Busca Local**: Meta-heurística que, a partir de uma solução inicial, realiza movimentos no espaço de
              soluções (troca e realocação de itens) para encontrar soluções melhores.
            
            Estas técnicas são adequadas para problemas NP-difíceis como o Bin Packing, onde métodos exatos
            podem ser computacionalmente inviáveis para instâncias grandes.
            """)
    
    with tabs_main[2]:  # Aba PLI
        st.title("Programação Linear Inteira")
        # Sub-abas dentro da aba PLI
        subtabs_pli = st.tabs(["Apresentação", "Solução"])
        
        with subtabs_pli[0]:  # Sub-aba Apresentação
            st.header("Apresentação da PLI")
            st.markdown("""
            ## Programação Linear Inteira para o Problema de Bin Packing
            
            A Programação Linear Inteira (PLI) é uma abordagem matemática exata para resolução de problemas de otimização
            como o Bin Packing. Diferentemente das heurísticas, a PLI garante a obtenção da solução ótima (quando convergente),
            porém com maior custo computacional.
            """)
            
            # Cards para PLI - semelhante ao layout da página de heurísticas
            col1, col2, col3, col4 = st.columns(4)
            
            # Card 1 - Modelo Matemático
            with col1:
                st.markdown("""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(109, 40, 217, 0.2); text-align: center;">
                    <div style="font-size: 24px;">📐</div>
                    <div style="font-weight: bold; margin: 10px 0;">MODELO MATEMÁTICO</div>
                    <div style="font-size: 18px; font-weight: bold;">Exato</div>
                    <div style="font-size: 12px; opacity: 0.7;">Formulação rigorosa do problema</div>
                </div>
                """, unsafe_allow_html=True)
            
            # Card 2 - Solvers
            with col2:
                st.markdown("""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(16, 185, 129, 0.2); text-align: center;">
                    <div style="font-size: 24px;">🧮</div>
                    <div style="font-weight: bold; margin: 10px 0;">SOLVERS</div>
                    <div style="font-size: 18px; font-weight: bold;">CPLEX/Gurobi</div>
                    <div style="font-size: 12px; opacity: 0.7;">Ferramentas avançadas de otimização</div>
                </div>
                """, unsafe_allow_html=True)
            
            # Card 3 - Garantias
            with col3:
                st.markdown("""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(59, 130, 246, 0.2); text-align: center;">
                    <div style="font-size: 24px;">✓</div>
                    <div style="font-weight: bold; margin: 10px 0;">GARANTIAS</div>
                    <div style="font-size: 18px; font-weight: bold;">Otimalidade</div>
                    <div style="font-size: 12px; opacity: 0.7;">Solução comprovadamente ótima</div>
                </div>
                """, unsafe_allow_html=True)
            
            # Card 4 - Limitações
            with col4:
                st.markdown("""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(245, 158, 11, 0.2); text-align: center;">
                    <div style="font-size: 24px;">⚠️</div>
                    <div style="font-weight: bold; margin: 10px 0;">LIMITAÇÕES</div>
                    <div style="font-size: 18px; font-weight: bold;">Escalabilidade</div>
                    <div style="font-size: 12px; opacity: 0.7;">Desafios com instâncias grandes</div>
                </div>
                """, unsafe_allow_html=True)
            
            # Descrição da abordagem de PLI
            st.markdown("""
            ### Formulação Matemática
            
            O problema de Bin Packing pode ser formulado como um problema de PLI da seguinte forma:
            """)
            
            # Layout aprimorado para índices e conjuntos
            col1, col2 = st.columns(2)
            
            with col1:
                st.markdown(r"""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(109, 40, 217, 0.2);">
                    <h4 style="font-weight: bold; margin-top: 0;">Índices e Conjuntos:</h4>
                    <ul style="margin-bottom: 0;">
                        <li>$i \in I$: conjunto de itens</li>
                        <li>$j \in J$: conjunto de bins (no pior caso, um bin para cada item)</li>
                    </ul>
                </div>
                """, unsafe_allow_html=True)
            
            with col2:
                st.markdown("""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(59, 130, 246, 0.2);">
                    <h4 style="font-weight: bold; margin-top: 0;">Parâmetros:</h4>
                    <ul style="margin-bottom: 0;">
                        <li>$w_i$: tamanho (peso) do item $i$</li>
                        <li>$C$: capacidade de cada bin</li>
                    </ul>
                </div>
                """, unsafe_allow_html=True)
            
            # Variáveis de decisão em destaque
            st.markdown("""
            <div style="border-radius: 10px; padding: 15px; background-color: rgba(16, 185, 129, 0.2); margin-top: 15px;">
                <h4 style="font-weight: bold; margin-top: 0;">Variáveis de Decisão:</h4>
                <ul style="margin-bottom: 0;">
                    <li>$y_j$: 1 se o bin $j$ é utilizado, 0 caso contrário</li>
                    <li>$x_{ij}$: 1 se o item $i$ é alocado ao bin $j$, 0 caso contrário</li>
                </ul>
            </div>
            """, unsafe_allow_html=True)
            
            # Função objetivo com destaque
            st.markdown(r"""
            <div style="border-radius: 10px; padding: 15px; background-color: rgba(245, 158, 11, 0.2); margin-top: 15px;">
                <h4 style="font-weight: bold; margin-top: 0;">Função Objetivo:</h4>
                <div style="text-align: center; font-size: 1.2em; padding: 10px;">
                    $\min \displaystyle\sum_{j \in J} y_j$
                </div>
                <p style="margin-bottom: 0; font-style: italic; text-align: center;">Minimizar o número de bins utilizados</p>
            </div>
            """, unsafe_allow_html=True)
            
            # Restrições com formatação melhorada
            st.markdown(r"""
            <div style="border-radius: 10px; padding: 15px; background-color: rgba(239, 68, 68, 0.2); margin-top: 15px;">
                <h4 style="font-weight: bold; margin-top: 0;">Restrições:</h4>
                <ol>
                    <li>
                        <strong>Cada item deve ser atribuído a exatamente um bin:</strong>
                        <div style="text-align: center; padding: 10px;">
                            $\displaystyle\sum_{j \in J} x_{ij} = 1, \quad \forall i \in I$
                        </div>
                    </li>
                    <li>
                        <strong>A capacidade de cada bin não pode ser excedida:</strong>
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
            
            # Comparativo entre Heurísticas e PLI
            st.markdown("""
            ### Comparação: Heurísticas vs. PLI
            """)
            
            col1, col2 = st.columns(2)
            
            with col1:
                st.markdown("""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(16, 185, 129, 0.2);">
                    <h4 style="text-align: center; margin-bottom: 10px;">Vantagens da PLI</h4>
                    <ul>
                        <li><strong>Solução Ótima:</strong> Garante a obtenção da solução ótima quando converge</li>
                        <li><strong>Limites (Bounds):</strong> Fornece limites inferiores para o problema</li>
                        <li><strong>Flexibilidade:</strong> Facilidade para adicionar novas restrições</li>
                        <li><strong>Certificados:</strong> Fornece provas de otimalidade ou inviabilidade</li>
                        <li><strong>Robustez:</strong> Comportamento previsível com diferentes instâncias</li>
                    </ul>
                </div>
                """, unsafe_allow_html=True)
            
            with col2:
                st.markdown("""
                <div style="border-radius: 10px; padding: 15px; background-color: rgba(245, 158, 11, 0.2);">
                    <h4 style="text-align: center; margin-bottom: 10px;">Desafios da PLI</h4>
                    <ul>
                        <li><strong>Complexidade Computacional:</strong> Tempo exponencial no pior caso</li>
                        <li><strong>Recursos:</strong> Maior consumo de memória e processamento</li>
                        <li><strong>Escalabilidade:</strong> Dificuldades com instâncias grandes</li>
                        <li><strong>Aplicabilidade:</strong> Pode ser impraticável em cenários de tempo real</li>
                        <li><strong>Dependência:</strong> Requer bibliotecas especializadas como CPLEX ou Gurobi</li>
                    </ul>
                </div>
                """, unsafe_allow_html=True)
            
            # Implementação em Python com CPLEX
            st.markdown("""
            ### Implementação com CPLEX
            
            Abaixo está um exemplo de implementação do problema de Bin Packing usando a biblioteca CPLEX em Python:
            """)
            
            st.code("""
from docplex.mp.model import Model

def bin_packing_cplex(item_weights, bin_capacity):
    n = len(item_weights)      # Número de itens
    m = n                      # Número máximo de bins (pior caso)
    
    # Criar modelo
    model = Model("BinPacking")
    
    # Variáveis de decisão
    x = {}  # x[i,j] = 1 se o item i for atribuído ao bin j
    for i in range(n):
        for j in range(m):
            x[i,j] = model.binary_var(name=f"x_{i}_{j}")
    
    y = {}  # y[j] = 1 se o bin j for utilizado
    for j in range(m):
        y[j] = model.binary_var(name=f"y_{j}")
    
    # Função objetivo: minimizar o número de bins utilizados
    model.minimize(model.sum(y[j] for j in range(m)))
    
    # Restrição: cada item deve ser atribuído a exatamente um bin
    for i in range(n):
        model.add_constraint(
            model.sum(x[i,j] for j in range(m)) == 1,
            ctname=f"item_{i}_assignment"
        )
    
    # Restrição: a capacidade de cada bin não pode ser excedida
    for j in range(m):
        model.add_constraint(
            model.sum(item_weights[i] * x[i,j] for i in range(n)) <= bin_capacity * y[j],
            ctname=f"bin_{j}_capacity"
        )
    
    # Resolver o modelo
    solution = model.solve(log_output=True)
    
    if solution:
        # Extrair solução
        bins_used = []
        for j in range(m):
            if solution.get_value(y[j]) > 0.5:  # y[j] = 1
                bin_items = []
                for i in range(n):
                    if solution.get_value(x[i,j]) > 0.5:  # x[i,j] = 1
                        bin_items.append((i, item_weights[i]))
                bins_used.append(bin_items)
        
        return {
            "status": "optimal",
            "num_bins": len(bins_used),
            "bins": bins_used,
            "objective_value": solution.get_objective_value()
        }
    else:
        return {"status": "infeasible"}
            """, language="python")
            
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
    
    # Informações no rodapé da sidebar
    st.sidebar.markdown("---")
    st.sidebar.info("""
    **Desenvolvido por:** Fábio Linhares  
    **Universidade:** UFAL  
    **Programa:** PPGI
    
    [Repositório GitHub](https://github.com/fabio-linhares/occ-2024-2/tree/main/b1)
    
    **v1.0** - Abril/2025
    """)

# Executar a aplicação
if __name__ == "__main__":
    main()