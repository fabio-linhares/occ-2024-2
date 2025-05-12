#!/usr/bin/env python3
"""
Wrapper para o CPLEX em ambientes NixOS.
Este script serve como intermediário para executar o CPLEX em ambientes NixOS,
que não podem executar binários dinâmicos genéricos diretamente.
"""
import os
import sys
import subprocess
import tempfile
import shutil
import time

def detect_nixos():
    """Detecta se estamos em um ambiente NixOS."""
    try:
        # Verificar se /etc/os-release contém NIXOS
        if os.path.exists("/etc/os-release"):
            with open("/etc/os-release", "r") as f:
                return "NixOS" in f.read()
        return False
    except:
        return False

def create_cplex_wrapper(cplex_path):
    """
    Cria um wrapper de script para o CPLEX que funciona no NixOS.
    
    Args:
        cplex_path: Caminho para o diretório do CPLEX
        
    Returns:
        str: Caminho para o script wrapper ou None se falhar
    """
    try:
        # Localizar o executável CPLEX (corrigindo o caminho)
        cplex_exec = os.path.join(cplex_path, "cplex/bin/x86-64_linux/cplex")
        if not os.path.exists(cplex_exec):
            print(f"Executável CPLEX não encontrado em: {cplex_exec}")
            return None
            
        # Criar diretório para o wrapper se não existir
        wrapper_dir = os.path.expanduser("~/.local/bin")
        os.makedirs(wrapper_dir, exist_ok=True)
        
        # Criar wrapper script FHS
        wrapper_path = os.path.join(wrapper_dir, "cplex_wrapper.sh")
        with open(wrapper_path, "w") as f:
            f.write(f"""#!/bin/sh
# CPLEX wrapper para NixOS usando FHS environment
# Este script deve ser executado dentro do ambiente criado por cplex_fhs.nix

export CPLEX_STUDIO_DIR="{cplex_path}"
export LD_LIBRARY_PATH="{cplex_path}/cplex/bin/x86-64_linux:{cplex_path}/cplex/lib/x86-64_linux/static_pic:$LD_LIBRARY_PATH"

# Executar CPLEX com os argumentos fornecidos
cplex-env {cplex_exec} "$@"
""")
        
        # Tornar executável
        os.chmod(wrapper_path, 0o755)
        print(f"Wrapper CPLEX criado em: {wrapper_path}")
        return wrapper_path
    except Exception as e:
        print(f"Erro ao criar wrapper: {str(e)}")
        return None

def run_cplex_with_nix_shell(cplex_path, args=None):
    """
    Executa o CPLEX usando steam-run para criar um ambiente FHS no NixOS.
    
    Args:
        cplex_path: Caminho para o diretório do CPLEX
        args: Lista de argumentos para o CPLEX
        
    Returns:
        tuple: (returncode, stdout, stderr)
    """
    try:
        cplex_exec = os.path.join(cplex_path, "cplex/bin/x86-64_linux/cplex")
        
        # Verificar se o executável existe
        if not os.path.exists(cplex_exec):
            return 1, "", f"Executável CPLEX não encontrado em: {cplex_exec}"
        
        # Criar script em um local mais acessível ao steam-run (diretório atual)
        script_path = os.path.join(os.getcwd(), f"cplex_cmd_{int(time.time())}.sh")
        
        # Criar conteúdo do script
        with open(script_path, 'w') as f:
            f.write(f"""#!/bin/sh
export LD_LIBRARY_PATH="{cplex_path}/cplex/bin/x86-64_linux:{cplex_path}/cplex/lib/x86-64_linux/static_pic:$LD_LIBRARY_PATH"
{cplex_exec} {' '.join(args) if args else ''}
""")
        
        # Tornar o script executável
        os.chmod(script_path, 0o755)
        
        # Verificar se steam-run está disponível
        steam_run_available = False
        try:
            subprocess.run(['which', 'steam-run'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            steam_run_available = True
        except:
            pass
        
        # Tentar primeiro o método padrão do PuLP para executar CPLEX
        try:
            print("Tentando executar CPLEX diretamente via PuLP no NixOS...")
            import pulp
            solver = pulp.CPLEX_CMD(path=cplex_exec)
            if solver.available():
                print("CPLEX disponível via PuLP!")
                return 0, "CPLEX disponível via PuLP", ""
        except Exception as e:
            print(f"PuLP não conseguiu executar CPLEX: {str(e)}")
        
        # Executar o script usando steam-run ou outro método FHS
        result = None
        if steam_run_available:
            print(f"Executando CPLEX via steam-run (ambiente FHS completo)...")
            result = subprocess.run(['steam-run', script_path], 
                                    capture_output=True, text=True)
        else:
            # Método alternativo: cria um script de shell mais simples
            with open("run_cplex.sh", "w") as f:
                f.write(f"""#!/bin/bash
# Script para executar CPLEX em ambiente NixOS
export LD_LIBRARY_PATH="{cplex_path}/cplex/bin/x86-64_linux:{cplex_path}/cplex/lib/x86-64_linux/static_pic:$LD_LIBRARY_PATH"
echo "Tentando executar: {cplex_exec} {' '.join(args) if args else ''}"
{cplex_exec} {' '.join(args) if args else ''} || echo "Falha na execução do CPLEX"
""")
            os.chmod("run_cplex.sh", 0o755)
            
            print("steam-run não encontrado, tentando método alternativo...")
            result = subprocess.run(["bash", "run_cplex.sh"], 
                                   capture_output=True, text=True)
            try:
                os.unlink("run_cplex.sh")
            except:
                pass
        
        # Limpar arquivo temporário do script
        try:
            os.unlink(script_path)
        except Exception as e:
            print(f"Aviso: Não foi possível remover script temporário: {str(e)}")
        
        if result:
            return result.returncode, result.stdout, result.stderr
        else:
            return 1, "", "Nenhum método de execução do CPLEX funcionou"
    except Exception as e:
        print(f"Erro ao executar CPLEX com ambiente FHS: {str(e)}")
        return 1, "", str(e)

def main():
    """Função principal."""
    if len(sys.argv) < 2:
        print("Uso: nixos_cplex_wrapper.py <caminho_do_cplex> [argumentos_do_cplex]")
        sys.exit(1)
    
    cplex_path = sys.argv[1]
    args = sys.argv[2:] if len(sys.argv) > 2 else []
    
    if detect_nixos():
        print("Ambiente NixOS detectado, usando método alternativo.")
        returncode, stdout, stderr = run_cplex_with_nix_shell(cplex_path, args)
        print(stdout)
        if stderr:
            print(stderr, file=sys.stderr)
        sys.exit(returncode)
    else:
        # Em outras distribuições, apenas executa o CPLEX normalmente
        cplex_exec = os.path.join(cplex_path, "cplex", "bin", "x86-64_linux", "cplex")
        if not os.path.exists(cplex_exec):
            print(f"CPLEX não encontrado em: {cplex_exec}")
            sys.exit(1)
        os.execv(cplex_exec, [cplex_exec] + args)

if __name__ == "__main__":
    main()