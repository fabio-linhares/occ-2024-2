import os
import re
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import pandas as pd
from datetime import datetime
import glob
import numpy as np
import time
import traceback
import matplotlib.dates as mdates

# Torna o watchdog opcional
WATCHDOG_AVAILABLE = False
try:
    from watchdog.observers import Observer
    from watchdog.events import FileSystemEventHandler
    WATCHDOG_AVAILABLE = True
except ImportError:
    print("Módulo watchdog não encontrado. O monitoramento contínuo de arquivos não estará disponível.")
    print("Para instalá-lo, execute: pip install watchdog")

# Definir os diretórios de log e saída
LOG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'results', 'logs')
OUTPUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'results', 'graphs')

# Função para extrair data do nome do arquivo
def extract_date_from_filename(filename):
    """Extrai a data e hora do nome do arquivo de log."""
    match = re.search(r'validation_log_(\d{6})-(\d{4})\.txt', os.path.basename(filename))
    if match:
        date_str, time_str = match.groups()
        day = date_str[0:2]
        month = date_str[2:4]
        year = date_str[4:6]
        hour = time_str[0:2]
        minute = time_str[2:4]
        try:
            return datetime(int(f"20{year}"), int(month), int(day), int(hour), int(minute))
        except ValueError:
            print(f"Data/hora inválida encontrada no nome do arquivo: {filename}")
            return None
    return None

# Função para parsear arquivo de log
def parse_validation_log(log_file):
    """Extrai os dados de BOV e BOV oficial de cada instância no arquivo de log."""
    instances = {}
    try:
        with open(log_file, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Erro ao ler o arquivo {log_file}: {e}")
        return instances

    instance_blocks = content.split('----------------------------------------')

    for instance_block in instance_blocks:
        instance_block = instance_block.strip()
        if not instance_block:
            continue

        # Extrair ID da instância
        instance_match = re.search(r'Arquivo de entrada:.*?instance_(\d+)\.txt', instance_block, re.IGNORECASE)
        if not instance_match:
            continue
        instance_id = int(instance_match.group(1))

        # Extrair BOV e BOV oficial com padrão mais flexível
        bov_pattern = r'Valor objetivo.*?[:\s]+(\d+\.?\d*)'
        bov_match = re.search(bov_pattern, instance_block, re.IGNORECASE)
        
        official_bov_pattern = r'BOV oficial.*?[:\s]+(\d+\.?\d*)'
        official_bov_match = re.search(official_bov_pattern, instance_block, re.IGNORECASE)
        
        if bov_match and official_bov_match:
            try:
                bov = float(bov_match.group(1))
                bov_official = float(official_bov_match.group(1))
                instances[instance_id] = {
                    'bov': bov,
                    'bov_official': bov_official
                }
            except ValueError:
                print(f"Erro ao converter BOV/BOV Oficial para float no arquivo {log_file}, instância {instance_id}")

    return instances

# Função para atualizar o gráfico
def update_plot():
    """Atualiza o gráfico com os dados de todos os arquivos de log disponíveis."""
    print("\n--- Iniciando atualização do gráfico ---")
    log_files = sorted(glob.glob(os.path.join(LOG_DIR, 'validation_log_*.txt')))

    print(f"Encontrados {len(log_files)} arquivos de log.")
    if not log_files:
        print("Nenhum arquivo de log encontrado para plotar.")
        return

    all_data = []
    for log_file in log_files:
        execution_time = extract_date_from_filename(log_file)
        if not execution_time:
            continue
        instances_data = parse_validation_log(log_file)
        if not instances_data:
            continue
        for instance_id, values in instances_data.items():
            bov = values['bov']
            bov_official = values['bov_official']
            improvement = (bov / bov_official) * 100 if bov_official > 0 else 0
            all_data.append({
                'execution_time': execution_time,
                'instance_id': instance_id,
                'bov': bov,
                'bov_official': bov_official,
                'improvement': improvement
            })

    if not all_data:
        print("Nenhum dado válido extraído dos arquivos de log.")
        return

    print(f"Dados extraídos de {len(all_data)} entradas. Gerando gráficos...")

    try:
        df = pd.DataFrame(all_data)
        df = df.sort_values(['instance_id', 'execution_time'])
    except Exception as e:
        print(f"Erro ao processar dados com Pandas: {e}")
        return

    unique_executions = sorted(df['execution_time'].unique())
    unique_instances = sorted(df['instance_id'].unique())

    official_bov_map = df.groupby('instance_id')['bov_official'].last().to_dict()
    df_official = pd.DataFrame({
        'instance_id': list(official_bov_map.keys()),
        'bov_official': list(official_bov_map.values())
    }).sort_values('instance_id')

    # Criar gráficos
    try:
        fig, ax = plt.subplots(2, 1, figsize=(18, 16), sharex=True)
        colors = plt.cm.viridis(np.linspace(0, 1, len(unique_executions)))

        fig.suptitle('Comparativo de Execuções por Instância - Projeto MercadoLivre v3', 
                    fontsize=18, y=0.99, fontweight='bold')

        ax[0].set_title('Comparativo do BOV Obtido por Execução vs BOV Oficial', 
                       fontsize=14, fontweight='bold')
        ax[1].set_title('Porcentagem do BOV Obtido em Relação ao BOV Oficial por Execução', 
                       fontsize=14, fontweight='bold')

        ax[0].set_ylabel('BOV (Valor Objetivo)')
        ax[0].grid(True, linestyle='--', alpha=0.7)
        ax[0].set_xticks(unique_instances)
        ax[0].tick_params(axis='x', rotation=45)

        ax[1].set_xlabel('ID da Instância')
        ax[1].set_ylabel('BOV / BOV Oficial (%)')
        ax[1].grid(True, linestyle='--', alpha=0.7)
        ax[1].axhline(100, color='red', linestyle='-.', linewidth=1.5, 
                    label='100% (Meta Oficial)', zorder=3)
        ax[1].axhspan(98, 102, color='lightgreen', alpha=0.3, label='Meta')
        ax[1].yaxis.set_major_formatter(mticker.PercentFormatter())
        ax[1].set_xticks(unique_instances)

        ax[0].plot(df_official['instance_id'], df_official['bov_official'],
                marker='D', markersize=10,  # Aumentar tamanho do marcador
                linestyle='--', linewidth=3.0,  # Linha mais grossa
                color='darkred', label='BOV Oficial (Referência)',  # Cor mais forte
                zorder=5)  # Garantir que fique acima das outras linhas

        # Adicionar rótulos com valores nos pontos
        for i, row in df_official.iterrows():
            ax[0].annotate(f"{row['bov_official']:.1f}", 
                          (row['instance_id'], row['bov_official']),
                          textcoords="offset points", 
                          xytext=(0,8), 
                          ha='center',
                          fontweight='bold',
                          bbox=dict(boxstyle="round,pad=0.3", fc="white", ec="darkred", alpha=0.8))

        execution_lines = []
        for i, exec_time in enumerate(unique_executions):
            exec_df = df[df['execution_time'] == exec_time].sort_values('instance_id')
            label_time = exec_time.strftime('%d/%m %H:%M')
            line, = ax[0].plot(exec_df['instance_id'], exec_df['bov'],
                    marker='o', markersize=4,
                    linestyle='-', linewidth=1.0,
                    label=f'Exec: {label_time}',
                    color=colors[i], alpha=0.8,
                    zorder=2)
            execution_lines.append(line)

        handles_exec = execution_lines + [ax[0].get_lines()[0]]
        labels_exec = [line.get_label() for line in execution_lines] + ['BOV Oficial (Referência)']

        handles_0, labels_0 = ax[0].get_legend_handles_labels()
        order = []
        # Adicionar BOV Oficial primeiro
        bov_idx = labels_0.index('BOV Oficial (Referência)')
        order.append(bov_idx)
        # Adicionar Melhor execução depois
        best_idx = [i for i, label in enumerate(labels_0) if 'MELHOR' in label]
        if best_idx:
            order.extend(best_idx)
        # Adicionar execução atual depois
        current_idx = [i for i, label in enumerate(labels_0) if 'ATUAL' in label]
        if current_idx:
            order.extend(current_idx)
        # Adicionar o resto
        other_idx = [i for i in range(len(labels_0)) if i not in order]
        order.extend(other_idx)

        ax[0].legend([handles_0[i] for i in order], [labels_0[i] for i in order], 
                   title='Execuções', loc='center left', bbox_to_anchor=(1.02, 0.5), 
                   fontsize='small')

        execution_lines_perc = []
        for i, exec_time in enumerate(unique_executions):
            exec_df = df[df['execution_time'] == exec_time].sort_values('instance_id')
            label_time = exec_time.strftime('%d/%m %H:%M')
            line, = ax[1].plot(exec_df['instance_id'], exec_df['improvement'],
                    marker='x', markersize=5,
                    linestyle=':', linewidth=1.0,
                    label=f'Exec: {label_time}',
                    color=colors[i], alpha=0.8,
                    zorder=2)
            execution_lines_perc.append(line)

        handles_perc = execution_lines_perc
        labels_perc = [line.get_label() for line in execution_lines_perc]
        line_100 = next((line for line in ax[1].get_lines() if line.get_label() == '100% (Meta Oficial)'), None)
        if line_100:
            handles_perc.append(line_100)
            labels_perc.append('100% (Meta Oficial)')

        ax[1].legend(handles=handles_perc, labels=labels_perc, title='Execuções', 
                    loc='center left', bbox_to_anchor=(1.02, 0.5), fontsize='small')

        # Identificar a execução com o melhor resultado
        best_exec_idx = -1
        best_exec_value = 0

        for i, exec_time in enumerate(unique_executions):
            exec_df = df[df['execution_time'] == exec_time]
            mean_improvement = exec_df['improvement'].mean()
            if mean_improvement > best_exec_value:
                best_exec_value = mean_improvement
                best_exec_idx = i

        # Destacar a linha do melhor resultado se encontrado
        if best_exec_idx >= 0:
            best_exec_time = unique_executions[best_exec_idx]
            best_exec_df = df[df['execution_time'] == best_exec_time].sort_values('instance_id')
            label_time = best_exec_time.strftime('%d/%m %H:%M')
            
            # Plotar a melhor execução com aparência destacada
            ax[0].plot(best_exec_df['instance_id'], best_exec_df['bov'],
                    marker='*', markersize=14,  # Estrela grande como marcador
                    linestyle='-', linewidth=3.0,
                    color='limegreen', 
                    label=f'MELHOR: Exec {label_time} (média: {best_exec_value:.1f}%)',
                    zorder=4)  # Acima das outras linhas, mas abaixo da oficial
                
            ax[1].plot(best_exec_df['instance_id'], best_exec_df['improvement'],
                    marker='*', markersize=14,
                    linestyle='-', linewidth=3.0,
                    color='limegreen', 
                    label=f'MELHOR: Exec {label_time}',
                    zorder=4)

        # Destacar a execução mais recente
        latest_exec = unique_executions[-1]  # A última na lista ordenada
        latest_df = df[df['execution_time'] == latest_exec].sort_values('instance_id')
        label_time = latest_exec.strftime('%d/%m %H:%M')

        # Plotar a execução mais recente com destaque
        ax[0].plot(latest_df['instance_id'], latest_df['bov'],
                marker='o', markersize=10,  # Marcador maior
                linestyle='-', linewidth=2.5,
                color='deepskyblue',  # Azul vibrante
                label=f'ATUAL: Exec {label_time}',
                zorder=3)

        ax[1].plot(latest_df['instance_id'], latest_df['improvement'],
                marker='o', markersize=10,
                linestyle='-', linewidth=2.5,
                color='deepskyblue',
                label=f'ATUAL: Exec {label_time}',
                zorder=3)

        # Adicionar setas apontando para os pontos da execução mais recente
        for i, row in latest_df.iterrows():
            ax[0].annotate("", xy=(row['instance_id'], row['bov']), 
                          xytext=(row['instance_id'], row['bov'] + 5),
                          arrowprops=dict(arrowstyle="->", color="deepskyblue", lw=1.5))

        fig.suptitle('Comparativo de Execuções por Instância - Projeto MercadoLivre v3', 
                    fontsize=16, y=0.99)
        
        # Ajustar layout para evitar sobreposição
        try:
            fig.tight_layout(rect=[0, 0.03, 0.88, 0.97])
        except ValueError as e:
            print(f"Aviso: Erro ao ajustar layout: {e}. Pode haver sobreposição.")

        # Salvar o gráfico
        try:
            output_path = os.path.join(OUTPUT_DIR, 'bov_executions_comparison.png')
            os.makedirs(OUTPUT_DIR, exist_ok=True)  # Garantir que o diretório existe
            fig.savefig(output_path)
            print(f"Gráfico salvo em: {output_path}")
        except Exception as e:
            print(f"Erro ao salvar o gráfico: {e}")
            traceback.print_exc()

        plt.close(fig)
        
    except Exception as e:
        print(f"Erro ao gerar gráficos: {e}")
        traceback.print_exc()
    
    print("--- Atualização do gráfico concluída ---")

def run_compara(config=None):
    """Função principal para execução a partir do menu."""
    try:
        # Criar diretórios se não existirem
        os.makedirs(LOG_DIR, exist_ok=True)
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        
        print(f"[Monitor] Analisando logs em: {LOG_DIR}")
        print(f"[Monitor] Salvando gráficos em: {OUTPUT_DIR}")
        
        # Gerar gráfico inicial com arquivos existentes
        print("[Monitor] Gerando gráfico inicial...")
        update_plot()

        # Monitoramento contínuo só se watchdog estiver disponível
        if WATCHDOG_AVAILABLE:
            # Perguntar se o usuário quer monitorar continuamente
            choice = input("\nDeseja continuar monitorando novos arquivos de log? (s/n): ").strip().lower()
            if choice.startswith('s'):
                # Iniciar monitoramento de novos arquivos
                print(f"\n[Monitor] Monitorando o diretório para novos arquivos de log...")
                print("Pressione Ctrl+C para encerrar o monitoramento.")
                
                event_handler = LogFileHandler()
                observer = Observer()
                observer.schedule(event_handler, path=LOG_DIR, recursive=False)
                observer.start()

                try:
                    while True:
                        time.sleep(5)
                except KeyboardInterrupt:
                    print("\n[Monitor] Interrupção recebida, parando o monitoramento...")
                    observer.stop()
                finally:
                    observer.join()
                    print("[Monitor] Monitoramento finalizado.")
            else:
                print("\nMonitoramento não iniciado. Voltando ao menu principal.")
        else:
            print("\nMonitoramento contínuo não disponível (watchdog não instalado).")
            print("Apenas a análise inicial foi realizada.")
        
        input("\nPressione Enter para continuar...")
        
    except Exception as e:
        print(f"Erro ao executar a ferramenta de comparação: {e}")
        traceback.print_exc()
        input("\nPressione Enter para continuar...")

# Definição condicional da classe LogFileHandler
if WATCHDOG_AVAILABLE:
    class LogFileHandler(FileSystemEventHandler):
        def on_created(self, event):
            """Chamado quando um arquivo ou diretório é criado."""
            if not event.is_directory and event.src_path.endswith('.txt') and 'validation_log_' in os.path.basename(event.src_path):
                print(f"\n[Monitor] Novo arquivo detectado: {os.path.basename(event.src_path)}")
                time.sleep(1)  # Pequena pausa para garantir que o arquivo esteja completamente escrito
                update_plot()
else:
    # Classe vazia para compatibilidade
    class LogFileHandler:
        pass

if __name__ == "__main__":
    run_compara()