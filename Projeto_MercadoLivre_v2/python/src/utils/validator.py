import os
import glob
from prettytable import PrettyTable
from src.models.problem import WaveOrderPickingProblem
from src.models.solution import WaveOrderPickingSolution

class SolutionValidator:
    """Centraliza funções de validação, análise de soluções e instâncias."""
    
    @staticmethod
    def validate_solution(problem, solution):
        """
        Verifica se uma solução é viável.
        
        Args:
            problem (WaveOrderPickingProblem): O problema
            solution (WaveOrderPickingSolution): A solução
            
        Returns:
            bool: True se a solução for viável, False caso contrário
        """
        # Verificar se o total de unidades está dentro dos limites
        total_units = solution.total_units
        if total_units < problem.wave_size_lb or total_units > problem.wave_size_ub:
            return False
        
        # Verificar cobertura de itens
        items_needed = {}
        for o in solution.selected_orders:
            for item, quantity in problem.orders[o].items():
                items_needed[item] = items_needed.get(item, 0) + quantity
        
        items_available = {}
        for a in solution.visited_aisles:
            for item, quantity in problem.aisles[a].items():
                items_available[item] = items_available.get(item, 0) + quantity
        
        # Cada item necessário deve ter unidades suficientes disponíveis
        for item, quantity_needed in items_needed.items():
            if quantity_needed > items_available.get(item, 0):
                return False
        
        return True
    
    @staticmethod
    def calculate_objective(solution):
        """
        Calcula o valor da função objetivo (produtividade).
        
        Args:
            solution (WaveOrderPickingSolution): A solução
            
        Returns:
            float: Valor da função objetivo
        """
        if not solution.visited_aisles:
            return 0.0
        return solution.total_units / len(solution.visited_aisles)
    
    @staticmethod
    def read_solution_file(file_path):
        """Lê um arquivo de solução e retorna as listas de pedidos e corredores."""
        selected_orders = []
        visited_aisles = []
        
        with open(file_path, 'r') as file:
            lines = file.readlines()
            
            # Ler número de pedidos
            num_orders = int(lines[0].strip())
            
            # Ler pedidos
            for i in range(1, num_orders + 1):
                selected_orders.append(int(lines[i].strip()))
            
            # Ler número de corredores
            num_aisles = int(lines[num_orders + 1].strip())
            
            # Ler corredores
            for i in range(num_orders + 2, num_orders + 2 + num_aisles):
                visited_aisles.append(int(lines[i].strip()))
        
        return selected_orders, visited_aisles
    
    @staticmethod
    def validate_solution_file(problem_file, solution_file, config=None):
        """
        Valida um arquivo de solução contra um problema.
        
        Args:
            problem_file (str): Caminho para o arquivo do problema
            solution_file (str): Caminho para o arquivo da solução
            config (dict, optional): Configuração da aplicação.
            
        Returns:
            tuple: (é_viável, valor_objetivo, mensagem)
        """
        # Carregar o problema
        problem = WaveOrderPickingProblem(config=config)
        problem.read_input(problem_file)
        
        # Ler a solução
        try:
            selected_orders, visited_aisles = SolutionValidator.read_solution_file(solution_file)
        except Exception as e:
            return False, 0, f"Erro ao ler o arquivo de solução: {str(e)}"
        
        # Criar o objeto solução
        solution = problem.create_solution(selected_orders, visited_aisles)
        
        # Validar
        is_feasible = SolutionValidator.validate_solution(problem, solution)
        objective_value = SolutionValidator.calculate_objective(solution) if is_feasible else 0
        
        message = "Solução viável." if is_feasible else "Solução inviável!"
        
        return is_feasible, objective_value, message
    
    
    @staticmethod
    def analyze_instance(file_path, config=None):
        """
        Analisa um arquivo de instância e retorna estatísticas.
        
        Args:
            file_path (str): Caminho para o arquivo de instância
            config (dict, optional): Configuração da aplicação.
            
        Returns:
            dict: Resumo das estatísticas da instância
        """
        problem = WaveOrderPickingProblem(config=config)
        problem.read_input(file_path)
        
        # Calcular estatísticas
        order_sizes = [len(order) for order in problem.orders.values()]
        aisle_sizes = [len(aisle) for aisle in problem.aisles.values()]
        
        return {
            "file_name": os.path.basename(file_path),
            "n_orders": problem.n_orders,
            "n_items": problem.n_items,
            "n_aisles": problem.n_aisles,
            "wave_size_lb": problem.wave_size_lb,
            "wave_size_ub": problem.wave_size_ub,
            "avg_items_per_order": sum(order_sizes) / len(order_sizes) if order_sizes else 0,
            "avg_items_per_aisle": sum(aisle_sizes) / len(aisle_sizes) if aisle_sizes else 0,
            "total_order_units": sum(problem.order_units.values()),
            "orders": problem.orders,
            "aisles": problem.aisles
        }
    
    @staticmethod
    def display_instance_brief(instance_data):
        """
        Exibe um resumo formatado da instância.
        
        Args:
            instance_data (dict): Dados da instância obtidos com analyze_instance
        """
        table = PrettyTable()
        table.field_names = ["Característica", "Valor"]
        table.align["Característica"] = "l"
        table.align["Valor"] = "r"
        
        table.add_row(["Arquivo", instance_data["file_name"]])
        table.add_row(["Pedidos", instance_data["n_orders"]])
        table.add_row(["Tipos de itens", instance_data["n_items"]])
        table.add_row(["Corredores", instance_data["n_aisles"]])
        table.add_row(["Limite inferior (LB)", instance_data["wave_size_lb"]])
        table.add_row(["Limite superior (UB)", instance_data["wave_size_ub"]])
        table.add_row(["Média de itens por pedido", f"{instance_data['avg_items_per_order']:.2f}"])
        table.add_row(["Média de itens por corredor", f"{instance_data['avg_items_per_aisle']:.2f}"])
        table.add_row(["Total de unidades nos pedidos", instance_data["total_order_units"]])
        
        print(table)