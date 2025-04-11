import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import sys
import os
from datetime import datetime

def main():
    # Verificar se o arquivo histórico existe
    historico_file = 'historico_desempenho.csv'
    if not os.path.exists(historico_file):
        print(f"Erro: arquivo {historico_file} não encontrado.")
        return
    
    # Carregar dados
    df = pd.read_csv(historico_file)
    
    # Converter coluna Data para datetime
    df['Data'] = pd.to_datetime(df['Data'])
    
    # Criar diretório para gráficos se não existir
    os.makedirs('graficos', exist_ok=True)
    
    # Gráfico 1: Evolução da razão itens/corredor por instância
    plt.figure(figsize=(12, 8))
    sns.lineplot(data=df, x='Data', y='Razão', hue='Instância')
    plt.title('Evolução da Razão Itens/Corredor por Instância')
    plt.xlabel('Data de Execução')
    plt.ylabel('Razão Itens/Corredor')
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig('graficos/evolucao_razao.png')
    
    # Gráfico 2: Distribuição da razão por instância (boxplot)
    plt.figure(figsize=(12, 8))
    sns.boxplot(data=df, x='Instância', y='Razão')
    plt.title('Distribuição da Razão Itens/Corredor por Instância')
    plt.xlabel('Instância')
    plt.ylabel('Razão')
    plt.xticks(rotation=90)
    plt.tight_layout()
    plt.savefig('graficos/distribuicao_razao.png')
    
    # Gráfico 3: Tempo de execução por instância
    plt.figure(figsize=(12, 8))
    sns.barplot(data=df, x='Instância', y='Tempo(ms)')
    plt.title('Tempo de Execução por Instância')
    plt.xlabel('Instância')
    plt.ylabel('Tempo (ms)')
    plt.xticks(rotation=90)
    plt.tight_layout()
    plt.savefig('graficos/tempo_execucao.png')
    
    # Gráfico 4: Relação entre número de pedidos e razão
    plt.figure(figsize=(10, 8))
    sns.scatterplot(data=df, x='Pedidos', y='Razão', hue='Instância', size='Corredores')
    plt.title('Relação entre Pedidos Atendidos e Razão')
    plt.xlabel('Número de Pedidos')
    plt.ylabel('Razão Itens/Corredor')
    plt.tight_layout()
    plt.savefig('graficos/relacao_pedidos_razao.png')
    
    print("Gráficos gerados com sucesso no diretório 'graficos/'")

if __name__ == "__main__":
    main()