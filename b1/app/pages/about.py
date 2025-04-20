import streamlit as st
import os
import base64

def app():
    st.title("Sobre o Aplicativo")
    
    # Informações do projeto
    st.markdown("""
    ## Projeto: Meta-heurística para o Problema de Bin Packing
    
    Este aplicativo foi desenvolvido como atividade avaliativa B1 para a disciplina de Otimização Combinatória 
    Computacional do Programa de Pós-Graduação em Informática (PPGI) da Universidade Federal de Alagoas (UFAL).
    
    ### Objetivo
    
    O objetivo principal deste projeto é implementar e avaliar diferentes abordagens para resolver instâncias do 
    problema de Bin Packing (BP), incluindo heurísticas construtivas, meta-heurísticas e métodos exatos.
    
    ### Algoritmos implementados
    
    - **First Fit Decreasing (FFD)**: Algoritmo construtivo que gera a solução inicial
    - **Busca Local**: Meta-heurística com movimentos de troca e realocação
        - **Movimento de Troca**: Trocar itens entre dois bins diferentes
        - **Movimento de Realocação**: Mover um item de um bin para outro
    - **Programação Linear Inteira**: Modelo matemático para encontrar a solução ótima
    """)
    
    # Informações técnicas
    with st.expander("Informações técnicas"):
        st.markdown("""
        ### Tecnologias utilizadas
        
        - **Python**: Linguagem de programação principal
        - **Streamlit**: Framework para criar aplicativos web interativos
        - **Pandas**: Manipulação e análise de dados
        - **Matplotlib**: Visualizações e gráficos estáticos
        - **Plotly**: Visualizações e gráficos interativos
        - **IBM CPLEX**: Solver para Programação Linear Inteira
        
        ### Função de Avaliação da Meta-heurística
        
        A função objetivo principal é minimizar o número de bins utilizados. Adicionalmente, 
        para guiar a busca local, utilizamos uma função de avaliação que considera:
        
        - Número total de bins utilizados (componente principal)
        - Balanceamento de carga dos bins (componente secundário)
        
        A função de avaliação pode ser expressa como:
        ```
        f(s) = k + α * Σ(1 - ocupação(Binᵢ))²
        ```
        
        Onde:
        - k é o número de bins utilizados
        - α é um parâmetro de penalização 
        - ocupação(Binᵢ) é a soma dos tamanhos dos itens no Binᵢ
        """)
    
    # Informações institucionais
    st.markdown("## Informações Institucionais")
    
    col1 = st.columns(1)[0] 
    
    with col1:
        # Tentar carregar logos se existirem
        logos_path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), "data", "logos")
        
        # Função para carregar e exibir logo
        def display_logo(logo_name):
            logo_path = os.path.join(logos_path, f"{logo_name}.png")
            if os.path.exists(logo_path):
                with open(logo_path, "rb") as f:
                    img_data = base64.b64encode(f.read()).decode("utf-8")
                st.markdown(f'<img src="data:image/png;base64,{img_data}" width="150">', unsafe_allow_html=True)
                return True
            return False
        
        # Exibir os logos disponíveis em linha
        st.markdown('<div style="display: flex; justify-content: space-evenly;">', unsafe_allow_html=True)
        display_logo("ufal") or st.markdown("### UFAL")
        display_logo("ic") or st.markdown("### Instituto de Computação")
        display_logo("ppgi") or st.markdown("### PPGI-UFAL")
        st.markdown('</div>', unsafe_allow_html=True)

        st.markdown("""
        ### Universidade Federal de Alagoas (UFAL)
        Programa de Pós-Graduação em Informática (PPGI)
        Instituto de Computação (IC)
                    
        ### Atividade Avaliativa B1
        
        **Disciplina**: Otimização Combinatória Computacional  
        **Professor**: 
        - Dr. Rian Gabriel Santos Pinheiro
        
        ### Aluno
        Fábio Linhares
        """)
    
    # Atualizações do projeto
    st.markdown("""
    ## Atualizações do Projeto
    
    ### Versão 1.0 (Abril/2025)
    - Implementação inicial da meta-heurística de Busca Local
    - Interface gráfica com Streamlit
    - Visualização interativa das soluções
    
    ### Versão 1.1 (Abril/2025)
    - Adição de comparação com Programação Linear Inteira
    - Melhorias na interface do usuário
    - Implementação de novas atividades práticas
    - Reorganização das abas e conteúdo
    """)
    
    # Referências
    st.markdown("""
    ## Referências
    
    1. Martello, S., & Toth, P. (1990). *Knapsack Problems: Algorithms and Computer Implementations*. John Wiley & Sons.
    
    2. Coffman, E. G., Garey, M. R., & Johnson, D. S. (1996). *Approximation algorithms for bin packing: A survey*. In D. S. Hochbaum (Ed.), Approximation Algorithms for NP-Hard Problems (pp. 46-93). PWS Publishing.
    
    3. Falkenauer, E. (1996). *A hybrid grouping genetic algorithm for bin packing*. Journal of heuristics, 2(1), 5-30.
    
    4. Lodi, A., Martello, S., & Vigo, D. (2002). *Recent advances on two-dimensional bin packing problems*. Discrete Applied Mathematics, 123(1-3), 379-396.
    
    5. Scholl, A., Klein, R., & Jürgens, C. (1997). *BISON: A fast hybrid procedure for exactly solving the one-dimensional bin packing problem*. Computers & Operations Research, 24(7), 627-645.
    
    6. IBM ILOG CPLEX (2023). *IBM ILOG CPLEX Optimization Studio CPLEX User's Manual Version 22.1.1*.

    ## Repositório
    
    [https://github.com/fabio-linhares/occ-2024-2/tree/main/b1](https://github.com/fabio-linhares/occ-2024-2/tree/main/b1)
    """)
    
    # Footer
    st.markdown("---")
    st.markdown("© 2025 Fábio Linhares | PPGI-UFAL | Meta-heurística para o Problema de Bin Packing | v1.1")




