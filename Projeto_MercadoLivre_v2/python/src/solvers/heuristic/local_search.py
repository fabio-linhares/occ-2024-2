import time
import numpy as np
import cupy as cp

class LocalSearch:
    """Framework para busca local com aceleração GPU opcional."""
    
    def __init__(self, problem, config=None):
        """Inicializa o framework de busca local."""
        self.problem = problem
        self.config = config or {}
        self.use_gpu = self.config.get('algorithm', {}).get('use_gpu', False)
        self.gpu_manager = None
        
        # Inicializar GPU Manager se necessário
        if self.use_gpu:
            try:
                from src.utils.gpu_manager import GPUManager
                self.gpu_manager = GPUManager(problem)
                self.gpu_manager.initialize()
                print("GPU habilitada para busca local")
            except Exception as e:
                print(f"Erro ao inicializar GPU: {str(e)}")
                self.use_gpu = False
    
    def _calculate_required_aisles(self, selected_orders):
        """Calcula os corredores necessários para atender aos pedidos selecionados."""
        # Usar aceleração GPU se disponível
        if self.use_gpu and self.gpu_manager:
            try:
                return self._calculate_required_aisles_gpu(selected_orders)
            except Exception as e:
                print(f"Erro ao calcular corredores com GPU: {str(e)}")
                # Fallback para CPU

        # Implementação CPU (fallback ou padrão)
        needed_items = {}
        for o_id in selected_orders:
            for item_id, quantity in self.problem.orders.get(o_id, {}).items():
                needed_items[item_id] = needed_items.get(item_id, 0) + quantity
        
        # Encontrar corredores que contêm os itens necessários
        required_aisles = set()
        item_coverage = {item_id: 0 for item_id in needed_items}
        
        # Primeiro, selecionar corredores com itens exclusivos
        for aisle_id in range(self.problem.n_aisles):
            aisle_items = self.problem.aisles.get(aisle_id, {})
            exclusive_items = set()
            
            for item_id in aisle_items:
                if item_id in needed_items:
                    # Verificar se o item só aparece neste corredor
                    if len(self.problem.item_units_by_aisle.get(item_id, {})) == 1:
                        exclusive_items.add(item_id)
            
            if exclusive_items:
                required_aisles.add(aisle_id)
                for item_id in exclusive_items:
                    item_coverage[item_id] += self.problem.aisles[aisle_id].get(item_id, 0)
        
        # Selecionar mais corredores até cobrir todos os itens
        uncovered_items = [item_id for item_id, coverage in item_coverage.items() 
                           if coverage < needed_items.get(item_id, 0)]
        
        while uncovered_items:
            best_aisle = -1
            best_coverage = 0
            
            for aisle_id in range(self.problem.n_aisles):
                if aisle_id in required_aisles:
                    continue
                
                # Calcular quantos itens adicionais este corredor cobre
                aisle_items = self.problem.aisles.get(aisle_id, {})
                additional_coverage = 0
                
                for item_id in uncovered_items:
                    if item_id in aisle_items:
                        additional_coverage += min(
                            needed_items[item_id] - item_coverage[item_id],
                            aisle_items[item_id]
                        )
                
                if additional_coverage > best_coverage:
                    best_coverage = additional_coverage
                    best_aisle = aisle_id
            
            if best_aisle == -1 or best_coverage == 0:
                break  # Não há mais corredores que possam melhorar a cobertura
            
            required_aisles.add(best_aisle)
            
            # Atualizar cobertura
            for item_id in uncovered_items.copy():
                if item_id in self.problem.aisles.get(best_aisle, {}):
                    item_coverage[item_id] += self.problem.aisles[best_aisle][item_id]
                    if item_coverage[item_id] >= needed_items[item_id]:
                        uncovered_items.remove(item_id)
        
        return list(required_aisles)

    def _calculate_required_aisles_gpu(self, selected_orders):
        """Versão GPU do cálculo de corredores necessários."""
        import numpy as np
        import cupy as cp
        
        # Verificar que temos o gerenciador GPU
        if not hasattr(self, 'gpu_manager') or not self.gpu_manager:
            from src.utils.gpu_manager import GPUManager
            self.gpu_manager = GPUManager(self.problem)
            self.gpu_manager.initialize()
        
        # Converter seleção de pedidos para máscara binária
        orders_mask = np.zeros(self.problem.n_orders, dtype=np.int32)
        for order_id in selected_orders:
            orders_mask[order_id] = 1
        
        # Transferir para GPU
        orders_mask_gpu = cp.array(orders_mask)
        
        # Calcular demanda de itens (item_order_matrix @ orders_mask)
        item_order_matrix = self.gpu_manager.gpu_data['item_order_matrix']
        item_demand = cp.matmul(item_order_matrix, orders_mask_gpu)
        
        # Determinar quais corredores são necessários
        item_aisle_matrix = self.gpu_manager.gpu_data['item_aisle_matrix']
        
        # Primeiro, identificar itens com demanda
        items_with_demand = (item_demand > 0)
        
        # Corredores exclusivos - obrigatórios para itens que só aparecem em um corredor
        item_aisle_count = cp.sum(item_aisle_matrix > 0, axis=1)
        exclusive_items = items_with_demand & (item_aisle_count == 1)
        
        # Encontrar quais corredores contêm itens exclusivos
        necessary_aisle_mask = cp.zeros(self.problem.n_aisles, dtype=cp.int32)
        
        # Para cada item exclusivo, adicionar seu corredor
        for item_idx in cp.where(exclusive_items)[0]:
            aisle_idx = cp.where(item_aisle_matrix[item_idx] > 0)[0]
            if len(aisle_idx) > 0:
                necessary_aisle_mask[aisle_idx[0]] = 1
        
        # Agora examinar itens não cobertos
        covered_items = cp.zeros_like(items_with_demand, dtype=cp.bool_)
        
        # Marcar itens já cobertos pelos corredores necessários
        for a_idx in cp.where(necessary_aisle_mask)[0]:
            covered_items = covered_items | ((item_aisle_matrix[:, a_idx] > 0) & items_with_demand)
        
        # Enquanto houver itens não cobertos, adicionar mais corredores
        while cp.any(items_with_demand & ~covered_items):
            best_aisle = -1
            best_coverage = 0
            
            # Encontrar corredor que cobre mais itens não cobertos
            for a_idx in range(self.problem.n_aisles):
                if necessary_aisle_mask[a_idx] == 1:
                    continue
                    
                # Itens adicionais que este corredor cobriria
                new_covered = ((item_aisle_matrix[:, a_idx] > 0) & items_with_demand & ~covered_items)
                coverage = cp.sum(new_covered)
                
                if coverage > best_coverage:
                    best_coverage = coverage
                    best_aisle = a_idx
            
            if best_aisle == -1 or best_coverage == 0:
                break
                
            # Adicionar este corredor
            necessary_aisle_mask[best_aisle] = 1
            
            # Atualizar itens cobertos
            covered_items = covered_items | ((item_aisle_matrix[:, best_aisle] > 0) & items_with_demand)
        
        # Converter para lista de IDs de corredores
        required_aisles = cp.where(necessary_aisle_mask == 1)[0].get().tolist()
        
        return required_aisles
    
    def swap_neighborhood(self, solution, max_swaps=50):
        """Explora a vizinhança de troca (swap) de pedidos."""
        if self.use_gpu and self.gpu_manager:
            return self._swap_neighborhood_gpu(solution, max_swaps)
        else:
            return self._swap_neighborhood_cpu(solution, max_swaps)
    
    def _swap_neighborhood_cpu(self, solution, max_swaps=50):
        """Implementação CPU da vizinhança de troca."""
        best_solution = solution
        best_objective = solution.objective_value if solution.is_feasible else 0
        
        # Obter pedidos selecionados e não selecionados
        selected_orders = set(solution.selected_orders)
        unselected_orders = set(range(self.problem.n_orders)) - selected_orders
        
        # Explorar trocas de pedidos
        swaps_tried = 0
        for order_out in list(selected_orders):
            if swaps_tried >= max_swaps:
                break
                
            units_out = self.problem.order_units.get(order_out, 0)
            
            for order_in in list(unselected_orders):
                swaps_tried += 1
                if swaps_tried >= max_swaps:
                    break
                    
                units_in = self.problem.order_units.get(order_in, 0)
                
                # Verificar se a troca mantém a solução dentro dos limites
                new_total = solution.total_units - units_out + units_in
                if new_total < self.problem.wave_size_lb or new_total > self.problem.wave_size_ub:
                    continue
                
                # Criar nova solução com a troca
                new_selected = list(selected_orders - {order_out} | {order_in})
                
                # Recalcular corredores necessários
                new_visited_aisles = self._calculate_required_aisles(new_selected)
                
                # Criar nova solução
                new_solution = self.problem.create_solution(new_selected, new_visited_aisles)
                new_solution.is_feasible = self._is_solution_feasible(new_solution)
                
                if new_solution.is_feasible:
                    new_solution.objective_value = self._compute_objective_function(new_solution)
                    
                    # Atualizar melhor solução se necessário
                    if new_solution.objective_value > best_objective:
                        best_solution = new_solution
                        best_objective = new_solution.objective_value
    
        return best_solution
    
    def _swap_neighborhood_gpu(self, solution, max_swaps=50):
        """Implementação GPU da vizinhança de troca."""
        # Aqui podemos implementar uma versão paralela usando CUDA/CuPy
        # Por enquanto, usamos a versão CPU como fallback
        return self._swap_neighborhood_cpu(solution, max_swaps)
        
    def insert_neighborhood(self, solution, max_inserts=30):
        """Explora a vizinhança de inserção de pedidos."""
        best_solution = solution
        best_objective = solution.objective_value if solution.is_feasible else 0
        
        # Obter pedidos selecionados e não selecionados
        selected_orders = set(solution.selected_orders)
        unselected_orders = set(range(self.problem.n_orders)) - selected_orders
        
        # Tentar inserir pedidos não selecionados
        attempts = 0
        for order_in in list(unselected_orders):
            if attempts >= max_inserts:
                break
                
            attempts += 1
            units_in = self.problem.order_units.get(order_in, 0)
            
            # Verificar se adicionar o pedido mantém a solução dentro dos limites
            new_total = solution.total_units + units_in
            if new_total > self.problem.wave_size_ub:
                continue
            
            # Criar nova solução com o pedido inserido
            new_selected = list(selected_orders | {order_in})
            
            # Recalcular corredores necessários
            new_visited_aisles = self._calculate_required_aisles(new_selected)
            
            # Criar nova solução
            new_solution = self.problem.create_solution(new_selected, new_visited_aisles)
            new_solution.is_feasible = self._is_solution_feasible(new_solution)
            
            if new_solution.is_feasible:
                new_solution.objective_value = self._compute_objective_function(new_solution)
                
                # Atualizar melhor solução se necessário
                if new_solution.objective_value > best_objective:
                    best_solution = new_solution
                    best_objective = new_solution.objective_value
                    
                    # Se encontramos uma melhoria, podemos continuar a partir desta solução
                    selected_orders = set(new_solution.selected_orders)
                    unselected_orders = set(range(self.problem.n_orders)) - selected_orders
        
        return best_solution
    
    def remove_neighborhood(self, solution, max_removes=30):
        """Explora a vizinhança de remoção de pedidos."""
        # Solução atual já é a melhor conhecida inicialmente
        best_solution = solution
        best_objective = solution.objective_value if solution.is_feasible else 0
        
        # Obter pedidos selecionados
        selected_orders = set(solution.selected_orders)
        
        # Experimentar remover cada pedido
        attempts = 0
        for order_out in list(selected_orders):
            if attempts >= max_removes:
                break
                
            attempts += 1
            units_out = self.problem.order_units.get(order_out, 0)
            
            # Verificar se remover o pedido mantém a solução dentro dos limites
            new_total = solution.total_units - units_out
            if new_total < self.problem.wave_size_lb:
                continue
            
            # Criar nova solução sem o pedido
            new_selected = list(selected_orders - {order_out})
            
            # Recalcular corredores necessários
            new_visited_aisles = self._calculate_required_aisles(new_selected)
            
            # Criar nova solução
            new_solution = self.problem.create_solution(new_selected, new_visited_aisles)
            new_solution.is_feasible = self._is_solution_feasible(new_solution)
            
            if new_solution.is_feasible:
                new_solution.objective_value = self._compute_objective_function(new_solution)
                
                # Atualizar melhor solução se necessário
                if new_solution.objective_value > best_objective:
                    best_solution = new_solution
                    best_objective = new_solution.objective_value
                    
                    # Se encontramos uma melhoria, podemos continuar a partir desta solução
                    selected_orders = set(new_solution.selected_orders)
        
        return best_solution
    
    def k_swap_neighborhood(self, solution, k=2, max_attempts=20):
        """Explora a vizinhança de troca de k pedidos simultaneamente."""
        if k <= 1:
            return self.swap_neighborhood(solution)
        
        best_solution = solution
        best_objective = solution.objective_value if solution.is_feasible else 0
        
        # Obter pedidos selecionados e não selecionados
        selected_orders = list(solution.selected_orders)
        unselected_orders = list(set(range(self.problem.n_orders)) - set(selected_orders))
        
        # Se não temos pedidos suficientes, reduzir k
        k = min(k, len(selected_orders), len(unselected_orders))
        if k <= 0:
            return solution
        
        # Tentar trocas aleatórias de k pedidos
        import random
        random.seed(42)  # Para reprodutibilidade
        
        for _ in range(max_attempts):
            # Selecionar k pedidos para remover e k para adicionar
            if len(selected_orders) >= k and len(unselected_orders) >= k:
                orders_out = random.sample(selected_orders, k)
                orders_in = random.sample(unselected_orders, k)
                
                # Calcular unidades removidas e adicionadas
                units_out = sum(self.problem.order_units.get(o, 0) for o in orders_out)
                units_in = sum(self.problem.order_units.get(o, 0) for o in orders_in)
                
                # Verificar se a troca mantém a solução dentro dos limites
                new_total = solution.total_units - units_out + units_in
                if new_total < self.problem.wave_size_lb or new_total > self.problem.wave_size_ub:
                    continue
                
                # Criar nova solução com a troca
                new_selected = list(set(selected_orders) - set(orders_out) | set(orders_in))
                
                # Recalcular corredores necessários
                new_visited_aisles = self._calculate_required_aisles(new_selected)
                
                # Criar nova solução
                new_solution = self.problem.create_solution(new_selected, new_visited_aisles)
                new_solution.is_feasible = self._is_solution_feasible(new_solution)
                
                if new_solution.is_feasible:
                    new_solution.objective_value = self._compute_objective_function(new_solution)
                    
                    # Atualizar melhor solução se necessário
                    if new_solution.objective_value > best_objective:
                        best_solution = new_solution
                        best_objective = new_solution.objective_value
                        
                        # Atualizar listas para a próxima iteração
                        selected_orders = list(new_solution.selected_orders)
                        unselected_orders = list(set(range(self.problem.n_orders)) - set(selected_orders))
        
        return best_solution
    
    def aisle_based_neighborhood(self, solution, max_attempts=20):
        """Explora vizinhança baseada em otimização de corredores."""
        best_solution = solution
        best_objective = solution.objective_value if solution.is_feasible else 0
        
        # Obter corredores atuais e potenciais
        current_aisles = set(solution.visited_aisles)
        all_aisles = set(range(self.problem.n_aisles))
        unused_aisles = all_aisles - current_aisles
        
        # Lista de pedidos atuais
        current_orders = set(solution.selected_orders)
        
        # Tentar substituir cada corredor
        for aisle_out in list(current_aisles):
            # Encontrar pedidos que dependem exclusivamente deste corredor
            dependent_orders = set()
            for order_id in current_orders:
                for item_id in self.problem.orders.get(order_id, {}):
                    aisle_options = set(self.problem.item_units_by_aisle.get(item_id, {}).keys())
                    if aisle_options == {aisle_out}:  # Item só existe neste corredor
                        dependent_orders.add(order_id)
                        break
            
            # Se nenhum pedido depende exclusivamente deste corredor, podemos tentar substituí-lo
            if not dependent_orders:
                for aisle_in in list(unused_aisles):
                    # Substituir o corredor
                    new_aisles = list(current_aisles - {aisle_out} | {aisle_in})
                    
                    # Verificar quais pedidos podem ser atendidos com os novos corredores
                    supported_orders = []
                    items_available = {}
                    
                    # Calcular itens disponíveis nos novos corredores
                    for aisle_id in new_aisles:
                        for item_id, quantity in self.problem.aisles.get(aisle_id, {}).items():
                            items_available[item_id] = items_available.get(item_id, 0) + quantity
                    
                    # Verificar quais pedidos atuais podem ser mantidos
                    for order_id in current_orders:
                        can_support = True
                        for item_id, quantity in self.problem.orders.get(order_id, {}).items():
                            if items_available.get(item_id, 0) < quantity:
                                can_support = False
                                break
                        
                        if can_support:
                            supported_orders.append(order_id)
                    
                    # Verificar se a substituição mantém a solução dentro dos limites
                    new_total = sum(self.problem.order_units.get(o, 0) for o in supported_orders)
                    if new_total < self.problem.wave_size_lb:
                        continue
                    
                    # Criar nova solução
                    new_solution = self.problem.create_solution(supported_orders, new_aisles)
                    new_solution.is_feasible = self._is_solution_feasible(new_solution)
                    
                    if new_solution.is_feasible:
                        new_solution.objective_value = self._compute_objective_function(new_solution)
                        
                        # Atualizar melhor solução se necessário
                        if new_solution.objective_value > best_objective:
                            best_solution = new_solution
                            best_objective = new_solution.objective_value
                            current_aisles = set(new_solution.visited_aisles)
                            current_orders = set(new_solution.selected_orders)
                            unused_aisles = all_aisles - current_aisles
        
        return best_solution
    
    # Método helper para verificar viabilidade da solução
    def _is_solution_feasible(self, solution):
        """Verifica se uma solução é viável."""
        from src.utils.validator import SolutionValidator
        return SolutionValidator.validate_solution(self.problem, solution)
    
    # Método helper para calcular valor objetivo
    def _compute_objective_function(self, solution):
        """Calcula o valor da função objetivo."""
        from src.utils.validator import SolutionValidator
        return SolutionValidator.calculate_objective(solution)