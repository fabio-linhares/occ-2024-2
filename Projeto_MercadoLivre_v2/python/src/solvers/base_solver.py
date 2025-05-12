from abc import ABC, abstractmethod
import time
from src.utils.validator import SolutionValidator

class BaseSolver(ABC):
    """
    Classe base abstrata para solvers do problema de Wave Order Picking.
    """
    
    def __init__(self, problem, config=None):
        """
        Inicializa o solver com um problema e configurações.
        
        Args:
            problem: O problema a ser resolvido
            config (dict, optional): Configurações para o solver
        """
        self.problem = problem
        self.config = config or {}
        self.max_runtime = self.config.get("algorithm", {}).get("max_runtime", 600)  # 10 minutos por padrão
    
    @abstractmethod
    def solve(self, start_time=None):
        """
        Método abstrato para resolver o problema.
        
        Args:
            start_time (float, optional): Tempo de início para controle de timeout
            
        Returns:
            WaveOrderPickingSolution: A solução encontrada
        """
        pass
    
    def get_remaining_time(self, start_time):
        """
        Calcula o tempo restante para execução.
        
        Args:
            start_time (float): Tempo de início da execução
            
        Returns:
            float: Tempo restante em segundos
        """
        elapsed = time.time() - start_time
        return max(self.max_runtime - elapsed, 1)  # pelo menos 1 segundo
    
    def check_timeout(self, start_time):
        """
        Verifica se o tempo de execução excedeu o limite.
        
        Args:
            start_time (float): Tempo de início da execução
            
        Returns:
            bool: True se o tempo excedeu o limite, False caso contrário
        """
        return self.get_remaining_time(start_time) <= 0
    
    # Métodos que agora usam o SolutionValidator
    def _is_solution_feasible(self, solution):
        """Verifica se uma solução é viável usando SolutionValidator."""
        return SolutionValidator.validate_solution(self.problem, solution)
    
    def _compute_objective_function(self, solution):
        """Calcula o valor da função objetivo usando SolutionValidator."""
        return SolutionValidator.calculate_objective(solution)