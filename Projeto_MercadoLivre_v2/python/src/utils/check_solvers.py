import os
import subprocess
import sys
import platform
from pathlib import Path
import importlib.util
import pulp

def is_nixos():
    """Verifica se estamos em um ambiente NixOS."""
    try:
        # Verificar se /etc/os-release contém NixOS
        if os.path.exists("/etc/os-release"):
            with open("/etc/os-release", "r") as f:
                return "NixOS" in f.read()
        return False
    except:
        return False

def check_cplex():
    """Verifica se o CPLEX está disponível e retorna informações sobre ele."""
    print("\nVerificando instalação do CPLEX:")
    
    # Verificar se o módulo PuLP está disponível
    pulp_available = importlib.util.find_spec("pulp") is not None
    
    if pulp_available:
        import pulp
        # Verificar se o CPLEX está disponível no PuLP
        cplex_api_available = hasattr(pulp, 'CPLEX') and pulp.CPLEX().available()
        cplex_cmd_available = hasattr(pulp, 'CPLEX_CMD') and pulp.CPLEX_CMD().available()
    else:
        cplex_api_available = False
        cplex_cmd_available = False
    
    print(f"PuLP API Python para CPLEX: {'Disponível' if cplex_api_available else 'Não disponível'}")
    print(f"PuLP CMD para CPLEX: {'Disponível' if cplex_cmd_available else 'Não disponível'}")
    
    # Verificar se docplex está disponível (API Python do CPLEX)
    docplex_available = importlib.util.find_spec("docplex") is not None
    if docplex_available:
        print("DocPlex (API Python do CPLEX): Disponível")
    else:
        print("DocPlex (API Python do CPLEX): Não disponível")
    
    # Locais comuns do CPLEX
    cplex_paths = [
        "/opt/ibm/ILOG/CPLEX_Studio2212/cplex/bin/x86-64_linux/cplex",
        "/opt/ibm/ILOG/CPLEX_Studio221/cplex/bin/x86-64_linux/cplex",
        "/opt/ibm/ILOG/CPLEX_Studio201/cplex/bin/x86-64_linux/cplex",
        "/usr/bin/cplex"
    ]
    
    # Verificar variável de ambiente
    if 'CPLEX_STUDIO_DIR' in os.environ:
        cplex_env_path = os.path.join(os.environ['CPLEX_STUDIO_DIR'], "cplex/bin/x86-64_linux/cplex")
        print(f"Variável de ambiente CPLEX_STUDIO_DIR definida: {os.environ['CPLEX_STUDIO_DIR']}")
        print(f"Verificando {cplex_env_path}: {'Existe' if os.path.exists(cplex_env_path) else 'Não existe'}")
        cplex_paths.insert(0, cplex_env_path)
    
    # Verificar wrapper para NixOS se aplicável
    nixos_environment = is_nixos()
    if nixos_environment:
        print("Ambiente NixOS detectado - verificando wrappers alternativos")
        wrapper_path = os.path.expanduser("~/.local/bin/cplex_wrapper.sh")
        if os.path.exists(wrapper_path):
            print(f"Wrapper CPLEX para NixOS encontrado: {wrapper_path}")
            cplex_paths.insert(0, wrapper_path)
    
    cplex_found = False
    cplex_exec_path = None
    cplex_version = "Desconhecida"
    
    for path in cplex_paths:
        if os.path.exists(path):
            cplex_found = True
            cplex_exec_path = path
            print(f"CPLEX encontrado em: {path}")
            
            # No NixOS, não podemos executar o binário diretamente para verificar a versão
            if not nixos_environment:
                try:
                    # Tentar obter a versão
                    result = subprocess.run([path, "-c", "display version"], 
                                            capture_output=True, text=True, timeout=5)
                    if result.returncode == 0:
                        cplex_version = result.stdout.strip()
                        print(f"Versão CPLEX: {cplex_version}")
                except Exception as e:
                    print(f"Erro ao verificar versão: {str(e)}")
            else:
                print("Versão não verificada (ambiente NixOS)")
            
            # Apenas verifique o primeiro encontrado
            break
    
    if not cplex_found:
        print("CPLEX não encontrado em nenhum dos locais verificados.")
    
    # Verificar suporte a GPU (isso será inferencial)
    print("\nVerificando suporte a GPU para CPLEX:")
    
    has_gpu = False
    try:
        # Este comando verificará se há GPUs NVIDIA disponíveis
        result = subprocess.run(['nvidia-smi'], capture_output=True, text=True, timeout=5)
        if result.returncode == 0:
            has_gpu = True
            print("GPUs NVIDIA detectadas, CPLEX possivelmente pode usar aceleração GPU")
            # Extrair informações básicas da GPU
            gpu_info = result.stdout.split('\n')
            for line in gpu_info[:10]:  # Mostrar apenas as primeiras linhas
                if "NVIDIA-SMI" in line or "|===" in line:
                    continue
                if line.strip():
                    print(f"  {line.strip()}")
        else:
            print("Nenhuma GPU NVIDIA detectada")
    except Exception as e:
        print(f"Erro ao verificar GPUs: {str(e)}")
    
    # Preparar wrapper para NixOS se necessário
    if nixos_environment and cplex_found and not os.path.exists(os.path.expanduser("~/.local/bin/cplex_wrapper.sh")):
        print("\nCriando wrapper para CPLEX em ambiente NixOS...")
        from src.utils.nixos_cplex_wrapper import create_cplex_wrapper
        cplex_dir = os.path.dirname(os.path.dirname(os.path.dirname(cplex_exec_path)))
        wrapper_path = create_cplex_wrapper(cplex_dir)
        if wrapper_path:
            print(f"Wrapper criado com sucesso: {wrapper_path}")
    
    return {
        "available": cplex_found,
        "version": cplex_version,
        "api_available": cplex_api_available,
        "cmd_available": cplex_cmd_available,
        "gpu_support": has_gpu,
        "nixos": nixos_environment,
        "exec_path": cplex_exec_path
    }

def check_solvers():
    """Verifica e exibe informações sobre os solvers disponíveis."""
    # Verificar ambiente
    print("\n" + "="*50)
    print("VERIFICAÇÃO DO AMBIENTE")
    print("="*50)
    print(f"Sistema: {platform.system()} {platform.release()}")
    print(f"Python: {platform.python_version()}")
    
    # Verificar PuLP
    print("\n" + "="*50)
    print("VERIFICAÇÃO DO PULP")
    print("="*50)
    
    try:
        import pulp
        print(f"PuLP versão: {pulp.__version__}")
        print("\nSolvers disponíveis:")
        
        # Verificar solvers comuns
        solvers = [
            ("CBC", pulp.PULP_CBC_CMD().available()),
            ("GLPK", pulp.GLPK_CMD().available() if hasattr(pulp, 'GLPK_CMD') else False),
            ("CPLEX", pulp.CPLEX_CMD().available() if hasattr(pulp, 'CPLEX_CMD') else False),
            ("GUROBI", pulp.GUROBI_CMD().available() if hasattr(pulp, 'GUROBI_CMD') else False),
            ("COIN", pulp.COIN_CMD().available() if hasattr(pulp, 'COIN_CMD') else False),
            ("SCIP", pulp.SCIP_CMD().available() if hasattr(pulp, 'SCIP_CMD') else False)
        ]
        
        # Verificar HiGHS (versões mais recentes do PuLP)
        if hasattr(pulp, 'HIGHS_CMD'):
            solvers.append(("HiGHS", pulp.HIGHS_CMD().available()))
        
        for solver_name, available in solvers:
            status = "Disponível" if available else "Não disponível"
            print(f"• {solver_name}: {status}")
            
    except ImportError:
        print("PuLP não está instalado.")
    
    # Adicionar verificação do CPLEX
    print("\n" + "="*50)
    print("VERIFICAÇÃO DO CPLEX")
    print("="*50)
    cplex_info = check_cplex()
    
    if cplex_info["available"]:
        print("\nResumo CPLEX:")
        print(f"• CPLEX instalado: Sim")
        print(f"• Versão: {cplex_info['version']}")
        print(f"• Suporte a API Python: {'Sim' if cplex_info['api_available'] else 'Não'}")
        print(f"• Suporte a linha de comando: {'Sim' if cplex_info['cmd_available'] else 'Não'}")
        print(f"• Ambiente NixOS: {'Sim' if cplex_info['nixos'] else 'Não'}")
        print(f"• Suporte potencial a GPU: {'Sim' if cplex_info['gpu_support'] else 'Não'}")
    else:
        print("\nCPLEX não encontrado ou não configurado corretamente.")

class NixOSCplexSolver(pulp.LpSolver):
    """Solver CPLEX personalizado para NixOS usando ambiente FHS."""
    
    def __init__(self, cplex_path, timeLimit=None, msg=True, options=None):
        """Inicializa o solver CPLEX para NixOS."""
        super().__init__()