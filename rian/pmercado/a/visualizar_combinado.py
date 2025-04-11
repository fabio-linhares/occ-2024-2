import os
import re
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from datetime import datetime
import time
from pathlib import Path
import warnings

# Configurações globais
warnings.filterwarnings("ignore")
plt.style.use('ggplot')  # Estilo mais profissional
CORES = plt.cm.viridis(np.linspace(0, 1, 10))  # Paleta de cores mais atraente
DIRETORIO_SAIDA = 'graficos'
os.makedirs(DIRETORIO_SAIDA, exist_ok=True)

class ParserAvancado:
    """Parser avançado para extrair dados de múltiplas fontes."""
    
    def __init__(self, diretorio):
        self.diretorio = diretorio
        self.dados_consolidados = []
        self.dados_instancias = []
        self.df_historico = None
        self.df_consolidados = None
        self.df_instancias = None
    
    def carregar_dados_historico(self):
        """Carrega dados do arquivo de histórico CSV."""
        historico_path = os.path.join(self.diretorio, 'historico_desempenho.csv')
        if os.path.exists(historico_path):
            try:
                self.df_historico = pd.read_csv(historico_path)
                # Corrigir formatos de colunas
                self.df_historico['Data'] = pd.to_datetime(self.df_historico['Data'], format='%d-%m-%Y %H:%M:%S')
                print(f"Carregados {len(self.df_historico)} registros do histórico de desempenho.")
                return True
            except Exception as e:
                print(f"Erro ao carregar histórico: {e}")
        else:
            print("Arquivo de histórico não encontrado.")
        return False
    
    def listar_relatorios(self):
        """Lista todos os arquivos de relatório no diretório."""
        relatorios = []
        for arquivo in os.listdir(self.diretorio):
            if arquivo.startswith("relatorio_") and arquivo.endswith(".txt"):
                relatorios.append(os.path.join(self.diretorio, arquivo))
        return sorted(relatorios)
    
    def processar_relatorio(self, arquivo_path):
        """Processa um único arquivo de relatório e extrai seus dados."""
        print(f"Processando relatório: {os.path.basename(arquivo_path)}")
        try:
            with open(arquivo_path, 'r') as f:
                conteudo = f.read()
            
            # Extrair data e hora da execução
            data_match = re.search(r'RELATÓRIO DE DESEMPENHO - ([\d\-]+ [\d:]+)', conteudo)
            if data_match:
                data_execucao = data_match.group(1)
                timestamp = datetime.strptime(data_execucao, '%d-%m-%Y %H:%M:%S')
            else:
                timestamp = datetime.fromtimestamp(os.path.getmtime(arquivo_path))
                data_execucao = timestamp.strftime('%d-%m-%Y %H:%M:%S')
            
            # Extrair dados das instâncias usando regex aprimorada
            instancias_dados = []
            
            # Padrão melhorado que lida com o formato específico dos relatórios
            detalhes_pattern = r'instance_(\d+)\.txt(\d+)\s+(\d+)\s+(\d+)\s+([\d\.]+)\s+(\d+)'
            
            for linha in conteudo.split('\n'):
                match = re.match(detalhes_pattern, linha.strip())
                if match:
                    inst_num = match.group(1)
                    pedidos = int(match.group(2))
                    corredores = int(match.group(3))
                    itens = int(match.group(4))
                    razao = float(match.group(5))
                    tempo = int(match.group(6))
                    
                    instancia_info = {
                        'timestamp': timestamp,
                        'data_execucao': data_execucao,
                        'instancia': f'instance_{inst_num}.txt',
                        'pedidos': pedidos,
                        'corredores': corredores,
                        'itens': itens,
                        'razao': razao,
                        'tempo': tempo,
                        'arquivo': os.path.basename(arquivo_path)
                    }
                    instancias_dados.append(instancia_info)
            
            # Se encontramos instâncias, adicionar aos dados
            if instancias_dados:
                self.dados_instancias.extend(instancias_dados)
                
                # Calcular estatísticas consolidadas a partir das instâncias
                razoes = [inst['razao'] for inst in instancias_dados]
                tempos = [inst['tempo'] for inst in instancias_dados]
                pedidos = [inst['pedidos'] for inst in instancias_dados]
                corredores = [inst['corredores'] for inst in instancias_dados]
                itens = [inst['itens'] for inst in instancias_dados]
                
                dados_consolidados = {
                    'timestamp': timestamp,
                    'data_execucao': data_execucao,
                    'razao_min': min(razoes) if razoes else 0,
                    'razao_max': max(razoes) if razoes else 0,
                    'razao_media': sum(razoes)/len(razoes) if razoes else 0,
                    'tempo_min': min(tempos) if tempos else 0,
                    'tempo_max': max(tempos) if tempos else 0,
                    'tempo_medio': sum(tempos)/len(tempos) if tempos else 0,
                    'pedidos_min': min(pedidos) if pedidos else 0,
                    'pedidos_max': max(pedidos) if pedidos else 0,
                    'pedidos_medio': sum(pedidos)/len(pedidos) if pedidos else 0,
                    'corredores_min': min(corredores) if corredores else 0,
                    'corredores_max': max(corredores) if corredores else 0,
                    'corredores_medio': sum(corredores)/len(corredores) if corredores else 0,
                    'itens_medio': sum(itens)/len(itens) if itens else 0,
                    'num_instancias': len(instancias_dados),
                    'arquivo': os.path.basename(arquivo_path)
                }
                
                self.dados_consolidados.append(dados_consolidados)
                print(f"  -> Extraídos dados de {len(instancias_dados)} instâncias.")
                return True
            else:
                print(f"  -> Nenhuma instância encontrada no relatório.")
                return False
                
        except Exception as e:
            print(f"Erro ao processar relatório {arquivo_path}: {e}")
            return False
    
    def processar_todos_relatorios(self):
        """Processa todos os relatórios e cria DataFrames."""
        relatorios = self.listar_relatorios()
        print(f"Encontrados {len(relatorios)} relatórios para processar.")
        
        self.dados_consolidados = []
        self.dados_instancias = []
        
        for relatorio in relatorios:
            self.processar_relatorio(relatorio)
        
        # Converter para DataFrames se tivermos dados
        if self.dados_consolidados:
            self.df_consolidados = pd.DataFrame(self.dados_consolidados)
            self.df_consolidados['timestamp'] = pd.to_datetime(self.df_consolidados['timestamp'])
            self.df_consolidados.sort_values('timestamp', inplace=True)
            print(f"Criado DataFrame consolidado com {len(self.df_consolidados)} execuções.")
        
        if self.dados_instancias:
            self.df_instancias = pd.DataFrame(self.dados_instancias)
            self.df_instancias['timestamp'] = pd.to_datetime(self.df_instancias['timestamp'])
            self.df_instancias.sort_values(['instancia', 'timestamp'], inplace=True)
            print(f"Criado DataFrame de instâncias com {len(self.df_instancias)} registros.")
        
        return self.df_consolidados, self.df_instancias
    
    def combinar_dados(self):
        """Combina dados do histórico com dados dos relatórios."""
        # Carregar histórico se ainda não fez
        if self.df_historico is None:
            self.carregar_dados_historico()
        
        # Processar relatórios se ainda não fez
        if self.df_consolidados is None or self.df_instancias is None:
            self.processar_todos_relatorios()
        
        # Se temos dados do histórico, podemos enriquecer nossos dados de instâncias
        if self.df_historico is not None and self.df_instancias is not None:
            # Preparar histórico para merge (converter formato de data para ser compatível)
            df_hist = self.df_historico.copy()
            df_hist['data_execucao'] = df_hist['Data'].dt.strftime('%d-%m-%Y %H:%M:%S')
            
            # Mesclar baseado no nome da instância e data de execução
            df_combinado = pd.merge(
                self.df_instancias, 
                df_hist[['data_execucao', 'Instância', 'Algoritmo', 'TempoTotal(ms)']],
                left_on=['data_execucao', 'instancia'],
                right_on=['data_execucao', 'Instância'],
                how='left'
            )
            
            # Substituir DataFrame de instâncias pelo mesclado
            if not df_combinado.empty:
                self.df_instancias = df_combinado
                print(f"Dados combinados com sucesso: {len(self.df_instancias)} registros enriquecidos.")
        
        return self.df_consolidados, self.df_instancias


class VisualizadorAvancado:
    """Classe para criar visualizações avançadas dos dados."""
    
    def __init__(self, diretorio_saida=DIRETORIO_SAIDA):
        self.diretorio_saida = diretorio_saida
        os.makedirs(self.diretorio_saida, exist_ok=True)
    
    def salvar_figura(self, fig, nome_arquivo, dpi=300):
        """Salva figura com configuração de alta qualidade."""
        caminho_completo = os.path.join(self.diretorio_saida, nome_arquivo)
        fig.savefig(caminho_completo, dpi=dpi, bbox_inches='tight')
        plt.close(fig)
        print(f"Gráfico salvo: {nome_arquivo}")
    
    def plot_evolucao_razao(self, df_instancias, top_n=5):
        """Gráfico mostrando a evolução da razão itens/corredores para as top instâncias."""
        if df_instancias is None or df_instancias.empty:
            return
        
        # Selecionar as top-n instâncias com maior variação na razão
        variacao = df_instancias.groupby('instancia')['razao'].agg(['mean', 'std'])
        variacao = variacao.sort_values('std', ascending=False)
        top_instancias = variacao.head(top_n).index.tolist()
        
        # Filtrar apenas essas instâncias
        df_plot = df_instancias[df_instancias['instancia'].isin(top_instancias)]
        
        # Criar figura
        fig, ax = plt.subplots(figsize=(12, 8))
        
        # Plotar linhas para cada instância
        sns.lineplot(
            data=df_plot, 
            x='timestamp', 
            y='razao', 
            hue='instancia',
            marker='o',
            linewidth=2.5,
            ax=ax
        )
        
        # Ajustar aparência
        ax.set_title('Evolução da Razão Itens/Corredores', fontsize=16)
        ax.set_xlabel('Data de Execução', fontsize=12)
        ax.set_ylabel('Razão Itens/Corredores', fontsize=12)
        plt.xticks(rotation=45)
        plt.grid(True, alpha=0.3)
        plt.legend(title='Instância', bbox_to_anchor=(1.05, 1), loc='upper left')
        
        # Adicionar anotações de melhoria
        for inst in top_instancias:
            df_inst = df_plot[df_plot['instancia'] == inst]
            if len(df_inst) > 1:
                primeiro = df_inst.iloc[0]['razao']
                ultimo = df_inst.iloc[-1]['razao']
                mudanca = (ultimo - primeiro) / primeiro * 100
                x = df_inst.iloc[-1]['timestamp']
                y = ultimo
                plt.annotate(
                    f"{mudanca:.1f}%", 
                    xy=(x, y),
                    xytext=(5, 0), 
                    textcoords='offset points',
                    ha='left', va='center',
                    fontweight='bold'
                )
        
        plt.tight_layout()
        self.salvar_figura(fig, 'evolucao_razao_avancado.png')
    
    def plot_comparativo_razoes(self, df_consolidados):
        """Gráfico comparativo das razões médias entre execuções."""
        if df_consolidados is None or df_consolidados.empty:
            return
        
        # Ordenar por razão média
        df_plot = df_consolidados.sort_values('razao_media', ascending=False)
        
        # Criar figura
        fig, ax = plt.subplots(figsize=(10, 6))
        
        # Plotar barras
        bars = ax.bar(
            df_plot['data_execucao'], 
            df_plot['razao_media'],
            color=plt.cm.viridis(np.linspace(0, 0.8, len(df_plot)))
        )
        
        # Adicionar valores nas barras
        for bar in bars:
            height = bar.get_height()
            ax.text(
                bar.get_x() + bar.get_width()/2., 
                height + 0.1,
                f'{height:.2f}', 
                ha='center', va='bottom',
                fontweight='bold'
            )
        
        # Ajustar aparência
        plt.title('Comparação da Razão Média Itens/Corredores por Execução', fontsize=14)
        plt.xlabel('Data de Execução', fontsize=12)
        plt.ylabel('Razão Média', fontsize=12)
        plt.xticks(rotation=45, ha='right')
        plt.grid(axis='y', alpha=0.3)
        
        # Adicionar linha de tendência
        if len(df_plot) > 1:
            x = range(len(df_plot))
            z = np.polyfit(x, df_plot['razao_media'], 1)
            p = np.poly1d(z)
            ax.plot(df_plot['data_execucao'], p(x), "r--", alpha=0.8, linewidth=2)
            
            # Adicionar texto indicando a tendência
            if z[0] > 0:
                trend_text = f"Tendência: +{z[0]:.4f} por execução"
            else:
                trend_text = f"Tendência: {z[0]:.4f} por execução"
            
            plt.figtext(0.01, 0.01, trend_text, ha="left", fontsize=10, 
                       bbox={"facecolor":"white", "alpha":0.8, "pad":5})
        
        plt.tight_layout()
        self.salvar_figura(fig, 'comparativo_razoes.png')
    
    def plot_comparativo_metricas(self, df_consolidados):
        """Gráfico comparativo das principais métricas entre execuções."""
        if df_consolidados is None or df_consolidados.empty:
            return
        
        # Preparar dados normalizados para comparação justa
        df_norm = df_consolidados.copy()
        
        # Normalizar as métricas para escala 0-1
        metricas = ['razao_media', 'tempo_medio', 'pedidos_medio', 'corredores_medio']
        for metrica in metricas:
            if metrica in df_norm.columns:
                min_val = df_norm[metrica].min()
                max_val = df_norm[metrica].max()
                if max_val > min_val:
                    # Para tempo e corredores, menor é melhor, então invertemos
                    if metrica in ['tempo_medio', 'corredores_medio']:
                        df_norm[f'{metrica}_norm'] = 1 - (df_norm[metrica] - min_val) / (max_val - min_val)
                    else:
                        df_norm[f'{metrica}_norm'] = (df_norm[metrica] - min_val) / (max_val - min_val)
                else:
                    df_norm[f'{metrica}_norm'] = 0.5  # Valor padrão se não há variação
        
        # Ordenar por data
        df_norm = df_norm.sort_values('timestamp')
        
        # Criar figura com múltiplos gráficos
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        axes = axes.flatten()
        
        metrica_nomes = {
            'razao_media': 'Razão Média',
            'tempo_medio': 'Tempo Médio (ms)',
            'pedidos_medio': 'Pedidos Médios',
            'corredores_medio': 'Corredores Médios'
        }
        
        metrica_cores = {
            'razao_media': 'green',
            'tempo_medio': 'red',
            'pedidos_medio': 'blue',
            'corredores_medio': 'purple'
        }
        
        # Plotar cada métrica
        for i, metrica in enumerate(metricas):
            ax = axes[i]
            
            # Plotar linha principal
            ax.plot(
                df_norm['data_execucao'], 
                df_norm[metrica], 
                'o-', 
                color=metrica_cores[metrica],
                linewidth=2.5,
                markersize=8
            )
            
            # Adicionar linha pontilhada para normalizada
            if f'{metrica}_norm' in df_norm.columns:
                ax_twin = ax.twinx()
                ax_twin.plot(
                    df_norm['data_execucao'], 
                    df_norm[f'{metrica}_norm'], 
                    's--', 
                    color='gray',
                    alpha=0.6,
                    linewidth=1.5,
                    markersize=6
                )
                ax_twin.set_ylabel('Valor Normalizado (0-1)', fontsize=10)
                ax_twin.set_ylim(-0.05, 1.05)
                ax_twin.grid(False)
            
            # Destacar melhor valor
            if metrica in ['tempo_medio', 'corredores_medio']:
                best_idx = df_norm[metrica].idxmin()
                best_text = "Menor é melhor"
            else:
                best_idx = df_norm[metrica].idxmax()
                best_text = "Maior é melhor"
                
            best_x = df_norm.loc[best_idx, 'data_execucao']
            best_y = df_norm.loc[best_idx, metrica]
            
            ax.scatter([best_x], [best_y], color='gold', s=120, zorder=5, edgecolor='black')
            
            # Configurar aparência
            ax.set_title(metrica_nomes[metrica], fontsize=14)
            ax.set_xlabel('Data de Execução', fontsize=10)
            ax.set_ylabel(metrica_nomes[metrica], fontsize=10)
            ax.tick_params(axis='x', rotation=45)
            ax.grid(True, alpha=0.3)
            
            # Adicionar texto indicativo
            ax.text(0.02, 0.02, best_text, transform=ax.transAxes, fontsize=9,
                   bbox={"facecolor":"white", "alpha":0.8, "pad":3})
        
        plt.suptitle('Evolução das Métricas de Desempenho', fontsize=16, y=1.02)
        plt.tight_layout()
        self.salvar_figura(fig, 'evolucao_metricas.png')
    
    def plot_distribuicao_razoes(self, df_instancias):
        """Gráfico mostrando a distribuição das razões por instância."""
        if df_instancias is None or df_instancias.empty:
            return
        
        # Calcular estatísticas
        stats = df_instancias.groupby('instancia')['razao'].agg(['count', 'mean', 'std', 'min', 'max'])
        stats = stats.sort_values('mean', ascending=False)
        
        # Filtrar instâncias com pelo menos 2 execuções
        stats = stats[stats['count'] >= 2]
        
        if stats.empty:
            return
        
        # Selecionar top 10 instâncias por média
        stats = stats.head(10)
        
        # Criar figura
        fig, ax = plt.subplots(figsize=(12, 6))
        
        # Preparar dados para barras de erro
        x = np.arange(len(stats.index))
        
        # Plotar barras com erro
        bars = ax.bar(x, stats['mean'], yerr=stats['std'], 
                     capsize=5, color=plt.cm.viridis(np.linspace(0, 0.8, len(stats))),
                     alpha=0.8)
        
        # Adicionar scatter points para min/max
        for i, (idx, row) in enumerate(stats.iterrows()):
            ax.scatter([i, i], [row['min'], row['max']], color='red', marker='_', s=200, zorder=10)
        
        # Configurar aparência
        ax.set_title('Distribuição da Razão Itens/Corredores por Instância', fontsize=16)
        ax.set_xlabel('Instância', fontsize=12)
        ax.set_ylabel('Razão Média (com desvio padrão)', fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(stats.index, rotation=45, ha='right')
        ax.grid(axis='y', alpha=0.3)
        
        # Adicionar valores acima das barras
        for i, bar in enumerate(bars):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height + 0.1,
                   f'{height:.2f}', ha='center', va='bottom', fontweight='bold')
        
        plt.tight_layout()
        self.salvar_figura(fig, 'distribuicao_razoes_avancado.png')
    
    def plot_dashboard_desempenho(self, df_consolidados, df_instancias):
        """Cria um dashboard completo do desempenho das execuções."""
        if df_consolidados is None or df_consolidados.empty:
            return
        
        # Calcular pontuação composta (maior é melhor)
        df_score = df_consolidados.copy()
        
        # Normalizar métricas importantes (0-1)
        metrics = {
            'razao_media': {'weight': 0.4, 'higher_better': True},
            'tempo_medio': {'weight': 0.3, 'higher_better': False},
            'pedidos_medio': {'weight': 0.2, 'higher_better': True},
            'corredores_medio': {'weight': 0.1, 'higher_better': False}
        }
        
        # Criar score
        score = pd.Series(0.0, index=df_score.index)
        
        for metric, params in metrics.items():
            if metric in df_score.columns:
                min_val = df_score[metric].min()
                max_val = df_score[metric].max()
                
                if max_val > min_val:  # Evitar divisão por zero
                    if params['higher_better']:
                        normalized = (df_score[metric] - min_val) / (max_val - min_val)
                    else:
                        normalized = 1 - (df_score[metric] - min_val) / (max_val - min_val)
                        
                    score += normalized * params['weight']
        
        # Escalar para 0-10
        df_score['score'] = score * 10 / sum(v['weight'] for v in metrics.values())
        
        # Ordenar por score
        df_score = df_score.sort_values('score', ascending=False)
        
        # Criar figura do dashboard
        fig = plt.figure(figsize=(16, 12))
        
        # Layout da figura
        gs = fig.add_gridspec(3, 3)
        
        # 1. Score geral (maior gráfico)
        ax1 = fig.add_subplot(gs[0, :])
        bars = ax1.bar(
            df_score['data_execucao'], 
            df_score['score'],
            color=plt.cm.viridis(np.linspace(0, 0.8, len(df_score)))
        )
        
        # Adicionar valores nas barras
        for bar in bars:
            height = bar.get_height()
            ax1.text(
                bar.get_x() + bar.get_width()/2., 
                height + 0.1,
                f'{height:.2f}', 
                ha='center', va='bottom',
                fontweight='bold'
            )
            
        ax1.set_title('Pontuação Geral das Execuções (0-10)', fontsize=14)
        ax1.set_xlabel('Data de Execução', fontsize=12)
        ax1.set_ylabel('Pontuação', fontsize=12)
        ax1.set_ylim(0, 10.5)
        ax1.grid(axis='y', alpha=0.3)
        ax1.tick_params(axis='x', rotation=45)
        
        # 2. Evolução da razão média
        ax2 = fig.add_subplot(gs[1, 0])
        ax2.plot(df_consolidados['data_execucao'], df_consolidados['razao_media'], 'o-', 
                color='green', linewidth=2.5)
        ax2.set_title('Razão Média', fontsize=12)
        ax2.tick_params(axis='x', rotation=45)
        ax2.grid(True, alpha=0.3)
        
        # 3. Evolução do tempo médio
        ax3 = fig.add_subplot(gs[1, 1])
        ax3.plot(df_consolidados['data_execucao'], df_consolidados['tempo_medio'], 'o-', 
                color='red', linewidth=2.5)
        ax3.set_title('Tempo Médio (ms)', fontsize=12)
        ax3.tick_params(axis='x', rotation=45)
        ax3.grid(True, alpha=0.3)
        
        # 4. Evolução dos pedidos médios
        ax4 = fig.add_subplot(gs[1, 2])
        ax4.plot(df_consolidados['data_execucao'], df_consolidados['pedidos_medio'], 'o-', 
                color='blue', linewidth=2.5)
        ax4.set_title('Pedidos Médios', fontsize=12)
        ax4.tick_params(axis='x', rotation=45)
        ax4.grid(True, alpha=0.3)
        
        # 5. Relação Razão x Corredores
        ax5 = fig.add_subplot(gs[2, 0])
        if df_instancias is not None and not df_instancias.empty:
            sns.scatterplot(
                data=df_instancias, 
                x='corredores', 
                y='razao',
                hue='data_execucao',
                palette='viridis',
                s=80,
                alpha=0.7,
                ax=ax5
            )
            ax5.set_title('Razão vs Corredores', fontsize=12)
            ax5.legend().remove()  # Remover legenda para economizar espaço
        
        # 6. Melhores razões por instância
        ax6 = fig.add_subplot(gs[2, 1:])
        if df_instancias is not None and not df_instancias.empty:
            # Encontrar máxima razão para cada instância
            best_razoes = df_instancias.loc[df_instancias.groupby('instancia')['razao'].idxmax()]
            best_razoes = best_razoes.sort_values('razao', ascending=False).head(8)
            
            sns.barplot(
                data=best_razoes,
                x='instancia',
                y='razao',
                palette='viridis',
                ax=ax6
            )
            ax6.set_title('Melhores Razões por Instância', fontsize=12)
            ax6.tick_params(axis='x', rotation=45)
        
        # Título geral e ajustes
        plt.suptitle('Dashboard de Desempenho das Execuções', fontsize=18, y=0.98)
        plt.figtext(0.5, 0.01, 
                   "Pontuação composta: 40% Razão + 30% Tempo + 20% Pedidos + 10% Corredores", 
                   ha="center", fontsize=10, 
                   bbox={"facecolor":"white", "alpha":0.8, "pad":5})
        
        plt.tight_layout()
        self.salvar_figura(fig, 'dashboard_desempenho.png')
        
        # Também gerar um relatório textual
        self._gerar_relatorio_textual(df_score)
    
    def _gerar_relatorio_textual(self, df_score):
        """Gera um relatório textual com informações detalhadas."""
        if df_score.empty:
            return
            
        # Ordenar por pontuação
        df = df_score.sort_values('score', ascending=False)
        
        # Preparar texto do relatório
        report = "RELATÓRIO DE DESEMPENHO DAS EXECUÇÕES\n"
        report += "====================================\n\n"
        
        # Adicionar informações sobre a melhor execução
        best_idx = df['score'].idxmax()
        best_exec = df.loc[best_idx]
        
        report += f"MELHOR EXECUÇÃO: {best_exec['data_execucao']}\n"
        report += f"Pontuação: {best_exec['score']:.2f}/10\n\n"
        report += "Métricas da melhor execução:\n"
        report += f"- Razão Média: {best_exec['razao_media']:.2f}\n"
        report += f"- Tempo Médio: {best_exec['tempo_medio']:.2f} ms\n"
        report += f"- Pedidos Médios: {best_exec['pedidos_medio']:.2f}\n"
        report += f"- Corredores Médios: {best_exec['corredores_medio']:.2f}\n\n"
        
        # Adicionar tabela de classificação
        report += "CLASSIFICAÇÃO GERAL:\n"
        report += "-----------------\n"
        for i, (idx, row) in enumerate(df.iterrows()):
            report += f"{i+1}. {row['data_execucao']} - {row['score']:.2f}/10\n"
        
        # Melhores em cada métrica
        report += "\nMELHORES POR MÉTRICA:\n"
        report += "-----------------\n"
        
        best_razao = df.loc[df['razao_media'].idxmax()]
        best_tempo = df.loc[df['tempo_medio'].idxmin()]
        best_pedidos = df.loc[df['pedidos_medio'].idxmax()]
        best_corredores = df.loc[df['corredores_medio'].idxmin()]
        
        report += f"Melhor Razão: {best_razao['data_execucao']} - {best_razao['razao_media']:.2f}\n"
        report += f"Melhor Tempo: {best_tempo['data_execucao']} - {best_tempo['tempo_medio']:.2f} ms\n"
        report += f"Melhor Pedidos: {best_pedidos['data_execucao']} - {best_pedidos['pedidos_medio']:.2f}\n"
        report += f"Melhor Corredores: {best_corredores['data_execucao']} - {best_corredores['corredores_medio']:.2f}\n"
        
        # Salvar relatório
        with open(os.path.join(self.diretorio_saida, 'relatorio_desempenho.txt'), 'w') as f:
            f.write(report)
        
        print(f"Relatório textual gerado em: {os.path.join(self.diretorio_saida, 'relatorio_desempenho.txt')}")
    
    def gerar_todos_graficos(self, df_consolidados, df_instancias):
        """Gera todos os gráficos disponíveis."""
        print("Gerando conjunto completo de visualizações...")
        
        if df_consolidados is not None and not df_consolidados.empty:
            # Gráficos baseados em dados consolidados
            self.plot_comparativo_razoes(df_consolidados)
            self.plot_comparativo_metricas(df_consolidados)
            
            # Dashboard completo
            self.plot_dashboard_desempenho(df_consolidados, df_instancias)
        
        if df_instancias is not None and not df_instancias.empty:
            # Gráficos baseados em dados de instâncias
            self.plot_evolucao_razao(df_instancias)
            self.plot_distribuicao_razoes(df_instancias)
        
        print("Visualizações geradas com sucesso!")


def main():
    # Diretório atual
    diretorio = os.path.dirname(os.path.abspath(__file__))
    
    # Inicializar parser e visualizador
    parser = ParserAvancado(diretorio)
    visualizador = VisualizadorAvancado()
    
    # Processar e combinar todos os dados
    parser.carregar_dados_historico()
    parser.processar_todos_relatorios()
    df_consolidados, df_instancias = parser.combinar_dados()
    
    # Gerar todos os gráficos
    visualizador.gerar_todos_graficos(df_consolidados, df_instancias)
    
    print(f"Processamento concluído. Gráficos salvos em: {visualizador.diretorio_saida}")


if __name__ == "__main__":
    main()