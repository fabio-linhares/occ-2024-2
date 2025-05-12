import time
from src.solvers.heuristic.local_search import LocalSearch

class VND:
    """Implementação de Variable Neighborhood Descent (VND) com GPU opcional."""
    
    def __init__(self, problem, config=None):
        """Inicializa o VND."""
        self.problem = problem
        self.config = config or {}
        self.local_search = LocalSearch(problem, config)
        
        # Configuração para debug e logging
        self.debug = self.config.get('meta_heuristic', {}).get('debug', False)
    
    def search(self, initial_solution, max_iterations=100):
        """Executa a busca VND a partir de uma solução inicial."""
        # Verificar se a solução inicial é viável
        if not initial_solution.is_feasible:
            if self.debug:
                print("VND iniciado com solução inviável. Tentando reparar...")
            
            # Tentar reparar a solução inicial
            initial_solution = self._repair_solution(initial_solution)
            
            if not initial_solution.is_feasible:
                if self.debug:
                    print("Não foi possível reparar a solução inicial para VND.")
                return initial_solution
        
        current_solution = initial_solution
        best_solution = initial_solution
        
        # Definir estruturas de vizinhança em ordem de complexidade crescente
        neighborhoods = [
            (self.local_search.swap_neighborhood, "Swap"),
            (self.local_search.insert_neighborhood, "Insert"),
            (self.local_search.remove_neighborhood, "Remove"),
            (self.local_search.k_swap_neighborhood, "K-Swap"),
            (self.local_search.aisle_based_neighborhood, "Aisle-Based")
        ]
        
        iteration = 0
        k = 0  # Índice da vizinhança atual
        
        start_time = time.time()
        
        while k < len(neighborhoods) and iteration < max_iterations:
            # Obter a função de vizinhança atual e seu nome
            neighborhood_function, neighborhood_name = neighborhoods[k]
            
            # Explorar a vizinhança k
            if neighborhood_name == "K-Swap":
                # Para k-swap, passamos um parâmetro k adicional
                new_solution = neighborhood_function(current_solution, k=2)
            else:
                new_solution = neighborhood_function(current_solution)
            
            # Verificar se houve melhoria
            if new_solution.objective_value > current_solution.objective_value:
                # Melhoria encontrada, atualizar solução atual e voltar à primeira vizinhança
                if self.debug:
                    improvement = new_solution.objective_value - current_solution.objective_value
                    print(f"VND Iter {iteration}: Melhoria na vizinhança {neighborhood_name}: +{improvement:.4f}")
                
                current_solution = new_solution
                k = 0  # Reiniciar da primeira vizinhança
                
                # Atualizar melhor solução se necessário
                if current_solution.objective_value > best_solution.objective_value:
                    best_solution = current_solution
            else:
                # Sem melhoria, avançar para próxima vizinhança
                k += 1
                if self.debug:
                    print(f"VND Iter {iteration}: Sem melhoria na vizinhança {neighborhood_name}, avançando para k={k}")
            
            iteration += 1
            
            # Verificar tempo limite
            if self.config.get('algorithm', {}).get('max_runtime', 0) > 0:
                elapsed = time.time() - start_time
                if elapsed > self.config['algorithm']['max_runtime']:
                    if self.debug:
                        print(f"VND interrompido por tempo limite após {iteration} iterações.")
                    break
        
        if self.debug:
            print(f"VND concluído após {iteration} iterações. Melhor valor: {best_solution.objective_value:.4f}")
        
        return best_solution
    
    def _repair_solution(self, solution):
        """Tenta reparar uma solução inviável."""
        # Se a solução já é viável, não há o que reparar
        if solution.is_feasible:
            return solution
        
        # Problema com o número total de unidades
        total_units = solution.total_units
        
        if total_units < self.problem.wave_size_lb:
            # Adicionar mais pedidos para atingir o limite inferior
            available_orders = set(range(self.problem.n_orders)) - set(solution.selected_orders)
            
            # Ordenar pedidos por unidades (decrescente)
            orders_by_units = [(o, self.problem.order_units.get(o, 0)) 
                              for o in available_orders]
            orders_by_units.sort(key=lambda x: x[1], reverse=True)
            
            new_selected = list(solution.selected_orders)
            
            for order_id, units in orders_by_units:
                if total_units >= self.problem.wave_size_lb:
                    break
                    
                new_selected.append(order_id)
                total_units += units
            
            # Recalcular corredores necessários usando GPU se disponível
            if self.local_search.use_gpu and hasattr(self.local_search, 'gpu_manager'):
                new_visited = self.local_search._calculate_required_aisles_gpu(new_selected)
            else:
                new_visited = self.local_search._calculate_required_aisles(new_selected)
            
            # Criar nova solução
            new_solution = self.problem.create_solution(new_selected, new_visited)
            new_solution.is_feasible = self.local_search._is_solution_feasible(new_solution)
            
            if new_solution.is_feasible:
                new_solution.objective_value = self.local_search._compute_objective_function(new_solution)
                return new_solution
        
        elif total_units > self.problem.wave_size_ub:
            # Remover pedidos para ficar abaixo do limite superior
            selected_orders = list(solution.selected_orders)
            
            # Ordenar pedidos por unidades (crescente)
            orders_by_units = [(o, self.problem.order_units.get(o, 0)) 
                             for o in selected_orders]
            orders_by_units.sort(key=lambda x: x[1])
            
            new_selected = list(selected_orders)
            
            for order_id, units in orders_by_units:
                new_selected.remove(order_id)
                total_units -= units
                
                if total_units <= self.problem.wave_size_ub:
                    # Verificar se ainda está acima do limite inferior
                    if total_units >= self.problem.wave_size_lb:
                        break
            
            # Se removemos pedidos demais, adicionar alguns de volta
            if total_units < self.problem.wave_size_lb:
                for order_id, units in reversed(orders_by_units):
                    if order_id not in new_selected:
                        if total_units + units <= self.problem.wave_size_ub:
                            new_selected.append(order_id)
                            total_units += units
                            
                            if total_units >= self.problem.wave_size_lb:
                                break
            
            # Recalcular corredores necessários
            new_visited = self.local_search._calculate_required_aisles(new_selected)
            
            # Criar nova solução
            new_solution = self.problem.create_solution(new_selected, new_visited)
            new_solution.is_feasible = self.local_search._is_solution_feasible(new_solution)
            
            if new_solution.is_feasible:
                new_solution.objective_value = self.local_search._compute_objective_function(new_solution)
                return new_solution
        
        # Se chegamos aqui, a solução não pôde ser reparada
        return solution