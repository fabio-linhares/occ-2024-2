import pandas as pd
import pulp
import time
import streamlit as st
import numpy as np
import matplotlib.pyplot as plt
import subprocess
import re
import os

###############################
def load_csv_file(file):
    """
    Carrega um arquivo CSV genérico.
    
    Args:
        file: Arquivo carregado via st.file_uploader
        
    Returns:
        DataFrame pandas ou None em caso de erro
    """
    try:
        # Tentar carregar com interpretação automática de tipos
        df = pd.read_csv(file, sep=None, engine='python')
        
        # Se a primeira coluna parecer ser um índice (sem nome ou Unnamed)
        if df.columns[0] == '' or str(df.columns[0]).startswith('Unnamed:'):
            df = df.set_index(df.columns[0])
            
        # Tentar converter colunas para tipos numéricos onde possível
        for col in df.columns:
            try:
                df[col] = pd.to_numeric(df[col])
            except:
                pass
                
        return df
    except Exception as e:
        st.error(f"Erro ao carregar o arquivo: {str(e)}")
        return None
###############################    
def resolver_tsp(df_distancias):
    """
    Resolve o Problema do Caixeiro Viajante usando Programação Linear Inteira (formulação MTZ).
    Args:
        df_distancias: DataFrame quadrado com nomes/destinos, custos na matriz.
    Returns:
        Dicionário com rota ótima e custo total.
    """
    cidades = list(df_distancias.index)
    n = len(cidades)
    dist = {(i,j): df_distancias.loc[i, j] for i in cidades for j in cidades if i != j}

    # Variáveis binárias: x[i][j] == 1 se vai de i para j
    x = pulp.LpVariable.dicts('x', dist, cat='Binary')

    # Variáveis auxiliares para subtour elimination (MTZ)
    u = pulp.LpVariable.dicts('u', cidades, lowBound=0, upBound=n-1, cat='Integer')

    model = pulp.LpProblem("TSP", pulp.LpMinimize)
    model += pulp.lpSum([x[i,j] * dist[(i,j)] for (i,j) in dist])  # Minimizar custo total

    # A cada cidade chega exatamente 1 aresta
    for j in cidades:
        model += pulp.lpSum([x[i,j] for i in cidades if i != j]) == 1
    # A cada cidade sai exatamente 1 aresta
    for i in cidades:
        model += pulp.lpSum([x[i,j] for j in cidades if i != j]) == 1
    # Eliminação de subtours (MTZ)
    for i in cidades:
        for j in cidades:
            if i != j and (i != cidades[0]) and (j != cidades[0]):
                model += (u[i] - u[j] + (n-1)*x[i,j] <= n-2)

    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time

    # Reconstruir rota: percorra x e busque arestas selecionadas
    rota = [cidades[0]]
    visitados = set(rota)
    atual = cidades[0]
    while len(rota) < n:
        for j in cidades:
            if atual != j and pulp.value(x[(atual, j)]) > 0.5:
                rota.append(j)
                visitados.add(j)
                atual = j
                break
    rota.append(cidades[0])  # Volta à origem

    return {
        "status": pulp.LpStatus[model.status],
        "rota": rota,
        "custo_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
###############################
def resolver_problema_mochila(df, capacidade):
    """
    Resolve o problema da mochila usando Programação Linear Inteira.
    
    Args:
        df: DataFrame com os itens, pesos e valores
        capacidade: Capacidade máxima da mochila
        
    Returns:
        Dicionário com os resultados da solução
    """
    # Verificar se as colunas necessárias existem
    if 'item' not in df.columns or 'peso' not in df.columns or 'valor' not in df.columns:
        return {"status": "erro", "mensagem": "O arquivo deve conter as colunas: item, peso, valor"}
    
    # Criar o modelo de PLI
    model = pulp.LpProblem("Problema_da_Mochila", pulp.LpMaximize)
    
    # Criar variáveis binárias para cada item
    itens = list(df['item'])
    x = pulp.LpVariable.dicts("x", itens, lowBound=0, upBound=1, cat=pulp.LpInteger)
    
    # Função objetivo: maximizar o valor total
    model += pulp.lpSum([x[i] * df.loc[df['item'] == i, 'valor'].values[0] for i in itens])
    
    # Restrição: peso total não deve exceder a capacidade
    model += pulp.lpSum([x[i] * df.loc[df['item'] == i, 'peso'].values[0] for i in itens]) <= capacidade
    
    # Resolver o modelo
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    
    # Coletar resultados
    resultados = {
        "status": pulp.LpStatus[model.status],
        "valor_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
    
    # Itens selecionados
    itens_selecionados = []
    peso_total = 0
    
    for i in itens:
        if pulp.value(x[i]) > 0.5:  # Considerando arredondamento
            item_peso = df.loc[df['item'] == i, 'peso'].values[0]
            item_valor = df.loc[df['item'] == i, 'valor'].values[0]
            itens_selecionados.append({
                "Item": i,
                "Peso": item_peso,
                "Valor": item_valor
            })
            peso_total += item_peso
    
    resultados["itens_selecionados"] = itens_selecionados
    resultados["peso_total"] = peso_total
    resultados["uso_capacidade"] = peso_total/capacidade*100
    
    return resultados
###############################
def resolver_problema_dieta(df_alimentos, df_nutrientes):
    """
    Resolve o problema da dieta usando Programação Linear Inteira.
    
    Args:
        df_alimentos: DataFrame com os alimentos, custos e nutrientes
        df_nutrientes: DataFrame com os requisitos mínimos e máximos de nutrientes
        
    Returns:
        Dicionário com os resultados da solução
    """
    # Verificar se as colunas necessárias existem
    if 'alimento' not in df_alimentos.columns or 'custo' not in df_alimentos.columns:
        return {"status": "erro", "mensagem": "O arquivo de alimentos deve conter as colunas: alimento, custo"}
    
    if 'nutriente' not in df_nutrientes.columns or 'minimo' not in df_nutrientes.columns:
        return {"status": "erro", "mensagem": "O arquivo de requisitos deve conter as colunas: nutriente, minimo, maximo"}
    
    # Criar o modelo PLI - minimização de custo
    model = pulp.LpProblem("Problema_da_Dieta", pulp.LpMinimize)
    
    # Obter lista de alimentos e nutrientes
    alimentos = list(df_alimentos['alimento'])
    nutrientes = list(df_nutrientes['nutriente'])
    
    # Criar variáveis de decisão (quantidade de cada alimento a ser consumido)
    x = pulp.LpVariable.dicts("x", alimentos, lowBound=0, cat=pulp.LpContinuous)
    
    # Função objetivo: minimizar o custo total
    model += pulp.lpSum([x[a] * df_alimentos.loc[df_alimentos['alimento'] == a, 'custo'].values[0] for a in alimentos])
    
    # Restrições nutricionais
    for n in nutrientes:
        # Obter mínimo e máximo permitidos para este nutriente
        minimo = df_nutrientes.loc[df_nutrientes['nutriente'] == n, 'minimo'].values[0]
        maximo = df_nutrientes.loc[df_nutrientes['nutriente'] == n, 'maximo'].values[0] if 'maximo' in df_nutrientes.columns else float('inf')
        
        # Somatório da quantidade do nutriente em cada alimento * quantidade do alimento
        nutriente_total = pulp.lpSum([x[a] * df_alimentos.loc[df_alimentos['alimento'] == a, n].values[0] if n in df_alimentos.columns else 0 for a in alimentos])
        
        # Adicionar restrições de mínimo e máximo
        model += nutriente_total >= minimo, f"Min_{n}"
        if not pd.isna(maximo) and maximo < float('inf'):
            model += nutriente_total <= maximo, f"Max_{n}"
    
    # Resolver o modelo
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    
    # Coletar resultados
    resultados = {
        "status": pulp.LpStatus[model.status],
        "custo_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
    
    # Alimentos selecionados e suas quantidades
    alimentos_selecionados = []
    
    for a in alimentos:
        if pulp.value(x[a]) > 1e-5:  # Valor maior que quase zero
            alimentos_selecionados.append({
                "Alimento": a,
                "Quantidade": pulp.value(x[a])
            })
    
    # Calcular nutrientes totais na dieta selecionada
    nutrientes_obtidos = {}
    for n in nutrientes:
        valor_obtido = sum([pulp.value(x[a]) * df_alimentos.loc[df_alimentos['alimento'] == a, n].values[0] if n in df_alimentos.columns else 0 for a in alimentos])
        minimo = df_nutrientes.loc[df_nutrientes['nutriente'] == n, 'minimo'].values[0]
        maximo = df_nutrientes.loc[df_nutrientes['nutriente'] == n, 'maximo'].values[0] if 'maximo' in df_nutrientes.columns else float('inf')
        
        nutrientes_obtidos[n] = {
            "obtido": valor_obtido,
            "minimo": minimo,
            "maximo": maximo if not pd.isna(maximo) else "Ilimitado"
        }
    
    resultados["alimentos_selecionados"] = alimentos_selecionados
    resultados["nutrientes_obtidos"] = nutrientes_obtidos
    
    return resultados
###############################
def resolver_set_cover(df):
    """
    Resolve o Problema de Cobertura Mínima usando PLI.
    df: DataFrame (linhas: elementos; colunas: conjuntos; valores 0/1).
    """
    # Converter os valores para numéricos, garantindo que a comparação funcione corretamente
    try:
        # Tentar converter todo o DataFrame para valores numéricos
        df_numeric = df.apply(pd.to_numeric, errors='coerce')
        
        # Usar o DataFrame convertido se não houver NaN, caso contrário, manter o original
        # e fazer conversões individuais durante a comparação
        if not df_numeric.isnull().any().any():
            df = df_numeric
    except:
        pass  # Se falhar, continuamos com o DataFrame original
    
    elementos = df.index.tolist()
    conjuntos = df.columns.tolist()

    x = pulp.LpVariable.dicts("x", conjuntos, cat='Binary')
    model = pulp.LpProblem("Set_Covering", pulp.LpMinimize)

    # Objetivo: minimizar número de conjuntos
    model += pulp.lpSum([x[j] for j in conjuntos])

    for i in elementos:
        # Converter o valor para float antes de comparar
        model += pulp.lpSum([x[j] for j in conjuntos if float(df.loc[i, j]) > 0]) >= 1, f'E_{i}'

    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time

    selecionados = [j for j in conjuntos if pulp.value(x[j]) > 0.5]
    return {
        "status": pulp.LpStatus[model.status],
        "selecionados": selecionados,
        "num_selecionados": len(selecionados),
        "tempo_solucao": solve_time
    }
###############################
def resolver_facility_location(df_facility, df_clientes):
    """
    Facility Location: Decide abertura de depósitos e atendimento de clientes.
    """
    F = list(df_facility['facility'])
    C = list(df_clientes['cliente'])
    custos_fixo = {row['facility']: row['fixo'] for i, row in df_facility.iterrows()}
    custos_atend = {(row['facility'], row['cliente']): row['custo'] for i,row in df_clientes.iterrows()}

    y = pulp.LpVariable.dicts('Open', F, cat='Binary')
    x = pulp.LpVariable.dicts('Assign', custos_atend, cat='Binary')
    model = pulp.LpProblem("Facility_Location", pulp.LpMinimize)

    # Cada cliente atendido por exatamente um depósito
    for c in C:
        model += pulp.lpSum([x[(f, c)] for f in F]) == 1

    # Só pode atender se depósito aberto
    for f in F:
        for c in C:
            model += x[(f, c)] <= y[f]

    # Objetivo: mínimo custo total
    model += pulp.lpSum([custos_fixo[f]*y[f] for f in F]) + pulp.lpSum([custos_atend[(f,c)]*x[(f,c)] for f in F for c in C])

    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time

    abertos = [f for f in F if pulp.value(y[f])>0.5]
    aloc = [(f,c) for (f,c) in custos_atend if pulp.value(x[(f,c)])>0.5]
    return {
        "status": pulp.LpStatus[model.status],
        "abertos": abertos,
        "alocacoes": aloc,
        "custo_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
###############################
def resolver_feed_problem(df_produtos, df_ingredientes, df_restricoes):
    """
    df_produtos: DataFrame com colunas 'produto', 'lucro'
    df_ingredientes: DataFrame com colunas 'produto', 'ingrediente', 'uso' (proporção de uso)
    df_restricoes: DataFrame com colunas 'ingrediente', 'min', 'max'
    """
    produtos = df_produtos['produto'].tolist()
    ingredientes = df_restricoes['ingrediente'].tolist()

    x = pulp.LpVariable.dicts("x", produtos, lowBound=0, cat=pulp.LpInteger)
    model = pulp.LpProblem("FeedProblem", pulp.LpMaximize)

    # Objetivo: maximizar lucro total
    lucros = {row['produto']: row['lucro'] for i,row in df_produtos.iterrows()}
    model += pulp.lpSum([x[p] * lucros[p] for p in produtos])

    # Restrições de ingredientes
    for i in ingredientes:
        uso_i = {row['produto']: row['uso'] for idx, row in df_ingredientes[df_ingredientes['ingrediente']==i].iterrows()}
        min_i = float(df_restricoes[df_restricoes['ingrediente']==i]['min'])
        max_i = float(df_restricoes[df_restricoes['ingrediente']==i]['max'])
        model += pulp.lpSum([uso_i.get(p,0)*x[p] for p in produtos]) >= min_i
        model += pulp.lpSum([uso_i.get(p,0)*x[p] for p in produtos]) <= max_i

    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time

    return {
        "status": pulp.LpStatus[model.status],
        "quantidade_produto": {p: pulp.value(x[p]) for p in produtos},
        "lucro_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
###############################
def carregar_problema_fluxo_custo_minimo(file_path):
    """
    Carrega um arquivo de problema de fluxo de custo mínimo no formato específico.
    
    Args:
        file_path: Caminho para o arquivo
        
    Returns:
        Tupla (DataFrame de arcos, nós de origem, demandas)
    """
    try:
        with open(file_path, 'r') as f:
            linhas = f.readlines()
        
        # A primeira linha contém o comentário, ignoramos
        # A segunda linha contém o número de nós e arcos
        n_nos, n_arcos = map(int, linhas[1].strip().split())
        
        # A terceira linha contém as ofertas/demandas
        ofertas_demandas = list(map(int, linhas[2].strip().split()))
        
        # A quarta linha é o cabeçalho dos arcos
        colunas = linhas[3].strip().split(',')
        
        # As linhas restantes contêm os arcos
        dados_arcos = []
        for i in range(4, len(linhas)):
            if linhas[i].strip():  # Ignora linhas vazias
                valores = linhas[i].strip().split(',')
                dados_arcos.append(dict(zip(colunas, valores)))
        
        # Converte para DataFrame
        df_arcos = pd.DataFrame(dados_arcos)
        
        # Converte colunas numéricas
        for col in ['custo', 'capacidade']:
            df_arcos[col] = pd.to_numeric(df_arcos[col])
        
        return df_arcos, ofertas_demandas
    except Exception as e:
        st.error(f"Erro ao carregar o arquivo de fluxo de custo mínimo: {str(e)}")
        return None, None

###############################
def resolver_min_cost_flow(df_arcos, origem=None, destino=None):
    """ 
    df_arcos: DataFrame com colunas: 'de', 'para', 'custo', 'capacidade'
    origem, destino: nomes dos nós origem e destino
    """
    nos = list(set(df_arcos['de']).union(df_arcos['para']))
    arcos = [(row['de'],row['para']) for idx, row in df_arcos.iterrows()]
    custo = {(row['de'],row['para']): row['custo'] for idx, row in df_arcos.iterrows()}
    capacidade = {(row['de'],row['para']): row['capacidade'] for idx, row in df_arcos.iterrows()}
    x = pulp.LpVariable.dicts("x", arcos, lowBound=0, upBound=None, cat=pulp.LpInteger)
    model = pulp.LpProblem("MinCostFlow", pulp.LpMinimize)
    model += pulp.lpSum([custo[a]*x[a] for a in arcos])
    for n in nos:
        if n==origem:
            model += pulp.lpSum([x[a] for a in arcos if a[0]==n]) - pulp.lpSum([x[a] for a in arcos if a[1]==n]) == 1
        elif n==destino:
            model += pulp.lpSum([x[a] for a in arcos if a[1]==n]) - pulp.lpSum([x[a] for a in arcos if a[0]==n]) == 1
        else:
            model += pulp.lpSum([x[a] for a in arcos if a[0]==n]) - pulp.lpSum([x[a] for a in arcos if a[1]==n]) == 0
    for a in arcos:
        model += x[a] <= capacidade[a]
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    
    # Converte as tuplas para strings para evitar problemas de serialização
    arcos_usados = {}
    for a in arcos:
        flow = pulp.value(x[a])
        if flow > 1e-5:
            arco_str = f"{a[0]}->{a[1]}"
            arcos_usados[arco_str] = flow
    
    return {
        "status": pulp.LpStatus[model.status],
        "arcos_usados": arcos_usados,
        "custo_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }

###############################
def resolver_min_cost_flow_from_file(file_path):
    """
    Resolve o problema de fluxo de custo mínimo a partir de um arquivo.
    
    Args:
        file_path: Caminho para o arquivo
        
    Returns:
        Dicionário com os resultados
    """
    df_arcos, ofertas_demandas = carregar_problema_fluxo_custo_minimo(file_path)
    if df_arcos is None:
        return {"status": "error", "mensagem": "Erro ao carregar o arquivo"}
    
    # Encontra origem (oferta positiva) e destino (demanda negativa)
    nos = list(range(1, len(ofertas_demandas) + 1))
    origens = [str(i) for i, o in zip(nos, ofertas_demandas) if o > 0]
    destinos = [str(i) for i, o in zip(nos, ofertas_demandas) if o < 0]
    
    # Criar o modelo
    arcos = [(row['de'], row['para']) for idx, row in df_arcos.iterrows()]
    custo = {(row['de'], row['para']): row['custo'] for idx, row in df_arcos.iterrows()}
    capacidade = {(row['de'], row['para']): row['capacidade'] for idx, row in df_arcos.iterrows()}
    
    x = pulp.LpVariable.dicts("x", arcos, lowBound=0, upBound=None, cat=pulp.LpInteger)
    model = pulp.LpProblem("MinCostFlow", pulp.LpMinimize)
    
    # Função objetivo: minimizar custo total
    model += pulp.lpSum([custo[a] * x[a] for a in arcos])
    
    # Restrições de conservação de fluxo com ofertas/demandas
    for i, oferta in enumerate(ofertas_demandas, 1):
        no = str(i)
        saidas = [a for a in arcos if a[0] == no]
        entradas = [a for a in arcos if a[1] == no]
        
        model += pulp.lpSum([x[a] for a in saidas]) - pulp.lpSum([x[a] for a in entradas]) == oferta
    
    # Restrições de capacidade
    for a in arcos:
        model += x[a] <= capacidade[a]
    
    # Resolver
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    
    # Converte as tuplas para strings para evitar problemas de serialização
    arcos_usados = {}
    for a in arcos:
        flow = pulp.value(x[a])
        if flow > 1e-5:
            arco_str = f"{a[0]}->{a[1]}"
            arcos_usados[arco_str] = flow
    
    return {
        "status": pulp.LpStatus[model.status],
        "arcos_usados": arcos_usados,
        "custo_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
###############################
def resolver_max_flow(df_arcos, origem, destino):
    """ 
    df_arcos: DataFrame com colunas: 'de', 'para', 'capacidade'
    """
    nos = list(set(df_arcos['de']).union(df_arcos['para']))
    arcos = [(row['de'],row['para']) for idx, row in df_arcos.iterrows()]
    capacidade = {(row['de'],row['para']): row['capacidade'] for idx, row in df_arcos.iterrows()}
    x = pulp.LpVariable.dicts("x", arcos, lowBound=0, cat=pulp.LpInteger)
    model = pulp.LpProblem("MaxFlow", pulp.LpMaximize)
    # fluxo total que sai da origem
    model += pulp.lpSum([x[a] for a in arcos if a[0]==origem])
    for n in nos:
        if n==origem or n==destino:
            continue
        model += pulp.lpSum([x[a] for a in arcos if a[0]==n]) == pulp.lpSum([x[a] for a in arcos if a[1]==n])
    for a in arcos:
        model += x[a] <= capacidade[a]
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    
    # Converte as tuplas para strings para evitar problemas de serialização
    arcos_usados = {}
    for a in arcos:
        flow = pulp.value(x[a])
        if flow > 1e-5:
            arco_str = f"{a[0]}->{a[1]}"
            arcos_usados[arco_str] = flow
    
    return {
        "status": pulp.LpStatus[model.status],
        "fluxo_maximo": pulp.value(model.objective),
        "arcos_usados": arcos_usados,
        "tempo_solucao": solve_time
    }
###############################
def carregar_problema_fluxo_maximo(file_path):
    """
    Carrega um arquivo de problema de fluxo máximo no formato específico.
    
    Args:
        file_path: Caminho para o arquivo
        
    Returns:
        Tupla (DataFrame de arcos, nó origem, nó destino)
    """
    try:
        with open(file_path, 'r') as f:
            linhas = f.readlines()
        
        # A primeira linha contém o comentário, ignoramos
        # A segunda linha contém: número de nós, número de arcos, origem, destino
        partes = linhas[1].strip().split()
        if len(partes) >= 4:  # Verifica se tem todos os campos necessários
            n_nos, n_arcos, origem, destino = partes[0], partes[1], partes[2], partes[3]
        else:
            st.error("Formato inválido: a segunda linha deve conter número de nós, arcos, origem e destino")
            return None, None, None
            
        # Procura pela linha de cabeçalho
        linha_cabecalho = None
        for i, linha in enumerate(linhas[2:], 2):
            if "de,para,capacidade" in linha:
                linha_cabecalho = i
                break
        
        if linha_cabecalho is None:
            st.error("Não foi encontrado o cabeçalho 'de,para,capacidade' no arquivo")
            return None, None, None
            
        # O cabeçalho contém os nomes das colunas
        colunas = linhas[linha_cabecalho].strip().split(',')
        
        # As linhas seguintes contêm os arcos
        dados_arcos = []
        for i in range(linha_cabecalho + 1, len(linhas)):
            if linhas[i].strip():  # Ignora linhas vazias
                valores = linhas[i].strip().split(',')
                if len(valores) == len(colunas):
                    dados_arcos.append(dict(zip(colunas, valores)))
        
        # Converte para DataFrame
        df_arcos = pd.DataFrame(dados_arcos)
        
        # Converte coluna de capacidade para numérico
        if 'capacidade' in df_arcos.columns:
            df_arcos['capacidade'] = pd.to_numeric(df_arcos['capacidade'])
        
        return df_arcos, origem, destino
    except Exception as e:
        st.error(f"Erro ao carregar o arquivo de fluxo máximo: {str(e)}")
        return None, None, None

###############################
def resolver_max_flow_from_file(file_path):
    """
    Resolve o problema de fluxo máximo a partir de um arquivo.
    
    Args:
        file_path: Caminho para o arquivo
        
    Returns:
        Dicionário com os resultados
    """
    df_arcos, origem, destino = carregar_problema_fluxo_maximo(file_path)
    if df_arcos is None:
        return {"status": "error", "mensagem": "Erro ao carregar o arquivo"}
    
    # Criar o modelo
    nos = list(set(df_arcos['de']).union(df_arcos['para']))
    arcos = [(row['de'], row['para']) for idx, row in df_arcos.iterrows()]
    capacidade = {(row['de'], row['para']): row['capacidade'] for idx, row in df_arcos.iterrows()}
    
    x = pulp.LpVariable.dicts("x", arcos, lowBound=0, cat=pulp.LpInteger)
    model = pulp.LpProblem("MaxFlow", pulp.LpMaximize)
    
    # Função objetivo: maximizar fluxo que sai da origem
    model += pulp.lpSum([x[a] for a in arcos if a[0] == origem])
    
    # Restrições de conservação de fluxo
    for n in nos:
        if n == origem or n == destino:
            continue
        model += pulp.lpSum([x[a] for a in arcos if a[0] == n]) == pulp.lpSum([x[a] for a in arcos if a[1] == n])
    
    # Restrições de capacidade
    for a in arcos:
        model += x[a] <= capacidade[a]
    
    # Resolver
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    
    # Converte as tuplas para strings para evitar problemas de serialização
    arcos_usados = {}
    for a in arcos:
        flow = pulp.value(x[a])
        if flow > 1e-5:
            arco_str = f"{a[0]}->{a[1]}"
            arcos_usados[arco_str] = flow
    
    return {
        "status": pulp.LpStatus[model.status],
        "fluxo_maximo": pulp.value(model.objective),
        "arcos_usados": arcos_usados,
        "tempo_solucao": solve_time
    }
###############################
def resolver_nurse_scheduling(df_turnos, dias_consec):
    """
    df_turnos: DataFrame com colunas 'dia', 'demanda'
    dias_consec: integer número de dias consecutivos de trabalho
    """
    dias = list(df_turnos['dia'])
    demanda = {row['dia']: row['demanda'] for idx, row in df_turnos.iterrows()}
    max_nurses = int(max(demanda.values())*1.5) # Heurística
    enfermeiras = [f"E{i}" for i in range(1,max_nurses + 1)]
    x = pulp.LpVariable.dicts("x", ((n,d) for n in enfermeiras for d in dias), cat='Binary')
    model = pulp.LpProblem("NurseScheduling", pulp.LpMinimize)
    # Demanda de cada dia
    for d in dias:
        model += pulp.lpSum([x[(n,d)] for n in enfermeiras]) >= demanda[d]
    # Restrição de dias consecutivos (simples: cada enfermeira só pode trabalhar em blocos de dias_consec)
    for n in enfermeiras:
        # Corrigindo o problema de índice - só vamos até dias_consec dias consecutivos
        for inicio in range(len(dias)-dias_consec):
            # Vamos garantir que não tentamos acessar mais dias do que existem na lista
            final = min(inicio + dias_consec, len(dias)-1)
            # Não pode trabalhar mais do que o bloco permitido
            model += pulp.lpSum([x[(n,dias[k])] for k in range(inicio, final+1)]) <= dias_consec
    # Minimiza o total de enfermeiras alocadas
    model += pulp.lpSum([x[(n,d)] for n in enfermeiras for d in dias])
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    esc_escala = {(n,d): pulp.value(x[(n,d)]) for n in enfermeiras for d in dias if pulp.value(x[(n,d)])>0.5}
    return {
        "status": pulp.LpStatus[model.status],
        "escala": esc_escala,
        "tempo_solucao": solve_time
    }
###############################
def resolver_cutting_stock(df_ordens, df_padroes, larg_folha):
    """
    df_ordens: DataFrame 'item', 'qtd_pedida', 'comprimento'
    df_padroes: DataFrame com colunas: 'padrao', 'quantidades' (string: '2A+1B+0C')
    larg_folha: largura folha/rolo inteiro (int)
    """
    padroes = df_padroes['padrao'].tolist()
    itens = df_ordens['item'].tolist()
    qtd_pedida = {row['item']: row['qtd_pedida'] for idx, row in df_ordens.iterrows()}
    padrao_itens = {}
    for idx, row in df_padroes.iterrows():
        parts = row['quantidades'].replace('+',' ').split()
        padrao_itens[row['padrao']] = {p[1:]:int(p[0]) for p in parts}
    x = pulp.LpVariable.dicts("x", padroes, lowBound=0, cat=pulp.LpInteger)
    model = pulp.LpProblem("CuttingStock", pulp.LpMinimize)
    # Atender cada ordem
    for i in itens:
        model += pulp.lpSum([padrao_itens[p].get(i,0)*x[p] for p in padroes]) >= qtd_pedida[i]
    model += pulp.lpSum([x[p] for p in padroes])  # Minimiza número de folhas usadas
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    return {
        "status": pulp.LpStatus[model.status],
        "folhas_usadas": {p: pulp.value(x[p]) for p in padroes if pulp.value(x[p])>1e-5},
        "tempo_solucao": solve_time
    }
###############################
def resolver_frequency_assignment(df_antenas, df_conflitos):
    """
    df_antenas: DataFrame col 'antena'
    df_conflitos: DataFrame colunas 'antena1', 'antena2' (arestas do grafo de conflitos)
    """
    antenas = df_antenas['antena'].tolist()
    x = pulp.LpVariable.dicts("freq", antenas, lowBound=1, cat=pulp.LpInteger)
    model = pulp.LpProblem("Assignment", pulp.LpMinimize)
    # Minimizar a maior frequência usada
    model += pulp.lpMax([x[a] for a in antenas])
    for idx, row in df_conflitos.iterrows():
        # Antenas em conflito não podem pegar mesma frequência
        model += x[row['antena1']] != x[row['antena2']]
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    return {
        "status": pulp.LpStatus[model.status],
        "atribuição_frequencias": {a:pulp.value(x[a]) for a in antenas},
        "tempo_solucao": solve_time
    }
###############################
def resolver_max_clique(df_adj):
    """
    df_adj: DataFrame de adjacência binária (nodos x nodos)
    """
    nodos = df_adj.index.tolist()
    x = pulp.LpVariable.dicts("x", nodos, cat="Binary")
    model = pulp.LpProblem("MaxClique", pulp.LpMaximize)
    model += pulp.lpSum([x[i] for i in nodos])
    for i in nodos:
        for j in nodos:
            if i != j and df_adj.loc[i,j] == 0:
                model += x[i] + x[j] <= 1
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    clique = [i for i in nodos if pulp.value(x[i]) > 0.5]
    return {
        "status": pulp.LpStatus[model.status],
        "clique_maxima": clique,
        "tamanho": len(clique),
        "tempo_solucao": solve_time
    }
###############################
def resolver_crop_planning(df_fazendas, df_culturas, df_restricoes):
    """
    df_fazendas: DataFrame colunas 'fazenda', 'area_max', 'agua_max'
    df_culturas: DataFrame colunas 'cultura', 'lucro', 'agua_por_area'
    df_restricoes: DataFrame colunas 'fazenda', 'cultura', 'proporcao_min', 'proporcao_max'
    """
    fazendas = df_fazendas['fazenda'].tolist()
    culturas = df_culturas['cultura'].tolist()
    x = pulp.LpVariable.dicts("planta", ((f, c) for f in fazendas for c in culturas), lowBound=0, cat=pulp.LpInteger)
    model = pulp.LpProblem("Plantio", pulp.LpMaximize)
    lucro = {row['cultura']: row['lucro'] for idx, row in df_culturas.iterrows()}
    agua = {row['cultura']: row['agua_por_area'] for idx, row in df_culturas.iterrows()}
    model += pulp.lpSum([x[(f,c)]*lucro[c] for f in fazendas for c in culturas])
    for f in fazendas:
        model += pulp.lpSum([x[(f,c)] for c in culturas]) <= float(df_fazendas[df_fazendas['fazenda']==f]['area_max'])
        model += pulp.lpSum([x[(f,c)]*agua[c] for c in culturas]) <= float(df_fazendas[df_fazendas['fazenda']==f]['agua_max'])
        for c in culturas:
            restr = df_restricoes[(df_restricoes['fazenda']==f)&(df_restricoes['cultura']==c)]
            if not restr.empty:
                model += x[(f,c)] >= float(restr['proporcao_min'])*float(df_fazendas[df_fazendas['fazenda']==f]['area_max'])
                model += x[(f,c)] <= float(restr['proporcao_max'])*float(df_fazendas[df_fazendas['fazenda']==f]['area_max'])
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    plantio = {(f,c):pulp.value(x[(f,c)]) for f in fazendas for c in culturas if pulp.value(x[(f,c)])>1e-5}
    return {
        "status": pulp.LpStatus[model.status],
        "plantio": plantio,
        "lucro_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
###############################
def resolver_blending_tintas(df_produtos, df_componentes, df_disp):
    """
    df_produtos: DataFrame com colunas 'produto', 'preco_venda'
    df_componentes: DataFrame com colunas 'produto', 'componente', 'proporcao'
    df_disp: DataFrame colunas 'componente', 'disponivel'
    """
    produtos = df_produtos['produto'].tolist()
    componentes = df_disp['componente'].tolist()
    x = pulp.LpVariable.dicts("x", produtos, lowBound=0, cat=pulp.LpInteger)
    model = pulp.LpProblem("Tintas", pulp.LpMaximize)
    preco = {rows['produto']: rows['preco_venda'] for i,rows in df_produtos.iterrows()}
    model += pulp.lpSum([x[p]*preco[p] for p in produtos])
    # Restrição: Não ultrapassar disponível de cada componente
    for comp in componentes:
        proporcoes = {row['produto']: row['proporcao'] for idx, row in df_componentes[df_componentes['componente']==comp].iterrows()}
        disponivel = float(df_disp[df_disp['componente']==comp]['disponivel'])
        model += pulp.lpSum([x[p]*proporcoes.get(p,0) for p in produtos]) <= disponivel
    start_time = time.time()
    model.solve(pulp.PULP_CBC_CMD(msg=False))
    solve_time = time.time() - start_time
    return {
        "status": pulp.LpStatus[model.status],
        "producao": {p: pulp.value(x[p]) for p in produtos if pulp.value(x[p])>1e-5},
        "lucro_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
###############################
def resolver_caminho_minimo(df_arcos, origem, destino):
    result = resolver_min_cost_flow(df_arcos, origem, destino)
    return result
###############################
def resolver_modelo_generico(filepath):
    """
    Resolve um modelo genérico de PLI a partir de um arquivo .mps ou .lp.
    """
    try:
        start_time = time.time()
        ext = os.path.splitext(filepath)[1].lower()
        if ext == '.mps':
            # Carrega o modelo MPS através do PuLP
            _, model = pulp.LpProblem.fromMPS(filepath)
            model.solve(pulp.PULP_CBC_CMD(msg=False))
            valor_objetivo = pulp.value(model.objective)
            status = pulp.LpStatus[model.status]
        elif ext == '.lp':
            # Usa CBC externo para resolver LP (encontra o binário do CBC no PATH)
            output_sol = filepath + ".sol"
            result = subprocess.run(
                ['cbc', filepath, 'solve', 'solu', output_sol], 
                capture_output=True, text=True, timeout=120)
            if not os.path.exists(output_sol):
                raise Exception("Erro ao chamar o solver CBC para arquivo LP:\n" + result.stdout + "\n" + result.stderr)
            # Parse resultado
            valor_objetivo, status = None, "Erro"
            # Procura o valor objetivo no arquivo de solução
            with open(output_sol, 'r', encoding="utf-8") as fin:
                for line in fin:
                    if line.startswith("Optimal - objective value"):
                        m = re.search(r"Optimal - objective value\s+([-\d.]+)", line)
                        if m:
                            valor_objetivo = float(m.group(1))
                            status = "Optimal"
                        break
            # Limpa arquivo temporário
            os.remove(output_sol)
        else:
            raise ValueError("Formato de arquivo não suportado. Só é aceito .mps ou .lp.")

        solve_time = time.time() - start_time
        return {
            "status": status,
            "valor_objetivo": valor_objetivo,
            "tempo_solucao": solve_time
        }
    except Exception as e:
        st.error(f"Erro ao resolver o modelo genérico: {str(e)}")
        return {
            "status": "Erro",
            "valor_objetivo": None,
            "tempo_solucao": None
        }
###############################
def app():
    st.header("Programação Linear Inteira (PLI)")
    st.markdown("""
    A Programação Linear Inteira (PLI) resolve problemas exatos de otimização combinatória. 
    Escolha abaixo qual problema deseja modelar e resolvê-lo.
                
    Obs.: Removido por ora o  "Modelo Genérico",
    """)
    problema = st.selectbox(
        "Selecione o Problema de PLI a resolver:",
        [
            "Problema da Mochila",
            "Problema da Dieta",
            "Problema da Ração",
            "Problema do Caixeiro Viajante",
            "Problema de Cobertura (Set Covering)",
            "Problema de Facility Location",
            "Problema de Fluxo de Custo Mínimo",
            "Problema do Caminho Mínimo",
            "Problema do Fluxo Máximo",
            "Problema de Escalonamento de Horários",
            "Problema de Padrões (Cutting Stock)",
            "Problema de Frequência (Frequency Assignment)",
            "Problema da Clique Máxima",
            "Problema do Plantio",
            "Problema das Tintas",
        ]
    )
    # 1. MODELO GENÉRICO
    if problema == "Modelo Genérico":
        st.markdown("""
            ### Modelo Genérico de Programação Linear Inteira
            Faça upload de um modelo `.mps` ou `.lp`
        """)
        upfile = st.file_uploader("Arquivo do modelo (.mps ou .lp)", type=["mps","lp"])
        if upfile is not None:
            with open("temp_model_"+upfile.name, "wb") as f:
                f.write(upfile.read())
            if st.button("Resolver Modelo"):
                with st.spinner("Resolvendo..."):
                    resultados = resolver_modelo_generico("temp_model_"+upfile.name)
                    st.success(f"Status: {resultados['status']}")
                    if resultados and 'valor_objetivo' in resultados and resultados['valor_objetivo'] is not None:
                        st.metric("Valor objetivo", f"{resultados['valor_objetivo']:.6f}")
                    else:
                        st.error("Não foi possível calcular o valor objetivo. Verifique os erros acima.")
                    st.metric("Tempo de resolução", f"{resultados['tempo_solucao']:.4f} seg")

    # 2. PROBLEMA DA MOCHILA (já estava implementado!)
    elif problema == "Problema da Mochila":
        st.markdown("""
        ### Problema da Mochila (Knapsack Problem)
        
        O problema da mochila consiste em selecionar itens com pesos e valores,
        maximizando o valor total sem exceder a capacidade da mochila.
        
        **Formato do arquivo esperado:**
        - CSV com colunas: item, peso, valor
        - Última linha deve conter a capacidade da mochila
        
        **Exemplo:**
        ```
        item,peso,valor
        1,10,60
        2,20,100
        3,30,120
        capacidade,50,0
        ```
        """)
        
        uploaded_file = st.file_uploader("Carregar instância do Problema da Mochila", type=["csv", "txt"])
        
        if uploaded_file is not None:
            df = load_csv_file(uploaded_file)
            
            if df is not None:
                # Extrair a capacidade (última linha)
                try:
                    capacidade = float(df.iloc[-1, 1])
                    df = df[:-1]  # Remover a última linha
                    
                    st.write("Dados carregados:")
                    st.dataframe(df)
                    st.write(f"Capacidade da mochila: {capacidade}")
                    
                    if st.button("Resolver Problema da Mochila"):
                        with st.spinner("Resolvendo o problema..."):
                            resultados = resolver_problema_mochila(df, capacidade)
                            
                            if resultados["status"] == "erro":
                                st.error(resultados["mensagem"])
                            else:
                                st.success(f"Status da solução: {resultados['status']}")
                                st.metric("Valor total na mochila", f"{resultados['valor_total']:.2f}")
                                st.metric("Tempo de resolução", f"{resultados['tempo_solucao']:.4f} segundos")
                                
                                # Itens selecionados
                                if resultados["itens_selecionados"]:
                                    st.write("### Itens selecionados")
                                    st.dataframe(pd.DataFrame(resultados["itens_selecionados"]))
                                    st.write(f"Peso total utilizado: {resultados['peso_total']:.2f} de {capacidade:.2f} ({resultados['uso_capacidade']:.2f}%)")
                except Exception as e:
                    st.error(f"Erro ao processar o arquivo: {str(e)}")

    # 3. PROBLEMA DA DIETA (já estava implementado!)
    elif problema == "Problema da Dieta":
        st.markdown("""
        ### Problema da Dieta
        
        O problema da dieta consiste em selecionar alimentos que satisfaçam requisitos nutricionais
        minimizando o custo total.
        
        **Formato dos arquivos esperados:**
        
        **1. Arquivo de Alimentos:**
        - CSV com colunas: alimento, custo, e uma coluna para cada nutriente
        - Exemplo:
        ```
        alimento,custo,proteina,lipidios,carboidratos,calorias
        Arroz,2.30,2.5,0.2,28.0,124
        Feijão,5.50,7.0,0.5,14.0,77
        Carne,25.00,26.0,15.0,0.0,288
        ```
        
        **2. Arquivo de Requisitos Nutricionais:**
        - CSV com colunas: nutriente, minimo, maximo
        - Exemplo:
        ```
        nutriente,minimo,maximo
        proteina,50,100
        lipidios,30,50
        carboidratos,200,300
        calorias,1800,2200
        ```
        """)
        
        # Interface de upload para os dois arquivos
        col1, col2 = st.columns(2)
        with col1:
            uploaded_alimentos = st.file_uploader("Carregar arquivo de alimentos", type=["csv", "txt"], key="alimentos")
        with col2:
            uploaded_nutrientes = st.file_uploader("Carregar arquivo de requisitos nutricionais", type=["csv", "txt"], key="nutrientes")
        
        # Se ambos os arquivos foram enviados
        if uploaded_alimentos is not None and uploaded_nutrientes is not None:
            df_alimentos = load_csv_file(uploaded_alimentos)
            df_nutrientes = load_csv_file(uploaded_nutrientes)
            
            if df_alimentos is not None and df_nutrientes is not None:
                # Mostrar os dados carregados
                st.subheader("Dados de Alimentos")
                st.dataframe(df_alimentos)
                
                st.subheader("Requisitos Nutricionais")
                st.dataframe(df_nutrientes)
                
                if st.button("Resolver Problema da Dieta"):
                    with st.spinner("Resolvendo o problema..."):
                        resultados = resolver_problema_dieta(df_alimentos, df_nutrientes)
                        
                        if resultados["status"] == "erro":
                            st.error(resultados["mensagem"])
                        else:
                            st.success(f"Status da solução: {resultados['status']}")
                            st.metric("Custo total da dieta", f"R$ {resultados['custo_total']:.2f}")
                            st.metric("Tempo de resolução", f"{resultados['tempo_solucao']:.4f} segundos")
                            
                            # Alimentos selecionados
                            if resultados["alimentos_selecionados"]:
                                st.write("### Alimentos selecionados")
                                
                                # Criar DataFrame para visualização
                                df_selecionados = pd.DataFrame(resultados["alimentos_selecionados"])
                                df_selecionados["Quantidade"] = df_selecionados["Quantidade"].apply(lambda x: f"{x:.2f}")
                                
                                # Mostrar tabela de alimentos selecionados
                                st.dataframe(df_selecionados)
                                
                                # Visualizar distribuição dos alimentos
                                fig, ax = plt.subplots(figsize=(10, 6))
                                alimentos = [item["Alimento"] for item in resultados["alimentos_selecionados"]]
                                quantidades = [item["Quantidade"] for item in resultados["alimentos_selecionados"]]
                                
                                # Converter strings para float
                                quantidades = [float(q) for q in quantidades]
                                
                                ax.bar(alimentos, quantidades, color='skyblue')
                                ax.set_title('Quantidade de Cada Alimento na Dieta Ótima')
                                ax.set_xlabel('Alimentos')
                                ax.set_ylabel('Quantidade')
                                plt.xticks(rotation=45, ha='right')
                                plt.tight_layout()
                                st.pyplot(fig)
                                
                                # Nutrientes obtidos
                                st.write("### Nutrientes obtidos")
                                
                                # Criar DataFrame para visualização dos nutrientes
                                nutrientes_data = []
                                for nutriente, valores in resultados["nutrientes_obtidos"].items():
                                    nutrientes_data.append({
                                        "Nutriente": nutriente,
                                        "Obtido": f"{valores['obtido']:.2f}",
                                        "Mínimo Requerido": f"{valores['minimo']:.2f}",
                                        "Máximo Permitido": valores['maximo']
                                    })
                                
                                st.dataframe(pd.DataFrame(nutrientes_data))
                                
                                # Visualizar comparação entre nutrientes obtidos e limites
                                fig2, ax2 = plt.subplots(figsize=(12, 6))
                                
                                nutrientes = list(resultados["nutrientes_obtidos"].keys())
                                obtidos = [resultados["nutrientes_obtidos"][n]["obtido"] for n in nutrientes]
                                minimos = [resultados["nutrientes_obtidos"][n]["minimo"] for n in nutrientes]
                                
                                maximos = []
                                for n in nutrientes:
                                    max_val = resultados["nutrientes_obtidos"][n]["maximo"]
                                    if max_val == "Ilimitado":
                                        # Usar um valor um pouco maior que o obtido para visualização
                                        maximos.append(resultados["nutrientes_obtidos"][n]["obtido"] * 1.2)
                                    else:
                                        maximos.append(float(max_val))
                                
                                x = np.arange(len(nutrientes))
                                width = 0.25
                                
                                ax2.bar(x - width, obtidos, width, label='Obtido', color='green')
                                ax2.bar(x, minimos, width, label='Mínimo', color='red')
                                ax2.bar(x + width, maximos, width, label='Máximo', color='blue')
                                
                                ax2.set_title('Comparação de Nutrientes')
                                ax2.set_xlabel('Nutrientes')
                                ax2.set_ylabel('Quantidade')
                                ax2.set_xticks(x)
                                ax2.set_xticklabels(nutrientes)
                                ax2.legend()
                                
                                plt.tight_layout()
                                st.pyplot(fig2)
    
    # 4. PROBLEMA DA RAÇÃO
    elif problema == "Problema da Ração":
        st.markdown("""
        ### Problema da Ração (Feed Problem)
        Faça upload dos arquivos:
        - Produtos: `produto, lucro`
        - Ingredientes: `produto, ingrediente, uso`
        - Restrições: `ingrediente, min, max`
        """)
        up1 = st.file_uploader("Produtos", type=["csv","txt"], key="prod_racao")
        up2 = st.file_uploader("Ingredientes", type=["csv","txt"], key="ing_racao")
        up3 = st.file_uploader("Restrições", type=["csv","txt"], key="rest_racao")
        if up1 and up2 and up3:
            df1, df2, df3 = load_csv_file(up1), load_csv_file(up2), load_csv_file(up3)
            if st.button("Resolver Ração"):
                with st.spinner("Resolvendo..."):
                    res = resolver_feed_problem(df1, df2, df3)
                    st.success(f"Status: {res['status']}")
                    st.write("Produção ótima de cada produto:", res["quantidade_produto"])
                    st.metric("Lucro total", f"R$ {res['lucro_total']:.2f}")
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 5. PROBLEMA DO CAIXEIRO VIAJANTE (já estava implementado!)
    elif problema == "Problema do Caixeiro Viajante":
        st.markdown("""
        ### Problema do Caixeiro Viajante (TSP)

        Formato esperado: CSV NxN, linhas e colunas nomeadas com as cidades.
        Exemplo:
        ```
        cidade,A,B,C,D,E
        A,0,10,15,20,8
        B,10,0,12,15,16
        C,15,12,0,6,14
        D,20,15,6,0,10
        E,8,16,14,10,0
        ```
        """)
        uploaded_file = st.file_uploader("Carregar matriz de distâncias (CSV)", type=["csv", "txt"])
        if uploaded_file is not None:
            # Carregue o arquivo diretamente como CSV, definindo a primeira coluna como índice
            try:
                df = pd.read_csv(uploaded_file, index_col=0)
                
                # Limpe os nomes das colunas (sem converter índice para evitar MultiIndex)
                df.columns = [str(col).strip() for col in df.columns]
                
                # Verifique se o dataframe tem o formato esperado
                if df.shape[0] != df.shape[1]:
                    st.error("O arquivo não contém uma matriz quadrada de distâncias.")
                else:
                    st.write("Matriz de distâncias:")
                    st.dataframe(df)
                    if st.button("Resolver TSP"):
                        with st.spinner("Resolvendo..."):
                            resultados = resolver_tsp(df)
                            if resultados["status"] != "Optimal":
                                st.error("Problema não pôde ser resolvido.")
                            else:
                                st.success(f"Status: {resultados['status']}")
                                st.metric("Custo total", f"{resultados['custo_total']:.2f}")
                                st.write("Rota ótima:")
                                st.write(" → ".join(map(str, resultados["rota"])))
                                st.metric("Tempo de resolução", f"{resultados['tempo_solucao']:.4f} segundos")
            except Exception as e:
                st.error(f"Erro ao processar o arquivo: {str(e)}")

    # 6. PROBLEMA DE COBERTURA (já estava implementado!)
    elif problema == "Problema de Cobertura (Set Covering)":
        st.markdown("""
        ### Problema de Cobertura

        Formato: matriz de incidência binária (linhas = elementos, colunas = conjuntos).
        """)
        uploaded_file = st.file_uploader("Carregar matriz de cobertura (CSV)", type=["csv", "txt"])
        if uploaded_file is not None:
            df = load_csv_file(uploaded_file)
            if df is not None:
                st.dataframe(df)
                if st.button("Resolver Set Covering"):
                    with st.spinner("Resolvendo..."):
                        resultados = resolver_set_cover(df)
                        if resultados["status"] != "Optimal":
                            st.error("Problema não pôde ser resolvido.")
                        else:
                            st.success(f"Status: {resultados['status']}")
                            st.metric("Cobertura mínima", resultados["num_selecionados"])
                            st.write("Conjuntos escolhidos:")
                            st.write(resultados["selecionados"])
                            st.metric("Tempo de resolução", f"{resultados['tempo_solucao']:.4f} segundos")

    # 7. FACILITY LOCATION (já estava implementado!)
    elif problema == "Problema de Facility Location":
        st.markdown("""
        ### Facility Location

        Carregue dois arquivos:
        1. Depos Depósitos (facility, fixo)
        2. Clientes e custos (facility, cliente, custo)
        """)
        col1, col2 = st.columns(2)
        with col1:
            uploaded_facility = st.file_uploader("CSV de depósitos", type=["csv"], key="facility")
        with col2:
            uploaded_clientes = st.file_uploader("CSV de clientes", type=["csv"], key="clientes")
        if uploaded_facility and uploaded_clientes:
            df_facility = load_csv_file(uploaded_facility)
            df_clientes = load_csv_file(uploaded_clientes)
            if df_facility is not None and df_clientes is not None:
                st.write("Depósitos:")
                st.dataframe(df_facility)
                st.write("Clientes:")
                st.dataframe(df_clientes)
                if st.button("Resolver Facility Location"):
                    with st.spinner("Resolvendo..."):
                        resultados = resolver_facility_location(df_facility, df_clientes)
                        if resultados["status"] != "Optimal":
                            st.error("Problema não pôde ser resolvido.")
                        else:
                            st.success(f"Status: {resultados['status']}")
                            st.metric("Custo total", f"{resultados['custo_total']:.2f}")
                            st.write("Depósitos abertos:", resultados["abertos"])
                            st.write("Alocação cliente → depósito:")
                            st.write(resultados["alocacoes"])
                            st.metric("Tempo de resolução", f"{resultados['tempo_solucao']:.4f} segundos")

    # 8. FLUXO DE CUSTO MÍNIMO
    elif problema == "Problema de Fluxo de Custo Mínimo":
        st.markdown("""
        ### Fluxo de Custo Mínimo
        
        Você pode resolver de duas maneiras:
        
        1. **Arquivo formatado específico**: 
           - Primeira linha: comentário
           - Segunda linha: número de nós, número de arcos
           - Terceira linha: ofertas/demandas para cada nó (positivo=oferta, negativo=demanda)
           - Quarta linha: cabeçalho (de,para,custo,capacidade)
           - Linhas seguintes: dados dos arcos
        
        2. **CSV genérico**:
           - CSV com colunas: `de,para,custo,capacidade`
           - Informe manualmente nós de origem e destino
        """)
        
        opcao = st.radio("Escolha o método de entrada:", ["Arquivo formatado específico", "CSV genérico"])
        
        if opcao == "Arquivo formatado específico":
            up = st.file_uploader("Arquivo de problema", type=["txt"])
            if up:
                # Salvar o arquivo temporariamente
                with open("temp_flow_problem.txt", "wb") as f:
                    f.write(up.read())
                
                if st.button("Resolver Fluxo de Custo Mínimo"):
                    with st.spinner("Resolvendo..."):
                        res = resolver_min_cost_flow_from_file("temp_flow_problem.txt")
                        if res.get("status") == "error":
                            st.error(res["mensagem"])
                        else:
                            st.success(f"Status: {res['status']}")
                            st.write("Fluxos ótimos usados:", res["arcos_usados"])
                            st.metric("Custo total", f"{res['custo_total']:.2f}")
                            st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")
        else:
            up = st.file_uploader("Arquivo de arcos", type=["csv", "txt"])
            origem = st.text_input("Nó origem")
            destino = st.text_input("Nó destino")
            if up and origem and destino:
                df = load_csv_file(up)
                if st.button("Resolver Fluxo de Custo Mínimo"):
                    with st.spinner("Resolvendo..."):
                        res = resolver_min_cost_flow(df, origem, destino)
                        st.success(f"Status: {res['status']}")
                        st.write("Fluxos ótimos usados:", res["arcos_usados"])
                        st.metric("Custo total", f"{res['custo_total']:.2f}")
                        st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 9. CAMINHO MÍNIMO    
    elif problema == "Problema do Caminho Mínimo":
        st.markdown("""
        ### Caminho Mínimo
        CSV como no Fluxo de Custo Mínimo. Informe origem e destino.
        """)
        up = st.file_uploader("Arquivo de arcos", type=["csv","txt"], key="cm_arcos")
        origem = st.text_input("Nó origem","",key="cm_origem")
        destino = st.text_input("Nó destino","",key="cm_destino")
        if up and origem and destino:
            df = load_csv_file(up)
            if st.button("Resolver Caminho Mínimo"):
                with st.spinner("Resolvendo..."):
                    res = resolver_caminho_minimo(df, origem, destino)
                    st.success(f"Status: {res['status']}")
                    st.write("Arcos utilizados:", res["arcos_usados"])
                    st.metric("Custo total", f"{res['custo_total']:.2f}")
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 10. FLUXO MÁXIMO
    elif problema == "Problema do Fluxo Máximo":
        st.markdown("""
        ### Fluxo Máximo
        
        Você pode resolver de duas maneiras:
        
        1. **Arquivo formatado específico**: 
           - Primeira linha: comentário
           - Segunda linha: número de nós, número de arcos, origem, destino
           - (Linhas em branco opcionais)
           - Linha com cabeçalho: de,para,capacidade
           - Linhas seguintes: dados dos arcos
           
        2. **CSV genérico**:
           - CSV com colunas: `de,para,capacidade`
           - Informe manualmente nós de origem e destino
        """)
        
        opcao = st.radio("Escolha o método de entrada:", ["Arquivo formatado específico", "CSV genérico"], key="fm_opcao")
        
        if opcao == "Arquivo formatado específico":
            up = st.file_uploader("Arquivo de problema", type=["txt"], key="fm_arq_esp")
            if up:
                # Salvar o arquivo temporariamente
                with open("temp_maxflow_problem.txt", "wb") as f:
                    f.write(up.read())
                
                if st.button("Resolver Fluxo Máximo"):
                    with st.spinner("Resolvendo..."):
                        res = resolver_max_flow_from_file("temp_maxflow_problem.txt")
                        if res.get("status") == "error":
                            st.error(res["mensagem"])
                        else:
                            st.success(f"Status: {res['status']}")
                            st.metric("Fluxo máximo", f"{res['fluxo_maximo']:.2f}")
                            st.write("Arcos utilizados:", res["arcos_usados"])
                            st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")
        else:
            up = st.file_uploader("Arquivo de arcos", type=["csv","txt"], key="fm_arcos")
            origem = st.text_input("Nó origem","",key="fm_origem")
            destino = st.text_input("Nó destino","",key="fm_destino")
            if up and origem and destino:
                df = load_csv_file(up)
                if st.button("Resolver Fluxo Máximo"):
                    with st.spinner("Resolvendo..."):
                        res = resolver_max_flow(df, origem, destino)
                        st.success(f"Status: {res['status']}")
                        st.metric("Fluxo máximo", f"{res['fluxo_maximo']:.2f}")
                        st.write("Arcos utilizados:", res["arcos_usados"])
                        st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 11. ESCALONAMENTO DE HORÁRIOS
    elif problema == "Problema de Escalonamento de Horários":
        st.markdown("""
        ### Escalonamento de Horários (Nurse Scheduling)
        CSV: `dia,demanda`. Informe quantidade de dias consecutivos de trabalho permitida.
        """)
        up = st.file_uploader("Demanda por dia", type=["csv","txt"])
        dias_consec = st.number_input("Dias consecutivos permitidos", min_value=1, value=2)
        if up:
            df = load_csv_file(up)
            if st.button("Resolver Escalonamento"):
                with st.spinner("Resolvendo..."):
                    res = resolver_nurse_scheduling(df, dias_consec)
                    st.success(f"Status: {res['status']}")
                    
                    # Formatação melhorada da escala
                    if res["status"] == "Optimal":
                        # Extrair dados da escala
                        escala = res["escala"]
                        
                        # Criar DataFrame para melhor visualização
                        df_dias = list(set([dia for _, dia in escala.keys()]))
                        df_dias.sort() # Ordenar os dias
                        
                        df_enf = list(set([enf for enf, _ in escala.keys()]))
                        df_enf.sort() # Ordenar as enfermeiras
                        
                        # Criar matriz de escala
                        matriz_escala = pd.DataFrame(0, index=df_enf, columns=df_dias)
                        
                        for (enf, dia), valor in escala.items():
                            if valor > 0.5:  # Para considerar arredondamento
                                matriz_escala.loc[enf, dia] = 1
                        
                        # Exibir resumo
                        st.subheader("Resumo da Escala")
                        total_turnos = sum(escala.values())
                        st.write(f"Total de turnos alocados: {total_turnos}")
                        
                        # Calcular a demanda total
                        demanda_total = sum([df.loc[df['dia'] == dia, 'demanda'].values[0] for dia in df_dias])
                        st.write(f"Demanda total de turnos: {demanda_total}")
                        
                        # Utilização de recursos
                        eficiencia = (total_turnos / (len(df_enf) * len(df_dias))) * 100
                        st.metric("Eficiência da escala", f"{eficiencia:.2f}%")
                        
                        # Exibir tabela de escala
                        st.subheader("Tabela de Escala")
                        # Substituir 0 por '-' e 1 por 'X' para melhor visualização
                        st.dataframe(matriz_escala.replace({0: '-', 1: 'X'}))
                        
                        # Exibir visualização gráfica da escala
                        st.subheader("Visualização da Escala")
                        fig, ax = plt.subplots(figsize=(10, max(6, len(df_enf) * 0.4)))
                        
                        # Criar heatmap da escala
                        sns_heatmap = None
                        try:
                            import seaborn as sns
                            sns_heatmap = sns.heatmap(matriz_escala, cmap="YlGnBu", 
                                                    cbar=False, linewidths=.5, 
                                                    linecolor='gray', ax=ax)
                            # Adicionar textos
                            for i in range(len(df_enf)):
                                for j in range(len(df_dias)):
                                    if matriz_escala.iloc[i, j] > 0.5:
                                        ax.text(j + 0.5, i + 0.5, "✓", 
                                                ha="center", va="center", 
                                                color="darkgreen", fontweight="bold")
                        except ImportError:
                            # Se seaborn não estiver disponível, usar matplotlib
                            ax.imshow(matriz_escala, cmap='Blues', aspect='auto')
                            ax.set_xticks(range(len(df_dias)))
                            ax.set_xticklabels(df_dias)
                            ax.set_yticks(range(len(df_enf)))
                            ax.set_yticklabels(df_enf)
                            for i in range(len(df_enf)):
                                for j in range(len(df_dias)):
                                    if matriz_escala.iloc[i, j] > 0.5:
                                        ax.text(j, i, "✓", ha="center", va="center", 
                                                color="darkgreen", fontweight="bold")
                        
                        ax.set_title('Escala de Trabalho')
                        ax.set_xlabel('Dias')
                        ax.set_ylabel('Enfermeiras')
                        
                        plt.tight_layout()
                        st.pyplot(fig)
                        
                        # Adicionar métricas por dia
                        st.subheader("Estatísticas por Dia")
                        metricas_dia = []
                        for dia in df_dias:
                            demanda_dia = df.loc[df['dia'] == dia, 'demanda'].values[0]
                            alocados_dia = matriz_escala[dia].sum()
                            metricas_dia.append({
                                'Dia': dia,
                                'Demanda': demanda_dia,
                                'Enfermeiras Alocadas': alocados_dia,
                                'Diferença': alocados_dia - demanda_dia
                            })
                        
                        st.dataframe(pd.DataFrame(metricas_dia))
                        
                        # Exibir o dicionário de escala original (opcional, pode ser ocultado)
                        with st.expander("Ver dados brutos da escala"):
                            st.write("Escala (enfermeira, dia):", res["escala"])
                    else:
                        st.error(f"Não foi possível encontrar uma solução ótima. Status: {res['status']}")
                        
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 12. CUTTING STOCK
    elif problema == "Problema de Padrões (Cutting Stock)":
        st.markdown("""
        ### Cutting Stock
        - Ordem: `item,qtd_pedida,comprimento`
        - Padrões: `padrao,quantidades` (por exemplo: 1A+2B+0C)
        """)
        up1 = st.file_uploader("Demanda das ordens", type=["csv","txt"], key="cs_ord")
        up2 = st.file_uploader("Padrões de corte", type=["csv","txt"], key="cs_pad")
        larg = st.number_input("Largura da folha disponível", min_value=1, value=100)
        if up1 and up2:
            df1,df2 = load_csv_file(up1), load_csv_file(up2)
            if st.button("Resolver Cutting Stock"):
                with st.spinner("Resolvendo..."):
                    res = resolver_cutting_stock(df1, df2, larg)
                    st.success(f"Status: {res['status']}")
                    st.write("Folhas usadas de cada padrão:", res["folhas_usadas"])
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 13. FREQUENCY ASSIGNMENT
    elif problema == "Problema de Frequência (Frequency Assignment)":
        st.markdown("""
        ### Frequency Assignment
        - Antenas: `antena`
        - Conflitos: `antena1,antena2`
        """)
        up1 = st.file_uploader("Antenas", type=["csv","txt"], key="fa_antenas")
        up2 = st.file_uploader("Conflitos", type=["csv","txt"], key="fa_conf")
        if up1 and up2:
            df1, df2 = load_csv_file(up1), load_csv_file(up2)
            if st.button("Resolver Assignment"):
                with st.spinner("Resolvendo..."):
                    res = resolver_frequency_assignment(df1, df2)
                    st.success(f"Status: {res['status']}")
                    st.write("Atribuição de frequências:", res["atribuição_frequencias"])
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 14. CLIQUE MÁXIMA
    elif problema == "Problema da Clique Máxima":
        st.markdown("""
        ### Clique Máxima
        CSV: matriz de adjacência (binária), linhas e colunas = vértices
        """)
        up = st.file_uploader("Matriz de adjacência", type=["csv","txt"])
        if up:
            df = load_csv_file(up)
            if st.button("Resolver Clique Máxima"):
                with st.spinner("Resolvendo..."):
                    res = resolver_max_clique(df)
                    st.success(f"Status: {res['status']}")
                    st.write(f"Clique máxima: {res['clique_maxima']} (tamanho {res['tamanho']})")
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 15. PLANTIO
    elif problema == "Problema do Plantio":
        st.markdown("""
        ### Plantio (Crop Planning)
        - Fazendas: `fazenda,area_max,agua_max`
        - Culturas: `cultura,lucro,agua_por_area`
        - Restrições: `fazenda,cultura,proporcao_min,proporcao_max`
        """)
        up1 = st.file_uploader("Fazendas", type=["csv","txt"], key="cp_fazendas")
        up2 = st.file_uploader("Culturas", type=["csv","txt"], key="cp_culturas")
        up3 = st.file_uploader("Restrições", type=["csv","txt"], key="cp_restr")
        if up1 and up2 and up3:
            df1, df2, df3 = load_csv_file(up1), load_csv_file(up2), load_csv_file(up3)
            
            # Mostrar os dados de entrada
            if df1 is not None and df2 is not None and df3 is not None:
                col1, col2, col3 = st.columns(3)
                
                with col1:
                    st.write("Fazendas:")
                    st.dataframe(df1)
                
                with col2:
                    st.write("Culturas:")
                    st.dataframe(df2)
                
                with col3:
                    st.write("Restrições:")
                    st.dataframe(df3)
            
            if st.button("Resolver Plantio"):
                with st.spinner("Resolvendo..."):
                    res = resolver_crop_planning(df1, df2, df3)
                    if res["status"] == "Optimal":
                        st.success(f"Status: {res['status']}")
                        
                        # Preparar dados para visualização
                        plantio_list = []
                        fazendas = []
                        culturas = []
                        
                        for (fazenda, cultura), area in res["plantio"].items():
                            plantio_list.append({
                                "Fazenda": fazenda,
                                "Cultura": cultura,
                                "Área Plantada": float(area)
                            })
                            if fazenda not in fazendas:
                                fazendas.append(fazenda)
                            if cultura not in culturas:
                                culturas.append(cultura)
                        
                        # Criar DataFrame com os resultados
                        df_plantio = pd.DataFrame(plantio_list)
                        
                        # Mostrar resultados em formato tabular
                        st.subheader("Plano de Plantio")
                        st.dataframe(df_plantio.sort_values(by=["Fazenda", "Cultura"]))
                        
                        # Mostrar métrica de lucro
                        st.metric("Lucro total", f"R$ {res['lucro_total']:.2f}")
                        
                        # Estatísticas de uso de recursos
                        st.subheader("Estatísticas de Uso de Recursos")
                        
                        # Calcular estatísticas por fazenda
                        estatisticas_fazenda = []
                        for fazenda in fazendas:
                            area_max = float(df1[df1['fazenda'] == fazenda]['area_max'].values[0])
                            agua_max = float(df1[df1['fazenda'] == fazenda]['agua_max'].values[0])
                            
                            # Calcular área utilizada
                            df_fazenda = df_plantio[df_plantio['Fazenda'] == fazenda]
                            area_usada = df_fazenda['Área Plantada'].sum()
                            
                            # Calcular água utilizada
                            agua_por_area = {row['cultura']: row['agua_por_area'] for idx, row in df2.iterrows()}
                            agua_usada = sum(row['Área Plantada'] * agua_por_area[row['Cultura']] 
                                            for _, row in df_fazenda.iterrows())
                            
                            estatisticas_fazenda.append({
                                "Fazenda": fazenda,
                                "Área Total": area_max,
                                "Área Utilizada": area_usada,
                                "Utilização de Área (%)": (area_usada / area_max) * 100 if area_max > 0 else 0,
                                "Água Disponível": agua_max,
                                "Água Utilizada": agua_usada,
                                "Utilização de Água (%)": (agua_usada / agua_max) * 100 if agua_max > 0 else 0
                            })
                        
                        # Mostrar estatísticas
                        df_estatisticas = pd.DataFrame(estatisticas_fazenda)
                        st.dataframe(df_estatisticas)
                        
                        # Visualizações gráficas
                        st.subheader("Visualizações")
                        
                        col1, col2 = st.columns(2)
                        
                        # Gráfico 1: Distribuição de área por fazenda
                        with col1:
                            fig1, ax1 = plt.subplots(figsize=(10, 6))
                            df_pivot = df_plantio.pivot_table(
                                values='Área Plantada', 
                                index='Fazenda', 
                                columns='Cultura', 
                                aggfunc='sum'
                            ).fillna(0)
                            
                            df_pivot.plot(kind='bar', stacked=True, ax=ax1, colormap='viridis')
                            ax1.set_title('Distribuição de Área por Fazenda')
                            ax1.set_xlabel('Fazenda')
                            ax1.set_ylabel('Área Plantada')
                            ax1.legend(title='Cultura', bbox_to_anchor=(1.05, 1), loc='upper left')
                            plt.tight_layout()
                            st.pyplot(fig1)
                        
                        # Gráfico 2: Comparação de uso de recursos
                        with col2:
                            fig2, ax2 = plt.subplots(figsize=(10, 6))
                            
                            x = np.arange(len(fazendas))
                            width = 0.35
                            
                            # Extrair dados para o gráfico
                            uso_area = [row["Utilização de Área (%)"] for row in estatisticas_fazenda]
                            uso_agua = [row["Utilização de Água (%)"] for row in estatisticas_fazenda]
                            
                            # Criar barras
                            ax2.bar(x - width/2, uso_area, width, label='% Área Utilizada', color='skyblue')
                            ax2.bar(x + width/2, uso_agua, width, label='% Água Utilizada', color='salmon')
                            
                            # Personalizar gráfico
                            ax2.set_ylim(0, 105)  # Limitar a 105% para visualizar melhor
                            ax2.set_ylabel('Porcentagem de Utilização')
                            ax2.set_title('Utilização de Recursos por Fazenda')
                            ax2.set_xticks(x)
                            ax2.set_xticklabels(fazendas)
                            ax2.legend()
                            
                            # Adicionar rótulos de porcentagem acima das barras
                            for i, v in enumerate(uso_area):
                                ax2.text(i - width/2, v + 3, f'{v:.1f}%', ha='center', va='bottom')
                            for i, v in enumerate(uso_agua):
                                ax2.text(i + width/2, v + 3, f'{v:.1f}%', ha='center', va='bottom')
                            
                            plt.tight_layout()
                            st.pyplot(fig2)
                        
                        # Gráfico 3: Distribuição do lucro por cultura
                        fig3, ax3 = plt.subplots(figsize=(10, 6))
                        
                        # Calcular lucro por cultura
                        lucro_por_cultura = {}
                        lucros = {row['cultura']: row['lucro'] for idx, row in df2.iterrows()}
                        for _, row in df_plantio.iterrows():
                            cultura = row['Cultura']
                            area = row['Área Plantada']
                            lucro_cultura = area * lucros[cultura]
                            if cultura in lucro_por_cultura:
                                lucro_por_cultura[cultura] += lucro_cultura
                            else:
                                lucro_por_cultura[cultura] = lucro_cultura
                        
                        # Criar gráfico de pizza
                        culturas_list = list(lucro_por_cultura.keys())
                        valores = list(lucro_por_cultura.values())
                        
                        # Destacar a fatia maior
                        explode = [0.1 if v == max(valores) else 0 for v in valores]
                        
                        ax3.pie(valores, labels=culturas_list, autopct='%1.1f%%', 
                                startangle=90, shadow=True, explode=explode)
                        ax3.axis('equal')  # Garantir que o gráfico seja um círculo
                        ax3.set_title('Distribuição do Lucro por Cultura')
                        
                        # Adicionar legenda com os valores
                        legenda = [f'{c}: R$ {v:.2f}' for c, v in zip(culturas_list, valores)]
                        ax3.legend(legenda, loc='best', bbox_to_anchor=(1.0, 1))
                        
                        plt.tight_layout()
                        st.pyplot(fig3)
                        
                    else:
                        st.error(f"Não foi possível encontrar uma solução ótima. Status: {res['status']}")
                    
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")

    # 16. TINTAS
    elif problema == "Problema das Tintas":
        st.markdown("""
        ### Tintas (Blending Problem)
        - Produtos: `produto,preco_venda`
        - Componentes: `produto,componente,proporcao`
        - Disponibilidade: `componente,disponivel`
        """)
        up1 = st.file_uploader("Produtos", type=["csv","txt"], key="bl_prod")
        up2 = st.file_uploader("Componentes", type=["csv","txt"], key="bl_comp")
        up3 = st.file_uploader("Disponibilidade", type=["csv","txt"], key="bl_disp")
        if up1 and up2 and up3:
            df1, df2, df3 = load_csv_file(up1), load_csv_file(up2), load_csv_file(up3)
            if st.button("Resolver Tintas"):
                with st.spinner("Resolvendo..."):
                    res = resolver_blending_tintas(df1, df2, df3)
                    st.success(f"Status: {res['status']}")
                    st.write("Produção ótima:", res["producao"])
                    st.metric("Lucro total", f"{res['lucro_total']:.2f}")
                    st.metric("Tempo de resolução", f"{res['tempo_solucao']:.3f}s")



