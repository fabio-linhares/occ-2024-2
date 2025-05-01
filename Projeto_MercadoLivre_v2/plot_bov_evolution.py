import os
import re
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import pandas as pd
from datetime import datetime
import glob
import numpy as np
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import matplotlib.dates as mdates  # Para formatar datas na legenda se necessário

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

        instance_match = re.search(r'Arquivo de entrada:.*?instance_(\d+)\.txt', instance_block, re.IGNORECASE)
        if not instance_match:
            continue
        instance_id = int(instance_match.group(1))

        bov_data_match = re.search(
            r'Valor objetivo \(BOV\):\s*(\d+\.?\d*)\s*BOV oficial:\s*(\d+\.?\d*)',
            instance_block,
            re.IGNORECASE | re.DOTALL
        )

        if bov_data_match:
            try:
                bov = float(bov_data_match.group(1))
                bov_official = float(bov_data_match.group(2))
                instances[instance_id] = {
                    'bov': bov,
                    'bov_official': bov_official
                }
            except ValueError:
                print(f"Erro ao converter BOV/BOV Oficial para float no arquivo {log_file}, instância {instance_id}")

    return instances

LOG_DIR = '/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v2/data'
OUTPUT_DIR = '/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v2/data/graphs'

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

    fig, ax = plt.subplots(2, 1, figsize=(18, 16), sharex=True)
    colors = plt.cm.viridis(np.linspace(0, 1, len(unique_executions)))

    ax[0].set_title('Comparativo do BOV Obtido por Execução vs BOV Oficial')
    ax[0].set_ylabel('BOV (Valor Objetivo)')
    ax[0].grid(True, linestyle='--', alpha=0.6)
    ax[0].set_xticks(unique_instances)
    ax[0].tick_params(axis='x', rotation=45)

    ax[0].plot(df_official['instance_id'], df_official['bov_official'],
               marker='D', markersize=6,
               linestyle='--', linewidth=2.0,
               color='black', label='BOV Oficial (Referência)',
               zorder=3)

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

    ax[0].legend(handles=handles_exec, labels=labels_exec, title='Execuções', loc='center left', bbox_to_anchor=(1.02, 0.5), fontsize='small')

    ax[1].set_title('Porcentagem do BOV Obtido em Relação ao BOV Oficial por Execução')
    ax[1].set_xlabel('ID da Instância')
    ax[1].set_ylabel('BOV / BOV Oficial (%)')
    ax[1].grid(True, linestyle=':', alpha=0.5)
    ax[1].axhline(100, color='red', linestyle='-.', linewidth=1.5, label='100% (Meta Oficial)', zorder=3)
    ax[1].yaxis.set_major_formatter(mticker.PercentFormatter())
    ax[1].set_xticks(unique_instances)

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

    ax[1].legend(handles=handles_perc, labels=labels_perc, title='Execuções', loc='center left', bbox_to_anchor=(1.02, 0.5), fontsize='small')

    fig.suptitle('Comparativo de Execuções por Instância - Projeto MercadoLivre v2', fontsize=16, y=0.99)
    try:
        fig.tight_layout(rect=[0, 0.03, 0.88, 0.97])
    except ValueError as e:
        print(f"Aviso: Erro ao ajustar layout: {e}. Pode haver sobreposição.")

    try:
        output_path = os.path.join(OUTPUT_DIR, 'bov_executions_comparison.png')
        fig.savefig(output_path)
        print(f"Gráfico de comparação por execução atualizado em {output_path}")
    except Exception as e:
        print(f"Erro ao salvar o gráfico de comparação: {e}")

    plt.close(fig)
    print("--- Atualização do gráfico concluída ---")

class LogFileHandler(FileSystemEventHandler):
    def on_created(self, event):
        """Chamado quando um arquivo ou diretório é criado."""
        if not event.is_directory and event.src_path.endswith('.txt') and 'validation_log_' in os.path.basename(event.src_path):
            print(f"\n[Monitor] Novo arquivo detectado: {os.path.basename(event.src_path)}")
            time.sleep(1)
            update_plot()

if __name__ == "__main__":
    try:
        os.makedirs(OUTPUT_DIR, exist_ok=True)
    except OSError as e:
        print(f"Erro ao criar diretório de saída {OUTPUT_DIR}: {e}")
        exit()

    print("[Monitor] Gerando gráfico inicial...")
    update_plot()

    print(f"[Monitor] Monitorando o diretório '{LOG_DIR}' para novos arquivos de log...")
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
    except Exception as e:
        print(f"\n[Monitor] Erro inesperado: {e}")
        observer.stop()

    observer.join()
    print("[Monitor] Monitoramento finalizado.")
