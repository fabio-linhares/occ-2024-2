import logging
import os
import sys
import time
import random
import numpy as np
import traceback

logging.basicConfig(level=logging.INFO, 
                   format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

from src.solvers.pli.pli_solver import PLISolver
from src.solvers.heuristic.greedy_solver import GreedySolver
from src.solvers.heuristic.ils_solver import ILSSolver

class SolverFactory:
    """Fábrica para criar instâncias de solvers apropriados."""
    
    @staticmethod
    def create_solver(problem, method_or_config=None, config=None):
        """
        Cria um solver apropriado para o problema.
        """
        # Verificar o padrão de chamada usado
        if isinstance(method_or_config, str):
            # Padrão 1: create_solver(problem, method, config)
            method = method_or_config
            config = config or {}
        else:
            # Padrão 2: create_solver(problem, config)
            config = method_or_config or {}
            # Verificar se há uma estratégia explicitamente selecionada
            strategy = config.get('algorithm', {}).get('strategy', 'auto')
            solver_type = config.get('algorithm', {}).get('solver', 'auto')
            
            # Usar a estratégia explícita se fornecida, senão usar o tipo de solver
            if strategy == 'heuristic':
                # Quando é heurística, verificar qual método específico foi configurado
                meta_config = config.get('meta_heuristic', {})
                meta_method = meta_config.get('method', 'ils')
                
                # Usar o método meta-heurístico configurado
                if meta_method in ['ils', 'simulated_annealing', 'grasp']:
                    method = meta_method
                else:
                    method = 'ils'  # ILS como padrão para meta-heurística
                    
                logger.info(f"Usando meta-heurística: {method}")
            elif strategy == 'pli':
                method = solver_type  # Usa o solver configurado para PLI
            else:
                # Lógica de seleção automática existente
                meta_config = config.get('meta_heuristic', {})
                method = solver_type
                
                # Para instâncias muito grandes, usar ILS
                if solver_type == 'auto':
                    if problem.n_orders <= 50:
                        method = 'pli'
                    elif problem.n_orders <= 200:
                        method = 'greedy_vnd'
                    else:
                        method = 'ils'
        
        # Mapear solvers comerciais/externos para o método apropriado
        solver_to_method = {
            'CPLEX': 'pli',
            'CPLEX_GPU': 'pli',
            'CBC': 'pli',
            'GLPK': 'pli',
            'GUROBI': 'pli',
            'HIGHS': 'pli'
        }
        
        # Se o método for um nome de solver, mapear para o método correto
        if method in solver_to_method:
            actual_method = solver_to_method[method]
            logger.info(f"Usando método {actual_method} com solver {method}")
            method = actual_method
        
        # Criar o solver apropriado
        if method == 'pli':
            logger.info(f"Criando solver PLI...")
            # Para PLI, tentar usar heurística inicial melhorada
            greedy = GreedySolver(problem, config)
            initial_solution = greedy.solve()
            
            # Se a heurística encontrou solução viável, usar como warmstart
            pli_solver = PLISolver(problem, config)
            if initial_solution and initial_solution.is_feasible:
                pli_solver.initial_solution = initial_solution
                
            return pli_solver
            
        elif method == 'ils':
            logger.info("Criando solver ILS (meta-heurística)...")
            return ILSSolver(problem, config)
            
        elif method == 'simulated_annealing':
            logger.info("Criando solver Simulated Annealing...")
            # Verificar se o módulo SimulatedAnnealing existe, caso contrário usar ILS
            try:
                from src.solvers.heuristic.simulated_annealing import SimulatedAnnealingSolver
                return SimulatedAnnealingSolver(problem, config)
            except ImportError:
                logger.warning("Solver Simulated Annealing não encontrado, usando ILS como alternativa")
                return ILSSolver(problem, config)
                
        elif method == 'grasp':
            logger.info("Criando solver GRASP...")
            # Verificar se o módulo GRASP existe, caso contrário usar ILS
            try:
                # GRASP ainda não implementado, usando ILS como fallback
                logger.warning("Solver GRASP não implementado ainda")
                logger.info("Usando ILS como alternativa")
                return ILSSolver(problem, config)
                # Descomentar quando a implementação estiver disponível:
                # from src.solvers.heuristic.grasp_solver import GRASPSolver
                # return GRASPSolver(problem, config)
            except ImportError:
                logger.warning("Solver GRASP não encontrado, usando ILS como alternativa")
                return ILSSolver(problem, config)
            
        elif method == 'greedy_vnd':
            logger.info("Criando solver Greedy+VND...")
            from src.solvers.heuristic.vnd import VND
            greedy = GreedySolver(problem, config)
            initial_solution = greedy.solve()
            
            if initial_solution and initial_solution.is_feasible:
                vnd = VND(problem, config)
                # Criar uma função que encapsula a busca VND
                def vnd_solver():
                    return vnd.search(initial_solution)
                return vnd_solver
            else:
                return ILSSolver(problem, config)
                
        elif method == 'heuristic' or method == 'greedy':
            # Verificar se devemos usar uma meta-heurística ao invés
            meta_config = config.get('meta_heuristic', {})
            meta_method = meta_config.get('method', '')
            
            # Se tiver um método de meta-heurística definido, usar em vez do guloso
            if meta_method == 'ils':
                logger.info("Redirecionando para solver ILS...")
                return ILSSolver(problem, config)
            elif meta_method in ['simulated_annealing', 'grasp']:
                logger.info(f"Redirecionando para solver {meta_method}...")
                try:
                    if meta_method == 'simulated_annealing':
                        from src.solvers.heuristic.simulated_annealing import SimulatedAnnealingSolver
                        return SimulatedAnnealingSolver(problem, config)
                    else:
                        # GRASP solver not implemented yet, using ILS as fallback
                        logger.warning("Solver GRASP não implementado ainda")
                        logger.info("Usando ILS como alternativa")
                        return ILSSolver(problem, config)
                except ImportError:
                    logger.warning(f"Solver {meta_method} não encontrado, usando ILS")
                    return ILSSolver(problem, config)
            
            # Se chegou aqui, usar o GreedySolver padrão
            logger.info("Criando solver Heurístico Guloso...")
            return GreedySolver(problem, config)
            
        else:
            logger.error(f"ERRO: Método desconhecido: {method}, usando ILS como fallback")
            return ILSSolver(problem, config)
    
    @staticmethod
    def compute_objective_function(solution):
        """Calcula o valor da função objetivo."""
        if not solution.visited_aisles:
            return 0.0
        if len(solution.visited_aisles) == 0:  # Verificação adicional
            return 0.0
        try:
            # Operações matemáticas críticas
            result = solution.total_units / len(solution.visited_aisles)
        except (ArithmeticError, ValueError) as e:
            logger.error(f"Erro matemático: {str(e)}")
            # Fornecer um valor padrão seguro
            result = 0
        return result