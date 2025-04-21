import pandas as pd
import pulp
import time
import streamlit as st
import numpy as np
import matplotlib.pyplot as plt

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
        return pd.read_csv(file, sep=None, engine='python')
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
    elementos = df.index.tolist()
    conjuntos = df.columns.tolist()

    x = pulp.LpVariable.dicts("x", conjuntos, cat='Binary')
    model = pulp.LpProblem("Set_Covering", pulp.LpMinimize)

    # Objetivo: minimizar número de conjuntos
    model += pulp.lpSum([x[j] for j in conjuntos])

    for i in elementos:
        model += pulp.lpSum([x[j] for j in conjuntos if df.loc[i, j] > 0]) >= 1, f'E_{i}'

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
def resolver_min_cost_flow(df_arcos, origem, destino):
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
    return {
        "status": pulp.LpStatus[model.status],
        "arcos_usados": {a: pulp.value(x[a]) for a in arcos if pulp.value(x[a])>1e-5},
        "custo_total": pulp.value(model.objective),
        "tempo_solucao": solve_time
    }
###############################
def resolver_caminho_minimo(df_arcos, origem, destino):
    return resolver_min_cost_flow(df_arcos, origem, destino)
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
    return {
        "status": pulp.LpStatus[model.status],
        "fluxo_maximo": pulp.value(model.objective),
        "arcos_usados": {a: pulp.value(x[a]) for a in arcos if pulp.value(x[a])>1e-5},
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
        for inicio in range(len(dias)-dias_consec+1):
            # Não pode trabalhar mais do que o bloco permitido
            model += pulp.lpSum([x[(n,dias[k])] for k in range(inicio,inicio+dias_consec+1)]) <= dias_consec
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
def app():
    st.header("Programação Linear Inteira (PLI)")
    st.markdown("""
    A Programação Linear Inteira (PLI) resolve problemas exatos de otimização combinatória. 
    Escolha abaixo qual problema deseja modelar e resolvê-lo.
    """)
    problema = st.selectbox(
        "Selecione o Problema de PLI a resolver:",
        [
            "Modelo Genérico",
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
                    st.metric("Valor objetivo", f"{resultados['valor_objetivo']:.6f}")
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
        """)
        uploaded_file = st.file_uploader("Carregar matriz de distâncias (CSV)", type=["csv", "txt"])
        if uploaded_file is not None:
            df = load_csv_file(uploaded_file)
            if df is not None:
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
                            st.write(" → ".join(resultados["rota"]))
                            st.metric("Tempo de resolução", f"{resultados['tempo_solucao']:.4f} segundos")

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
        CSV: `de,para,custo,capacidade`. Informe origem e destino.
        """)
        up = st.file_uploader("Arquivo de arcos", type=["csv","txt"])
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
        CSV: `de,para,capacidade`. Informe origem e destino.
        """)
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
                    st.write("Escala (enfermeira, dia):", res["escala"])
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
            df1,df2,df3 = load_csv_file(up1), load_csv_file(up2), load_csv_file(up3)
            if st.button("Resolver Plantio"):
                with st.spinner("Resolvendo..."):
                    res = resolver_crop_planning(df1, df2, df3)
                    st.success(f"Status: {res['status']}")
                    st.write("Plano de plantio (fazenda, cultura):", res["plantio"])
                    st.metric("Lucro total", f"{res['lucro_total']:.2f}")
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



