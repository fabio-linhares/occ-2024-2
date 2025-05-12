import os
import glob
import sys
import traceback
import re
from time import time
from src.models.problem import WaveOrderPickingProblem
from src.factories.solver_factory import SolverFactory
from src.ui.config_dialog import PLIConfigDialog
from src.ui.console import clear_screen
import concurrent.futures

def solve_challenge(config):
    """Resolve o desafio usando o algoritmo implementado."""
    clear_screen()
    print("RESOLVER DESAFIO".center(60))
    print("=" * 60)
    
    # Usar diretório de instâncias da configuração como padrão
    default_input_dir = config["paths"]["instances_dir"]
    default_output_dir = config["paths"]["results_dir"]
    
    # Inicializar a lista de instâncias no início da função para garantir que ela sempre exista
    instances = []
    results = []
    
    # Primeiro perguntar se deseja usar PLI ou Heurística
    print("\nEscolha o tipo de solver:")
    print("1. PLI (Programação Linear Inteira) - mais preciso, mais lento")
    print("2. Heurística - mais rápido, pode ser menos preciso")
    
    solver_choice = input("\nEscolha uma opção (1/2) [1]: ").strip() or "1"
    
    # Após a escolha do tipo de solver
    if solver_choice == "2":
        # Configurar para usar heurística
        config["algorithm"]["strategy"] = "heuristic"
        
        # Garantir que a seção meta_heuristic exista
        if "meta_heuristic" not in config:
            config["meta_heuristic"] = {}
        
        # Definir ILS como método padrão se não estiver definido
        if "method" not in config["meta_heuristic"]:
            config["meta_heuristic"]["method"] = "ils"
            
        # Configurar alguns parâmetros padrão para ILS
        if config["meta_heuristic"]["method"] == "ils":
            if "max_iterations" not in config["meta_heuristic"]:
                config["meta_heuristic"]["max_iterations"] = 100
            if "perturbation_strength" not in config["meta_heuristic"]:
                config["meta_heuristic"]["perturbation_strength"] = 0.3
            if "max_iterations_without_improvement" not in config["meta_heuristic"]:
                config["meta_heuristic"]["max_iterations_without_improvement"] = 20
        
        print("\nUtilizando solver heurístico.")
    else:
        config["algorithm"]["strategy"] = "pli"
        print("\nUtilizando solver PLI.")
    
    # Adicionar uma linha em branco antes das opções de resolução para melhor legibilidade
    print("\nOpções de resolução:")
    print("1. Resolver todas as instâncias de todos os subdiretórios")
    print("2. Resolver instâncias de um subdiretório específico")
    print("3. Resolver uma instância específica")
    
    choice = input("\nEscolha uma opção (1/2/3): ").strip()
    
    # Definir tempo limite
    timeout = config.get("algorithm", {}).get("timeout", 600)
    print(f"\nTempo limite atual: {timeout} segundos")
    if input("Deseja ajustar o tempo limite? (s/n): ").lower().startswith('s'):
        new_timeout = input("Novo tempo limite (segundos): ")
        try:
            timeout = float(new_timeout)
            config["algorithm"]["timeout"] = timeout
        except ValueError:
            print("Valor inválido, mantendo o tempo limite atual.")
    
    # Configurar parâmetros de otimização apenas se for usar PLI
    if solver_choice == "1":
        print("\nConfigurando parâmetros para otimização:")
        config_dialog = PLIConfigDialog(config)
        config = config_dialog.display()
    else:
        # Se for heurística, perguntar se quer configurar
        if input("\nDeseja configurar parâmetros da heurística? (s/n): ").lower().startswith('s'):
            from src.ui.metaheuristic_dialog import MetaheuristicDialog
            metaheuristic_dialog = MetaheuristicDialog(config)
            config = metaheuristic_dialog.display()
    
    # Definir quais instâncias resolver com base na escolha
    if choice == "1":
        # Resolver todas as instâncias
        subdirs = None
        results = _process_all_instances(config, subdirs)
    elif choice == "2":
        # Resolver instâncias de um subdiretório
        print(f"\nSubdiretórios disponíveis em {default_input_dir}:")
        subdirs = [d for d in os.listdir(default_input_dir) if os.path.isdir(os.path.join(default_input_dir, d))]
        
        if not subdirs:
            print(f"\nNenhum subdiretório encontrado em {default_input_dir}")
            input("\nPressione Enter para continuar...")
            return
            
        for i, subdir in enumerate(subdirs, 1):
            print(f"{i}. {subdir}/")
        
        while True:
            try:
                subdir_idx = int(input("\nEscolha um subdiretório (número): ")) - 1
                if 0 <= subdir_idx < len(subdirs):
                    subdirs = [subdirs[subdir_idx]]
                    break
                else:
                    print(f"Por favor, escolha um número entre 1 e {len(subdirs)}.")
            except ValueError:
                print("Por favor, digite um número válido.")
        
        # Processar instâncias com as configurações já definidas
        results = _process_all_instances(config, subdirs)
    elif choice == "3":
        # Resolver uma instância específica - fluxo simplificado
        print("\nBuscando todas as instâncias disponíveis...")
        
        # Primeiro, coletar todos os arquivos de instância disponíveis
        all_instance_files = []
        all_subdirs = [d for d in os.listdir(default_input_dir) if os.path.isdir(os.path.join(default_input_dir, d))]
        
        for subdir in all_subdirs:
            subdir_path = os.path.join(default_input_dir, subdir)
            instance_files = glob.glob(os.path.join(subdir_path, "instance_*.txt"))
            for file in instance_files:
                all_instance_files.append((file, subdir))
        
        # Ordenar numericamente por número de instância
        all_instance_files.sort(key=lambda x: int(re.search(r'instance_(\d+)\.txt', os.path.basename(x[0])).group(1)))
        
        # Exibir todas as instâncias disponíveis
        print(f"\nInstâncias disponíveis:")
        for i, (file_path, subdir) in enumerate(all_instance_files, 1):
            file_name = os.path.basename(file_path)
            print(f"{i}. {subdir}/{file_name}")
            
        # Selecionar instância
        while True:
            try:
                file_idx = int(input("\nEscolha uma instância (número): ")) - 1
                if 0 <= file_idx < len(all_instance_files):
                    instance_path = all_instance_files[file_idx][0]
                    break
                else:
                    print(f"Por favor, escolha um número entre 1 e {len(all_instance_files)}.")
            except ValueError:
                print("Por favor, digite um número válido.")
                
        instances = [instance_path]
    
        for i, instance_path in enumerate(instances, 1):
            instance_name = os.path.basename(instance_path)
            print(_format_instance_header(instance_name))
            print(f"[{i}/{len(instances)}] Processando: {instance_path}")
            
            # Definir start_time aqui para evitar o erro de referência não definida
            start_time = time()
            
            # Carregar o problema
            try:
                problem = WaveOrderPickingProblem(config=config)
                problem.read_input(instance_path)
            
                # Analisar a instância
                n_orders = problem.n_orders
                n_items = problem.n_items
                n_aisles = problem.n_aisles
                lb = problem.wave_size_lb
                ub = problem.wave_size_ub
                
                print(f"  • Número de pedidos: {n_orders}")
                print(f"  • Número de tipos de itens: {n_items}")
                print(f"  • Número de corredores: {n_aisles}")
                print(f"  • Limites de Wave: LB={lb}, UB={ub}")
                
                # Criar solver com o método configurado
                solver = SolverFactory.create_solver(problem, config)
                
                # Resolver o problema
                solution = solver.solve(start_time)
                
                # Verificar se encontrou solução viável
                if solution is None or not solution.is_feasible:
                    # Evitar mensagem duplicada, o solver já deve ter imprimido isso
                    pass
                else:
                    # Exibir estatísticas da solução
                    print(f"\nSolução encontrada!")
                    print(f"  • Pedidos selecionados: {len(solution.selected_orders)}")
                    print(f"  • Corredores visitados: {len(solution.visited_aisles)}")
                    print(f"  • Total de unidades: {solution.total_units}")
                    print(f"  • Valor objetivo: {solution.objective_value:.4f}")
                    
                    # Salvar resultado
                    output_filename = os.path.join(
                        default_output_dir,
                        os.path.relpath(instance_path, default_input_dir).replace(".txt", "_result.txt")
                    )
                    
                    # Garantir que o diretório de saída exista
                    os.makedirs(os.path.dirname(output_filename), exist_ok=True)
                    
                    # Escrever a solução no arquivo
                    problem.write_output(solution, output_filename)
                    print(f"\nSolução salva em: {output_filename}")
            except Exception as e:
                print(f"\nErro ao processar instância: {str(e)}")
                traceback.print_exc()  # Mostrar o traceback para debug
    
    # Resolver instâncias
    if choice == "1" or choice == "2":
        pass  # Já processado acima
    
    # Exibir relatório final - verificar se há instâncias antes
    if instances and len(instances) > 1:
        print("\n" + "=" * 80)
        print("RELATÓRIO FINAL".center(80))
        print("=" * 80)
        
        success_count = sum(1 for r in results if r.get('success', False))
        print(f"Total de instâncias: {len(instances)}")
        print(f"Soluções viáveis encontradas: {success_count} ({success_count/len(instances)*100:.1f}%)")
        
        if success_count > 0:
            avg_obj = sum(r.get('objective', 0) for r in results if r.get('success', False)) / success_count
            print(f"Valor médio da função objetivo: {avg_obj:.4f}")
    
    input("\nPressione Enter para continuar...")

def _process_all_instances(config, subdirs=None):
    """Processa todas as instâncias dos subdiretórios especificados."""
    # Obter diretórios a processar
    input_dir = config["paths"]["instances_dir"]
    output_dir = config["paths"]["results_dir"]
    
    if not subdirs:
        subdirs = [d for d in os.listdir(input_dir) if os.path.isdir(os.path.join(input_dir, d))]
    
    all_instances = []
    for subdir in subdirs:
        subdir_path = os.path.join(input_dir, subdir)
        instances = glob.glob(os.path.join(subdir_path, "instance_*.txt"))
        all_instances.extend(instances)
    
    # Configurar número de threads baseado na CPU/GPU
    max_workers = os.cpu_count() - 1 if os.cpu_count() > 1 else 1
    if config.get('algorithm', {}).get('use_gpu', False):
        # Reduzir paralelismo para não sobrecarregar a GPU
        max_workers = min(max_workers, 2)
    
    # Processar instâncias em paralelo
    results = []
    
    print(f"Processando {len(all_instances)} instâncias com {max_workers} workers...")
    
    with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers) as executor:
        # Usar dicionário para rastrear qual instância corresponde a cada future
        futures = {}
        for instance_path in all_instances:
            future = executor.submit(_process_instance, instance_path, output_dir, config)
            futures[future] = instance_path
        
        for future in concurrent.futures.as_completed(futures):
            instance_path = futures[future]  # Recuperar a instância deste future
            instance_name = os.path.basename(instance_path)
            try:
                result = future.result()
                if result is not None:
                    print(_format_instance_header(instance_name))
                    results.append(result)
                    print(f"Concluído: {instance_name}")
                else:
                    print(_format_instance_header(instance_name))
                    print(f"Aviso: Resultado nulo para {instance_name}")
                    # Adicionar resultado de erro para manter rastreamento
                    results.append({
                        'instance': instance_path,
                        'success': False,
                        'error': "Resultado nulo do worker",
                        'time': 0
                    })
            except Exception as e:
                print(f"Erro ao processar {os.path.basename(instance_path)}: {str(e)}")
                # Adicionar resultado de erro
                results.append({
                    'instance': instance_path,
                    'success': False,
                    'error': str(e),
                    'time': 0
                })
    
    return results

def _process_instance(instance_path, output_dir, config):
    """Processa uma única instância."""
    instance_name = os.path.basename(instance_path)
    start_time = time()
    result = {
        'instance': instance_path,
        'success': False,
        'error': None,
        'time': 0,
        'name': instance_name  # Adicionar o nome para uso posterior
    }
    
    try:
        # Carregar o problema
        problem = WaveOrderPickingProblem(config=config)
        problem.read_input(instance_path)
        
        # Analisar a instância
        n_orders = problem.n_orders
        n_items = problem.n_items
        n_aisles = problem.n_aisles
        lb = problem.wave_size_lb
        ub = problem.wave_size_ub
        
        # Verificação de validade da instância
        if n_orders == 0:
            result['error'] = "Instância inválida - 0 pedidos"
            result['time'] = time() - start_time
            return result
                
        if n_aisles == 0:
            result['error'] = "Instância inválida - 0 corredores"
            result['time'] = time() - start_time
            return result
        
        # Pegar o método da configuração
        method = config.get("algorithm", {}).get("strategy", "heuristic")
        
        # Criar solver com o método configurado
        solver = SolverFactory.create_solver(problem, method, config)
        
        # Resolver o problema
        solution = solver.solve(start_time)
        
        # Verificar se encontrou solução
        if solution is None or not solution.is_feasible:
            result['error'] = "Sem solução viável"
            result['time'] = time() - start_time
            return result
        
        # Salvar resultado
        output_filename = os.path.join(
            output_dir,
            os.path.relpath(instance_path, config["paths"]["instances_dir"]).replace(".txt", "_result.txt")
        )
        
        # Garantir que o diretório de saída exista
        os.makedirs(os.path.dirname(output_filename), exist_ok=True)
        
        # Escrever a solução no arquivo
        problem.write_output(solution, output_filename)
        
        # Completar resultados
        result['success'] = True
        result['objective'] = solution.objective_value
        result['n_orders'] = len(solution.selected_orders)
        result['n_aisles'] = len(solution.visited_aisles)
        result['total_units'] = solution.total_units
        result['time'] = time() - start_time
        
    except Exception as e:
        result['error'] = str(e)
        result['time'] = time() - start_time
    
    return result  # Importante: sempre retornar o resultado com o nome

def _format_instance_header(instance_name):
    """Formata um cabeçalho atraente para exibir o nome da instância."""
    border = "#" * 60
    padding = "#" + " " * 58 + "#"
    title = f"#     INSTÂNCIA: {instance_name}" + " " * (42 - len(instance_name)) + "#"
    return f"\n{border}\n{padding}\n{title}\n{padding}\n{border}"