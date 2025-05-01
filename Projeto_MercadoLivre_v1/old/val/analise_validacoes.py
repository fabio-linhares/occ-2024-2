
#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
from datetime import datetime

# Configurações
plt.style.use('ggplot')
DIRETORIO_GRAFICOS = 'graficos'
ARQUIVO_CSV = 'historico_validacoes.csv'

# Criar diretório para gráficos se não existir
os.makedirs(DIRETORIO_GRAFICOS, exist_ok=True)

# Carregar dados
print(f"Carregando dados de: {ARQUIVO_CSV}")
df = pd.read_csv(ARQUIVO_CSV)

# Converter timestamp para datetime
df['Timestamp'] = pd.to_datetime(df['Timestamp'])

# Ordenar por timestamp
df = df.sort_values('Timestamp')

# Adicionar coluna de execução (número sequencial por data)
df['Execucao'] = df.groupby(df['Timestamp'].dt.date)['Timestamp'].rank()

# Adicionar coluna de data formatada para facilitar plotagem
df['Data'] = df['Timestamp'].dt.strftime('%Y-%m-%d')

# Adicionar coluna de solução válida
df['Solucao_Valida'] = (df['LB_OK'] & df['UB_OK'] & df['Disponibilidade_OK'] & df['IDs_Validos']).astype(int)

# 1. Gráfico de evolução da razão por instância
def plot_evolucao_razao():
    plt.figure(figsize=(14, 8))
    
    # Selecionar top 10 instâncias com mais execuções
    top_instancias = df['Instancia'].value_counts().head(10).index.tolist()
    df_plot = df[df['Instancia'].isin(top_instancias)]
    
    sns.lineplot(data=df_plot, x='Timestamp', y='Razao', hue='Instancia', marker='o')
    
    plt.title('Evolução da Razão por Instância ao Longo do Tempo', fontsize=16)
    plt.xlabel('Data da Validação', fontsize=12)
    plt.ylabel('Razão (Itens/Corredores)', fontsize=12)
    plt.xticks(rotation=45)
    plt.grid(True, alpha=0.3)
    plt.legend(title='Instância', bbox_to_anchor=(1.05, 1), loc='upper left')
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'evolucao_razao.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: evolucao_razao.png")

# 2. Gráfico de barras comparando razão média por data
def plot_razao_media_por_data():
    plt.figure(figsize=(12, 6))
    
    # Calcular média da razão por data
    df_por_data = df.groupby('Data')['Razao'].mean().reset_index()
    
    # Plot
    ax = sns.barplot(data=df_por_data, x='Data', y='Razao')
    
    # Adicionar rótulos nas barras
    for i, bar in enumerate(ax.patches):
        ax.text(
            bar.get_x() + bar.get_width()/2., 
            bar.get_height() + 0.1, 
            f'{bar.get_height():.2f}', 
            ha='center', va='bottom'
        )
    
    plt.title('Razão Média por Data de Validação', fontsize=16)
    plt.xlabel('Data', fontsize=12)
    plt.ylabel('Razão Média', fontsize=12)
    plt.xticks(rotation=45)
    plt.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'razao_media_por_data.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: razao_media_por_data.png")

# 3. Gráfico de validação das restrições ao longo do tempo
def plot_restricoes_tempo():
    plt.figure(figsize=(14, 8))
    
    # Preparar dados para plotagem
    df_plot = df.groupby('Data')[['LB_OK', 'UB_OK', 'Disponibilidade_OK', 'IDs_Validos', 'Solucao_Valida']].mean()
    df_plot = df_plot * 100  # Converter para porcentagem
    
    # Plot
    ax = df_plot.plot(kind='bar', figsize=(14, 8))
    
    plt.title('Percentual de Conformidade por Restrição ao Longo do Tempo', fontsize=16)
    plt.xlabel('Data', fontsize=12)
    plt.ylabel('Porcentagem de Conformidade (%)', fontsize=12)
    plt.ylim(0, 105)
    plt.grid(axis='y', alpha=0.3)
    plt.xticks(rotation=45)
    plt.legend(title='Restrição', labels=[
        'Limite Inferior', 
        'Limite Superior', 
        'Disponibilidade de Itens', 
        'IDs Válidos',
        'Solução Completamente Válida'
    ])
    
    # Adicionar rótulos de porcentagem
    for i, p in enumerate(ax.patches):
        ax.annotate(
            f'{p.get_height():.1f}%', 
            (p.get_x() + p.get_width()/2., p.get_height() + 1), 
            ha='center', va='bottom'
        )
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'restricoes_tempo.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: restricoes_tempo.png")

# 4. Gráfico de dispersão: Razão vs. Número de Corredores
def plot_razao_vs_corredores():
    plt.figure(figsize=(12, 8))
    
    sns.scatterplot(
        data=df, 
        x='NumCorredores', 
        y='Razao', 
        hue='Instancia',
        size='TotalItens',
        sizes=(50, 300),
        alpha=0.7
    )
    
    plt.title('Relação entre Razão e Número de Corredores Visitados', fontsize=16)
    plt.xlabel('Número de Corredores', fontsize=12)
    plt.ylabel('Razão (Itens/Corredores)', fontsize=12)
    plt.grid(True, alpha=0.3)
    
    # Adicionar linha de tendência
    x = df['NumCorredores']
    y = df['Razao']
    
    if len(x) > 1:
        z = np.polyfit(x, y, 1)
        p = np.poly1d(z)
        plt.plot(x, p(x), "r--", alpha=0.7)
        
        # Adicionar equação da linha de tendência
        plt.text(
            0.05, 0.95, 
            f'y = {z[0]:.4f}x + {z[1]:.4f}', 
            transform=plt.gca().transAxes,
            bbox=dict(facecolor='white', alpha=0.7)
        )
    
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'razao_vs_corredores.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: razao_vs_corredores.png")

# 5. Gráfico de heatmap mostrando a correlação entre as métricas
def plot_correlacao_metricas():
    # Selecionar apenas colunas numéricas
    colunas_numericas = ['Razao', 'TotalItens', 'NumCorredores', 'LB', 'UB', 
                         'LB_OK', 'UB_OK', 'Disponibilidade_OK', 'IDs_Validos', 'Solucao_Valida']
    df_corr = df[colunas_numericas].corr()
    
    plt.figure(figsize=(10, 8))
    sns.heatmap(df_corr, annot=True, cmap='coolwarm', fmt='.2f', linewidths=0.5)
    
    plt.title('Correlação entre Métricas', fontsize=16)
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'correlacao_metricas.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: correlacao_metricas.png")

# 6. Dashboard com múltiplos gráficos
def plot_dashboard():
    fig = plt.figure(figsize=(15, 12))
    fig.suptitle('Dashboard de Análise de Validações', fontsize=18, y=0.98)
    
    # Grid layout
    gs = fig.add_gridspec(3, 3)
    
    # 1. Evolução da razão média ao longo do tempo
    ax1 = fig.add_subplot(gs[0, :])
    df_evolucao = df.groupby('Data')['Razao'].mean().reset_index()
    sns.lineplot(data=df_evolucao, x='Data', y='Razao', marker='o', color='blue', ax=ax1)
    ax1.set_title('Evolução da Razão Média ao Longo do Tempo')
    ax1.set_ylabel('Razão Média')
    ax1.set_xlabel('Data')
    ax1.grid(True, alpha=0.3)
    ax1.tick_params(axis='x', rotation=45)
    
    # 2. Distribuição de razões
    ax2 = fig.add_subplot(gs[1, 0])
    sns.histplot(data=df, x='Razao', kde=True, ax=ax2)
    ax2.set_title('Distribuição das Razões')
    ax2.set_xlabel('Razão')
    ax2.set_ylabel('Frequência')
    
    # 3. Proporção de restrições atendidas
    ax3 = fig.add_subplot(gs[1, 1])
    df_restricoes = pd.DataFrame({
        'Restrição': ['LB', 'UB', 'Disponibilidade', 'IDs', 'Solução Completa'],
        'Conformidade (%)': [
            df['LB_OK'].mean() * 100,
            df['UB_OK'].mean() * 100,
            df['Disponibilidade_OK'].mean() * 100,
            df['IDs_Validos'].mean() * 100,
            df['Solucao_Valida'].mean() * 100
        ]
    })
    sns.barplot(data=df_restricoes, x='Restrição', y='Conformidade (%)', ax=ax3)
    ax3.set_title('Conformidade por Restrição')
    ax3.set_xlabel('')
    ax3.set_ylim(0, 105)
    ax3.tick_params(axis='x', rotation=45)
    
    # Adicionar rótulos nas barras
    for i, p in enumerate(ax3.patches):
        ax3.annotate(
            f'{p.get_height():.1f}%', 
            (p.get_x() + p.get_width()/2., p.get_height() + 1), 
            ha='center', va='bottom'
        )
    
    # 4. Top instâncias por razão média
    ax4 = fig.add_subplot(gs[1, 2])
    df_top = df.groupby('Instancia')['Razao'].mean().reset_index()
    df_top = df_top.sort_values('Razao', ascending=False).head(10)
    sns.barplot(data=df_top, x='Razao', y='Instancia', ax=ax4)
    ax4.set_title('Top 10 Instâncias por Razão Média')
    ax4.set_xlabel('Razão Média')
    ax4.set_ylabel('')
    
    # 5. Dispersão: Itens vs. Corredores
    ax5 = fig.add_subplot(gs[2, :])
    scatter = ax5.scatter(
        df['NumCorredores'], 
        df['TotalItens'], 
        c=df['Razao'], 
        s=df['Razao']*20, 
        alpha=0.7, 
        cmap='viridis'
    )
    ax5.set_title('Relação entre Total de Itens e Número de Corredores')
    ax5.set_xlabel('Número de Corredores')
    ax5.set_ylabel('Total de Itens')
    ax5.grid(True, alpha=0.3)
    
    # Adicionar barra de cores
    cbar = plt.colorbar(scatter, ax=ax5)
    cbar.set_label('Razão')
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'dashboard_completo.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: dashboard_completo.png")

# 7. Relatório resumo
def gerar_relatorio_resumo():
    # Criar arquivo de relatório
    relatorio_path = os.path.join(DIRETORIO_GRAFICOS, 'relatorio_resumo.txt')
    
    with open(relatorio_path, 'w') as f:
        f.write("RELATÓRIO RESUMO DE VALIDAÇÕES\n")
        f.write("==============================\n\n")
        
        # Estatísticas gerais
        f.write(f"Total de validações: {len(df)}\n")
        f.write(f"Período: {df['Timestamp'].min().strftime('%Y-%m-%d')} a {df['Timestamp'].max().strftime('%Y-%m-%d')}\n")
        f.write(f"Instâncias distintas: {df['Instancia'].nunique()}\n\n")
        
        # Conformidade
        f.write("CONFORMIDADE COM RESTRIÇÕES:\n")
        f.write("--------------------------\n")
        f.write(f"Limite Inferior (LB): {df['LB_OK'].mean()*100:.2f}%\n")
        f.write(f"Limite Superior (UB): {df['UB_OK'].mean()*100:.2f}%\n")
        f.write(f"Disponibilidade de Itens: {df['Disponibilidade_OK'].mean()*100:.2f}%\n")
        f.write(f"IDs Válidos: {df['IDs_Validos'].mean()*100:.2f}%\n")
        f.write(f"Soluções Completamente Válidas: {df['Solucao_Valida'].mean()*100:.2f}%\n\n")
        
        # Estatísticas de razão
        f.write("ESTATÍSTICAS DE RAZÃO:\n")
        f.write("---------------------\n")
        f.write(f"Razão média global: {df['Razao'].mean():.4f}\n")
        f.write(f"Razão máxima: {df['Razao'].max():.4f}\n")
        f.write(f"Razão mínima: {df['Razao'].min():.4f}\n")
        f.write(f"Desvio padrão da razão: {df['Razao'].std():.4f}\n\n")
        
        # Melhores instâncias por razão
        f.write("TOP 5 INSTÂNCIAS POR RAZÃO MÉDIA:\n")
        f.write("--------------------------------\n")
        top_instancias = df.groupby('Instancia')['Razao'].mean().sort_values(ascending=False).head(5)
        for i, (instancia, razao) in enumerate(top_instancias.items(), 1):
            f.write(f"{i}. {instancia}: {razao:.4f}\n")
        
        f.write("\n")
        
        # Evolução da solução
        f.write("EVOLUÇÃO DA SOLUÇÃO:\n")
        f.write("-------------------\n")
        
        # Verificar tendência nas últimas validações
        ultimas_data = df.groupby('Data')['Razao'].mean().reset_index()
        if len(ultimas_data) >= 2:
            primeira_razao = ultimas_data.iloc[0]['Razao']
            ultima_razao = ultimas_data.iloc[-1]['Razao']
            variacao = ((ultima_razao - primeira_razao) / primeira_razao) * 100
            
            f.write(f"Razão média na primeira data: {primeira_razao:.4f}\n")
            f.write(f"Razão média na última data: {ultima_razao:.4f}\n")
            f.write(f"Variação percentual: {variacao:.2f}%\n")
            
            if variacao > 0:
                f.write(f"Tendência: MELHORIA de {variacao:.2f}% na razão média\n")
            elif variacao < 0:
                f.write(f"Tendência: PIORA de {abs(variacao):.2f}% na razão média\n")
            else:
                f.write("Tendência: ESTÁVEL (sem variação significativa)\n")
    
    print(f"Relatório resumo gerado: {relatorio_path}")

# Executar todas as funções de visualização
if __name__ == "__main__":
    # Verificar se há dados suficientes
    if len(df) < 2:
        print("Não há dados suficientes para gerar visualizações. Execute mais validações.")
    else:
        # Criar gráficos
        plot_evolucao_razao()
        plot_razao_media_por_data()
        plot_restricoes_tempo()
        plot_razao_vs_corredores()
        plot_correlacao_metricas()
        plot_dashboard()
        gerar_relatorio_resumo()
        
        print(f"\nTodos os gráficos foram salvos no diretório: {DIRETORIO_GRAFICOS}")
        print(f"Total de registros analisados: {len(df)}")
    
