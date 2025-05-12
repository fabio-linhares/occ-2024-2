import traceback

from src.challenge_runner import solve_challenge
from src.config_manager import load_config
from src.ui.console import display_markdown_file, clear_screen  # Adicionando clear_screen
from src.ui.menu import display_menu, load_menu_options
from src.verificar_instancias import verificar_instancias
from src.utils.check_solvers import check_solvers, check_cplex, is_nixos
from compara import run_compara
import os
import shutil
import glob
import subprocess
import tempfile
import pulp 
import os
# -*- coding: utf-8 -*-


# Carrega configurações globais
CONFIG = load_config()

# Atualizar a função no arquivo principal main.py
def validar_resultados(CONFIG):
    """Valida os resultados das soluções de forma detalhada."""
    from src.utils.solution_validator import AdvancedSolutionValidator
    AdvancedSolutionValidator.validar_resultados(CONFIG)

def verificar_cplex(CONFIG):
    """Verifica a disponibilidade e configuração do CPLEX."""
    clear_screen()
    print("VERIFICAÇÃO DO CPLEX".center(60))
    print("=" * 60)
    
    from src.utils.check_solvers import check_cplex, is_nixos
    
    cplex_info = check_cplex()
    nixos_detected = is_nixos()
    
    if cplex_info["available"]:
        print("\nO CPLEX está instalado e parece estar configurado corretamente.")
        print(f"\nVersão detectada: {cplex_info['version']}")
        
        # Mostrar configuração atual
        cplex_config = CONFIG.get('cplex', {})
        print("\nConfiguração atual:")
        print(f"• Caminho: {cplex_config.get('path', 'Não configurado')}")
        print(f"• Modo de aceleração: {cplex_config.get('acceleration', 'auto')}")
        print(f"• Threads: {cplex_config.get('num_threads', 0)}")
        print(f"• Modo paralelo: {cplex_config.get('parallel_mode', 0)}")
        
        # Verificar suporte a GPU
        if cplex_info["gpu_support"]:
            print("\n🚀 Suporte a GPU detectado! CPLEX está configurado para usar aceleração GPU.")
        else:
            print("\nNenhuma GPU compatível detectada. CPLEX usará apenas CPU.")
        
        # Perguntar se o usuário quer testar
        if input("\nDeseja executar um teste com o CPLEX? (s/n): ").lower().startswith('s'):
            print("\nExecutando teste simples com CPLEX...")
            try:
                # import pulp
                # import os
                
                model = pulp.LpProblem("TestCPLEX", pulp.LpMinimize)
                x = pulp.LpVariable("x", lowBound=0)
                y = pulp.LpVariable("y", lowBound=0)
                model += x + y
                model += x >= 1
                model += y >= 1
                
                # Caminho para o executável CPLEX
                cplex_path = cplex_config.get('path', '')
                if cplex_path:
                    cplex_exec = os.path.join(cplex_path, "cplex", "bin", "x86-64_linux", "cplex")
                    if os.path.exists(cplex_exec):
                        if nixos_detected:
                            from src.utils.check_solvers import NixOSCplexSolver
                            solver = NixOSCplexSolver(cplex_path, msg=True)
                        else:
                            solver = pulp.CPLEX_CMD(path=cplex_exec, msg=True)
                            
                        status = model.solve(solver)
                        print(f"\nResultado: {pulp.LpStatus[status]}")
                        print(f"Valor objetivo: {pulp.value(model.objective)}")
                        print(f"x = {x.value()}, y = {y.value()}")
                    else:
                        print(f"Executável CPLEX não encontrado em: {cplex_exec}")
                else:
                    print("Caminho CPLEX não configurado.")
            except Exception as e:
                print(f"\nErro ao executar teste: {str(e)}")
    else:
        print("\nCPLEX não encontrado ou não configurado corretamente.")
        print("\nVerifique os seguintes pontos:")
        print("1. Garanta que o CPLEX está instalado")
        print("2. Configure o caminho do CPLEX na configuração")
        print("3. Adicione o diretório bin do CPLEX ao PATH")
    
    input("\nPressione Enter para continuar...")

def verificar_gpu(CONFIG):
    """Verifica suporte a GPU/CUDA e exibe informações."""
    clear_screen()
    print("VERIFICAÇÃO DE SUPORTE A GPU/CUDA".center(60))
    print("=" * 60)
    
    try:
        # Tentar importar CuPy
        import cupy as cp
        print("\n✅ CuPy importado com sucesso!")
        
        # Verificar versão do CuPy
        print(f"Versão do CuPy: {cp.__version__}")
        
        # Verificar dispositivos CUDA disponíveis
        print("\nDispositivos CUDA disponíveis:")
        try:
            num_gpus = cp.cuda.runtime.getDeviceCount()
            print(f"Quantidade de GPUs: {num_gpus}")
            
            for i in range(num_gpus):
                device = cp.cuda.Device(i)
                props = cp.cuda.runtime.getDeviceProperties(i)
                print(f"\nGPU {i}:")
                print(f"  Nome: {props['name'].decode()}")
                print(f"  Memória total: {props['totalGlobalMem'] / (1024**3):.2f} GB")
                print(f"  Compute Capability: {props['major']}.{props['minor']}")
                print(f"  Multiprocessadores: {props['multiProcessorCount']}")
                
                # Testar uma operação simples para verificar funcionamento
                x = cp.array([1, 2, 3])
                y = cp.array([4, 5, 6])
                z = cp.add(x, y)
                print(f"  Teste de operação: {z.get()}")
        except Exception as e:
            print(f"Erro ao verificar dispositivos: {str(e)}")
        
    except ImportError:
        print("\n❌ CuPy não está instalado ou não pôde ser importado.")
        print("Para habilitar aceleração GPU, instale CuPy:")
        print("pip install cupy-cuda12x # para CUDA 12.x")
        print("pip install cupy-cuda11x # para CUDA 11.x")
    
    # Verificar bibliotecas relacionadas
    try:
        import numpy as np
        print(f"\nNumPy: ✅ (versão {np.__version__})")
    except ImportError:
        print("\nNumPy: ❌ (não instalado)")
    
    # Verificar configuração atual
    print("\nConfiguração atual para GPU:")
    gpu_enabled = CONFIG.get('algorithm', {}).get('use_gpu', False)
    print(f"• Aceleração GPU habilitada: {'Sim' if gpu_enabled else 'Não'}")
    
    # Oferecer opção para habilitar/desabilitar
    response = input("\nDeseja alterar a configuração de GPU? (s/n): ").strip().lower()
    if response.startswith('s'):
        CONFIG['algorithm']['use_gpu'] = not gpu_enabled
        print(f"Aceleração GPU {'habilitada' if CONFIG['algorithm']['use_gpu'] else 'desabilitada'}.")
    
    input("\nPressione Enter para continuar...")

def configurar_metaheuristica(CONFIG):
    """Configura parâmetros das meta-heurísticas."""
    from src.ui.metaheuristic_dialog import MetaheuristicDialog
    
    clear_screen()
    
    # Criar e exibir o diálogo de configuração
    dialog = MetaheuristicDialog(CONFIG)
    updated_config = dialog.display()
    
    if updated_config:
        # Atualizar a configuração global
        CONFIG.update(updated_config)
        print("\nConfigurações de meta-heurísticas atualizadas com sucesso!")
    else:
        print("\nConfigurações não foram alteradas.")
    
    input("\nPressione Enter para continuar...")
    return CONFIG

def resolver_com_metaheuristica(config):
    """Resolve o desafio usando apenas meta-heurística configurada."""
    from src.challenge_runner import solve_challenge
    
    # Fazer uma cópia das configurações atuais
    meta_config = config.copy()
    
    # Forçar o uso da meta-heurística configurada
    meta_config['algorithm']['solver'] = meta_config.get('meta_heuristic', {}).get('method', 'ils')
    
    print("\n" + "=" * 60)
    print("RESOLVER COM META-HEURÍSTICA".center(60))
    print("=" * 60)
    
    # Mostrar informações sobre a meta-heurística que será usada
    method = meta_config['algorithm']['solver']
    method_names = {
        'ils': 'ILS (Iterated Local Search)',
        'simulated_annealing': 'Simulated Annealing',
        'grasp': 'GRASP'
    }
    
    print(f"\nMeta-heurística selecionada: {method_names.get(method, method)}")
    print("Parâmetros configurados:")
    for param, value in meta_config.get('meta_heuristic', {}).items():
        print(f"• {param}: {value}")
    
    print("\nUsando apenas a meta-heurística para resolver o problema.")
    print("Esta abordagem pode ser mais rápida, mas possivelmente menos precisa que o PLI.")
    
    # Perguntar se o usuário quer continuar
    choice = input("\nContinuar com a resolução? (S/n): ").strip().lower()
    if choice and choice not in ['s', 'sim']:
        print("Operação cancelada.")
        input("\nPressione Enter para continuar...")
        return
    
    # Chamar a função solve_challenge com as configurações modificadas
    solve_challenge(meta_config)

def main():
    """Função principal que inicia o programa."""
    # Carrega opções do menu a partir do arquivo
    menu_file = "menu_options.txt"
    menu_options = load_menu_options(menu_file)
    
    # Loop principal
    while True:
  
            
        choice = display_menu(menu_options)

        if choice == 0:  # Sair
            print("Limpando diretórios de cache...")

            # Encontrar e remover diretórios __pycache__
            pycache_dirs = []
            for root, dirs, _ in os.walk('.'):
                for dir_name in dirs:
                    if '__pycache__' in dir_name or 'pycache' in dir_name.lower():
                        pycache_dirs.append(os.path.join(root, dir_name))

            if pycache_dirs:
                print("Removendo diretórios de cache:")
                for dir_path in pycache_dirs:
                    print(f"  {dir_path}")
                    shutil.rmtree(dir_path)
            else:
                print("Nenhum diretório de cache encontrado.")

            # Remover arquivos clone*.log na raiz
            log_files = glob.glob('clone*.log')
            if log_files:
                print("Removendo arquivos clone*.log:")
                for log_file in log_files:
                    print(f"  {log_file}")
                    os.remove(log_file)
            else:
                print("Nenhum arquivo clone*.log encontrado.")

            # Limpar a tela conforme o sistema operacional
            os.system('cls' if os.name == 'nt' else 'clear')
            print("Suplício Encerrado!")
            break

        option, value = menu_options[choice]
        
        # Mapeamento direto de nomes de funções para implementações
        function_map = {
            "solve_challenge": solve_challenge,
            "verificar_instancias": verificar_instancias,
            "validar_resultados": validar_resultados,
            "verificar_cplex": verificar_cplex,
            "verificar_gpu": verificar_gpu,
            "configurar_metaheuristica": configurar_metaheuristica,
            "resolver_com_metaheuristica": resolver_com_metaheuristica,  # Nova função
            "comparar_resultados": run_compara, 
        }
        
        try:
            # Verifica o tipo de ação baseado no valor associado
            if isinstance(value, str) and "." in value:
                # É um arquivo
                display_markdown_file(value)
            elif isinstance(value, str) and value in function_map:
                # É uma função conhecida, executa com CONFIG
                function_map[value](CONFIG)
            elif value == "comparar_resultados":
                run_compara(CONFIG)
            else:
                print(f"Opção {option} ainda não implementada ou inválida.")
                input("\nPressione Enter para continuar...")
        except Exception as e:
            print(f"\nErro ao executar a opção {option}: {str(e)}")
            traceback.print_exc()  # Adicionado para mostrar o stack trace completo
            input("\nPressione Enter para continuar...")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nPrograma interrompido pelo usuário.")
    except Exception as e:
        print(f"\nErro inesperado: {str(e)}")
        traceback.print_exc()
        input("\nPressione Enter para continuar...")