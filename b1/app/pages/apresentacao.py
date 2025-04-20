import streamlit as st

def app():
    st.title("Apresentação")
    
    # Informações do projeto
    st.markdown("""
    ## B1
    
    Primeira avaliação da disciplina de Otimização Combinatória Computacional (OCC) 2024.2 propostas pelos professores Bruno e Rian.
    """)

    # Atividades propostas
    st.markdown("""
    ## Atividades Propostas
    """)
    
    # Atividades em expanders
    with st.expander("Atividade 1: Problema de Bin Packing - Professor Bruno"):
        st.markdown("""
    
        O problema de Bin Packing é um problema clássico de otimização combinatória que consiste em empacotar 
        um conjunto de itens em recipientes (bins) de capacidade fixa, de forma a minimizar o número de recipientes utilizados.
        
        Nesta atividade, você deve:
        
        1. Implementar o algoritmo First Fit Decreasing (FFD)
        2. Implementar uma meta-heurística de busca local

        """)
    
    with st.expander("Atividade 2: Otimização de Portfólio de Mercado - Professor Rian"):
        st.markdown("""
                
        Implementar os seguintes problemas de programação linear inteira:
            
            1. Problema da ração
            2. Problema da dieta
            3. Problema do Plantio
            4. Problema das tintas
            5. Problema do transporte
            6. Problema do floxo máximo
            7. Escalonamento de horário
            8. Problema de cobertura
            9. Problema da mochila
            10. Problema de padrões
            11. Problema das facilidades
            12. Problema de Frequência
            13. Problema da clique máxima

        """)
    
       
    # Footer
    st.markdown("---")
    st.markdown("© 2025 | Otimização Combinatória Computacional | PPGI-UFAL | v1.0")