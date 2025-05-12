import os
import json
import configparser
from pathlib import Path
import numpy as np

DEFAULT_CONFIG_PATH = "config.ini"

def load_config(config_path=DEFAULT_CONFIG_PATH):
    """
    Carrega configurações do arquivo especificado ou cria um arquivo com configurações padrão.
    
    Args:
        config_path (str): Caminho para o arquivo de configuração
        
    Returns:
        dict: Dicionário contendo todas as configurações
    """
    config = {}
    
    if os.path.exists(config_path):
        print(f"Carregando configurações de: {config_path}")
        
        if config_path.endswith('.json'):
            with open(config_path, 'r', encoding='utf-8') as file:
                config = json.load(file)
        else:
            parser = configparser.ConfigParser()
            parser.read(config_path, encoding='utf-8')
            
            # Converter o configparser para dicionário
            for section in parser.sections():
                config[section] = {}
                for key, value in parser[section].items():
                    # Processar listas de valores (separados por vírgula)
                    if ',' in value and not (value.startswith('"') or value.startswith("'")):
                        # Se parece ser uma lista de números
                        if all(item.strip().replace('.', '', 1).isdigit() or 
                               item.strip().startswith('-') and item.strip()[1:].replace('.', '', 1).isdigit() 
                               for item in value.split(',')):
                            # Converter para lista de números
                            if any('.' in item for item in value.split(',')):
                                config[section][key] = [float(item.strip()) for item in value.split(',')]
                            else:
                                config[section][key] = [int(item.strip()) for item in value.split(',')]
                        else:
                            # Lista de strings
                            config[section][key] = [item.strip() for item in value.split(',')]
                    # Converter valores para int, float ou bool quando possível
                    else:
                        try:
                            if value.lower() in ('true', 'false'):
                                config[section][key] = parser.getboolean(section, key)
                            elif '.' in value and value.replace('.', '', 1).replace('-', '', 1).isdigit():
                                config[section][key] = parser.getfloat(section, key)
                            elif value.replace('-', '', 1).isdigit():
                                config[section][key] = parser.getint(section, key)
                            else:
                                config[section][key] = _clean_value(value)
                        except:
                            config[section][key] = _clean_value(value)
    else:
        print(f"Arquivo de configuração não encontrado. Criando configuração padrão em: {config_path}")
        config = create_default_config(config_path)
    
    # Processar configurações especiais após o carregamento
    config = post_process_config(config)
    
    return config

def post_process_config(config):
    """
    Processa configurações especiais após o carregamento.
    """
    
    # Verificar se estamos em um ambiente NixOS
    is_nixos = False
    try:
        if os.path.exists("/etc/os-release"):
            with open("/etc/os-release", "r") as f:
                is_nixos = "NixOS" in f.read()
    except:
        pass
    
    # Configuração automática do CPLEX se não estiver definido
    if 'cplex' not in config:
        config['cplex'] = {}
    
    if 'path' not in config['cplex']:
        # Verificar o caminho recém-instalado
        cplex_path = "/opt/ibm/ILOG/CPLEX_Studio2212"
        if os.path.exists(cplex_path):
            config['cplex']['path'] = cplex_path
            print(f"CPLEX encontrado automaticamente em: {cplex_path}")
    
    # Se está definido o caminho CPLEX mas não o modo de aceleração, definir GPU como padrão
    if 'path' in config['cplex'] and 'acceleration' not in config['cplex']:
        config['cplex']['acceleration'] = 'gpu'
        config['cplex']['num_threads'] = 0  # Auto
        config['cplex']['parallel_mode'] = 1  # Oportunista
    
    # Se estamos no NixOS, adicionar flag para indicar isso
    if is_nixos:
        config['cplex']['nixos'] = True
    
    # Definir CPLEX como solver padrão na seção algorithm
    if 'algorithm' not in config:
        config['algorithm'] = {}
    
    # Usar CPLEX_GPU como padrão se disponível
    if 'path' in config['cplex'] and os.path.exists(config['cplex']['path']):
        config['algorithm']['solver'] = 'CPLEX_GPU'
    
    return config

def create_default_config(config_path):
    """
    Cria um arquivo de configuração padrão no caminho especificado.
    
    Args:
        config_path (str): Caminho onde o arquivo será criado
        
    Returns:
        dict: Configurações padrão
    """
    # Configurações padrão extensas
    default_config = {
        "paths": {
            "instances_dir": "datasets",
            "results_dir": "results",
            "logs_dir": "logs"
        },
        "algorithm": {
            "max_runtime": 600,
            "strategy": "greedy",
            "seed": 42,
            "use_pulp": True,
            "model_type": "mip",
            "solver": "CBC"
        },
        "constraints": {
            "wave_size_min_factor": 1.0,
            "wave_size_max_factor": 1.0,
            "enforce_full_orders": True,
            "soft_constraints": True
        },
        "objective": {
            "productivity_weight": 1.0,
            "order_count_weight": 0.0,
            "normalize": True,
            "objective_method": "linearized"
        },
        "model": {
            "formulation": "lagrangian_augmented",
            "bigM": 500,
            "optimize_timeout": 30
        },
        "linearization": {
            "inverse_corr_var": True,
            "use_mccormick": True,
            "fractional_approximation": "exact"
        },
        "lagrangian": {
            "enabled": True,
            "rho_initial": 10.0,
            "lambda_update_factor": 1.2,
            "max_iterations": 20,
            "convergence_tolerance": 0.001,
            "lambda_lb_initial": 0.0,
            "lambda_ub_initial": 0.0,
            "lambda_coverage_initial": 0.0
        },
        "penalties": {
            "lb_penalty": 1000.0,
            "ub_penalty": 1000.0,
            "item_coverage_penalty": 1000.0,
            "linearization_penalty": 500.0
        },
        "piecewise": {
            "enabled": True,
            "num_breakpoints": 5,
            "max_violation": 10,
            "scaling_factor": 100,
            "lb_breakpoints": [0, 2, 5, 8, 10],
            "ub_breakpoints": [0, 2, 5, 8, 10],
            "coverage_breakpoints": [0, 1, 2, 3, 5]
        },
        "augmented_objective": {
            "use_original_term": True,
            "use_lagrange_terms": True,
            "use_piecewise_terms": True,
            "dynamic_penalties": True
        },
        "heuristic": {
            "construction_method": "greedy",
            "local_search": True,
            "improvement_threshold": 0.01,
            "max_iterations": 10000,
            "time_limit_percent": 80
        },
        "advanced_linearization": {
            "use_disaggregated_mccormick": False,
            "use_perspective_formulation": False,
            "use_sos2_variables": False,
            "lambda_fixing_heuristic": True
        }
    }
    
    # Criar diretório pai se não existir
    directory = os.path.dirname(config_path)
    if directory and not os.path.exists(directory):
        os.makedirs(directory)
    
    # Salvar o arquivo no formato apropriado
    if config_path.endswith('.json'):
        with open(config_path, 'w', encoding='utf-8') as file:
            json.dump(default_config, file, indent=4)
    else:
        parser = configparser.ConfigParser()
        
        for section, items in default_config.items():
            parser[section] = {}
            for key, value in items.items():
                # Converter listas para strings com vírgulas
                if isinstance(value, list):
                    parser[section][key] = ','.join(str(item) for item in value)
                else:
                    parser[section][key] = str(value)
        
        with open(config_path, 'w', encoding='utf-8') as file:
            parser.write(file)
    
    return default_config

def get_model_parameters(config):
    """
    Extrai parâmetros específicos do modelo matemático a partir da configuração.
    Converte em estruturas de dados adequadas para uso no solver.
    
    Args:
        config (dict): Configuração completa
        
    Returns:
        dict: Parâmetros do modelo organizados para uso direto
    """
    model_params = {
        # Parâmetros básicos do modelo
        "use_soft_constraints": config["constraints"]["soft_constraints"],
        "big_m": config["model"]["bigM"],
        
        # Configurações de linearização
        "linearization": {
            "use_inverse_var": config["linearization"]["inverse_corr_var"],
            "use_mccormick": config["linearization"]["use_mccormick"],
            "method": config["linearization"]["fractional_approximation"]
        },
        
        # Parâmetros do método do Lagrangeano Aumentado
        "lagrangian": {
            "enabled": config["lagrangian"]["enabled"],
            "rho": config["lagrangian"]["rho_initial"],
            "update_factor": config["lagrangian"]["lambda_update_factor"],
            "max_iter": config["lagrangian"]["max_iterations"],
            "convergence_tol": config["lagrangian"]["convergence_tolerance"],
            "initial_multipliers": {
                "lb": config["lagrangian"]["lambda_lb_initial"],
                "ub": config["lagrangian"]["lambda_ub_initial"],
                "coverage": config["lagrangian"]["lambda_coverage_initial"]
            }
        },
        
        # Configurações para penalidades
        "penalties": {
            "lb": config["penalties"]["lb_penalty"],
            "ub": config["penalties"]["ub_penalty"],
            "coverage": config["penalties"]["item_coverage_penalty"],
            "linearization": config["penalties"]["linearization_penalty"]
        },
        
        # Configurações para linearização por partes
        "piecewise": {
            "enabled": config["piecewise"]["enabled"],
            "breakpoints": {
                "lb": config["piecewise"]["lb_breakpoints"],
                "ub": config["piecewise"]["ub_breakpoints"],
                "coverage": config["piecewise"]["coverage_breakpoints"]
            },
            "scaling": config["piecewise"]["scaling_factor"]
        }
    }
    
    return model_params

def _clean_value(value_str):
    """Remove comentários de valores e converte para o tipo apropriado."""
    # Remover comentários (tudo após o primeiro ponto e vírgula sem aspas)
    if ';' in value_str and not (value_str.startswith('"') or value_str.startswith("'")):
        value_str = value_str.split(';')[0].strip()
    
    # Converter para o tipo apropriado
    try:
        # Tentar converter para int
        return int(value_str)
    except ValueError:
        try:
            # Tentar converter para float
            return float(value_str)
        except ValueError:
            # É um booleano?
            if value_str.lower() in ('true', 'yes', 'on', '1'):
                return True
            elif value_str.lower() in ('false', 'no', 'off', '0'):
                return False
            # Manter como string
            return value_str