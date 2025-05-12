import os
import glob
from src.utils.validator import SolutionValidator
from src.ui.console import clear_screen

def verificar_instancias(config):
    """Verifica e exibe informações sobre as instâncias disponíveis."""
    clear_screen()
    print("VERIFICAÇÃO DE INSTÂNCIAS".center(60))
    print("=" * 60)
    
    # Usar diretório de instâncias da configuração
    instances_dir = config["paths"]["instances_dir"]
    
    print(f"\nProcurando instâncias em: {instances_dir}\n")
    
    # Procurar todos os subdiretórios
    subdirs = [d for d in os.listdir(instances_dir) if os.path.isdir(os.path.join(instances_dir, d))]
    
    if not subdirs:
        print(f"Nenhum subdiretório encontrado em {instances_dir}")
        input("\nPressione Enter para continuar...")
        return
    
    # Exibir subdiretórios disponíveis
    print("Subdiretórios disponíveis:")
    for i, subdir in enumerate(subdirs, 1):
        print(f"{i}. {subdir}/")
    
    while True:
        try:
            subdir_idx = int(input("\nEscolha um subdiretório (número): ")) - 1
            if 0 <= subdir_idx < len(subdirs):
                break
            else:
                print(f"Por favor, escolha um número entre 1 e {len(subdirs)}.")
        except ValueError:
            print("Por favor, digite um número válido.")
    
    subdir = subdirs[subdir_idx]
    subdir_path = os.path.join(instances_dir, subdir)
    
    # Procurar instâncias no subdiretório
    instances = sorted(glob.glob(os.path.join(subdir_path, "instance_*.txt")))
    
    if not instances:
        print(f"\nNenhuma instância encontrada em {subdir_path}")
        input("\nPressione Enter para continuar...")
        return
    
    print(f"\nArquivos de instância disponíveis em {subdir_path}:")
    for i, instance in enumerate(instances, 1):
        print(f"{i}. {os.path.basename(instance)}")
    
    while True:
        try:
            file_idx = int(input("\nEscolha um arquivo para análise (número): ")) - 1
            if 0 <= file_idx < len(instances):
                break
            else:
                print(f"Por favor, escolha um número entre 1 e {len(instances)}.")
        except ValueError:
            print("Por favor, digite um número válido.")
    
    instance_file = instances[file_idx]
    
    # Analisar e exibir informações da instância
    # print(f"\nAnalisando instância: {os.path.basename(instance_file)}")
    instance_data = SolutionValidator.analyze_instance(instance_file, config) # Passar config
    SolutionValidator.display_instance_brief(instance_data)
    
    input("\nPressione Enter para continuar...")