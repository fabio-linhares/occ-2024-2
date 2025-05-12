import numpy as np
import cupy as cp
from time import time

class GPUManager:
    """Gerencia dados e computações na GPU para o problema de Wave Order Picking."""
    
    def __init__(self, problem):
        """
        Inicializa o gerenciador GPU com os dados do problema.
        
        Args:
            problem: Instância de WaveOrderPickingProblem
        """
        self.problem = problem
        self.gpu_data = {}
        self.is_initialized = False
        
    def initialize(self):
        """Inicializa estruturas de dados na GPU."""
        if self.is_initialized:
            print("GPU já inicializada, reutilizando estruturas existentes")
            return
        
        import cupy as cp
        print("Reutilizando matrizes GPU existentes do pré-processamento")
        self.is_initialized = True
        
        # Verificar se os dados já estão na GPU
        reuse_existing = hasattr(self.problem, '_gpu_item_order_map') and \
                       hasattr(self.problem, '_gpu_item_aisle_map')
        
        # Inicializar dicionário de dados GPU
        self.gpu_data = {}
        
        if reuse_existing:
            # Copiar matrizes existentes
            self.gpu_data['item_order_matrix'] = self.problem._gpu_item_order_map
            self.gpu_data['item_aisle_matrix'] = self.problem._gpu_item_aisle_map
            
            # Criar array de unidades por pedido na GPU (faltava essa inicialização)
            self.gpu_data['order_units'] = cp.array(
                [self.problem.order_units.get(o, 0) for o in range(self.problem.n_orders)],
                dtype=cp.float32
            )
            
            # Adicionar outros dados necessários
            self.gpu_data['n_orders'] = self.problem.n_orders
            self.gpu_data['n_aisles'] = self.problem.n_aisles
            self.gpu_data['n_items'] = self.problem.n_items
            self.gpu_data['wave_size_lb'] = self.problem.wave_size_lb
            self.gpu_data['wave_size_ub'] = self.problem.wave_size_ub
    
    def _create_item_order_matrix(self):
        """
        Cria uma matriz densa de itens por pedidos (item x pedido).
        
        Returns:
            np.ndarray: Matriz onde cell[i,j] = quantidade do item i no pedido j
        """
        matrix = np.zeros((self.problem.n_items, self.problem.n_orders), dtype=np.float32)
        
        for order_id, items in self.problem.orders.items():
            for item_id, quantity in items.items():
                matrix[item_id, order_id] = quantity
                
        return matrix
    
    def _create_item_aisle_matrix(self):
        """
        Cria uma matriz densa de itens por corredores (item x corredor).
        
        Returns:
            np.ndarray: Matriz onde cell[i,j] = quantidade do item i no corredor j
        """
        matrix = np.zeros((self.problem.n_items, self.problem.n_aisles), dtype=np.float32)
        
        for aisle_id, items in self.problem.aisles.items():
            for item_id, quantity in items.items():
                matrix[item_id, aisle_id] = quantity
                
        return matrix
    
    def compute_objective(self, orders_mask, aisles_mask):
        """
        Calcula o valor objetivo na GPU.
        
        Args:
            orders_mask: Array binário (0/1) de pedidos selecionados
            aisles_mask: Array binário (0/1) de corredores selecionados
            
        Returns:
            float: Valor da função objetivo (total_units / num_aisles)
        """
        if not self.is_initialized:
            self.initialize()
        
        # Converter para arrays CuPy se necessário
        if isinstance(orders_mask, np.ndarray):
            orders_mask = cp.array(orders_mask)
        if isinstance(aisles_mask, np.ndarray):
            aisles_mask = cp.array(aisles_mask)
        
        # Calcular unidades totais
        total_units = cp.sum(self.gpu_data['order_units'] * orders_mask)
        
        # Calcular número de corredores
        num_aisles = cp.sum(aisles_mask)
        
        # Evitar divisão por zero
        if num_aisles == 0:
            return 0.0
            
        return float(total_units / num_aisles)
    
    def check_constraints(self, orders_mask, aisles_mask):
        """
        Verifica se uma solução atende às restrições.
        
        Args:
            orders_mask: Array binário (0/1) de pedidos selecionados
            aisles_mask: Array binário (0/1) de corredores selecionados
            
        Returns:
            dict: Resultados da verificação de restrições
        """
        if not self.is_initialized:
            self.initialize()
            
        # Converter para arrays CuPy se necessário
        if isinstance(orders_mask, np.ndarray):
            orders_mask = cp.array(orders_mask)
        if isinstance(aisles_mask, np.ndarray):
            aisles_mask = cp.array(aisles_mask)
        
        # Restrição de tamanho da wave
        total_units = cp.sum(self.gpu_data['order_units'] * orders_mask)
        lb_satisfied = total_units >= self.gpu_data['wave_size_lb']
        ub_satisfied = total_units <= self.gpu_data['wave_size_ub']
        
        # Restrição de cobertura de itens
        # Calcular demanda total de itens para pedidos selecionados
        item_demand = cp.matmul(self.gpu_data['item_order_matrix'], orders_mask)
        
        # Calcular disponibilidade total de itens em corredores selecionados
        item_supply = cp.matmul(self.gpu_data['item_aisle_matrix'], aisles_mask)
        
        # Verificar se cada item tem oferta suficiente
        supply_satisfied = cp.all(item_demand <= item_supply)
        
        return {
            'lb_satisfied': bool(lb_satisfied),
            'ub_satisfied': bool(ub_satisfied),
            'supply_satisfied': bool(supply_satisfied),
            'total_units': float(total_units),
            'item_demand': item_demand.get(),
            'item_supply': item_supply.get()
        }
    
    def dinkelbach_iteration(self, q_value, penalties):
        """Executa uma iteração do método de Dinkelbach na GPU."""
        if not self.is_initialized:
            self.initialize()
        
        import cupy as cp  # Importar aqui para garantir disponibilidade
        
        # Extrair dados necessários
        item_order_matrix = self.gpu_data['item_order_matrix']
        item_aisle_matrix = self.gpu_data['item_aisle_matrix']
        order_units = self.gpu_data['order_units']
        wave_size_lb = self.gpu_data['wave_size_lb']
        wave_size_ub = self.gpu_data['wave_size_ub']
        n_orders = self.gpu_data['n_orders']
        n_aisles = self.gpu_data['n_aisles']
        
        # Classificar pedidos por benefício (unidades totais)
        order_coefficients = order_units.copy()
        order_indices = cp.argsort(-order_coefficients).get()
        
        # Inicializar solução vazia
        current_orders = cp.zeros(n_orders, dtype=cp.int32)
        
        # Abordagem gulosa para selecionar pedidos
        total_units = 0
        for idx in order_indices:
            units_to_add = float(order_units[idx])
            if total_units + units_to_add <= wave_size_ub:
                current_orders[idx] = 1
                total_units += units_to_add
                
                # Se atingiu o limite inferior, podemos parar de adicionar pedidos
                if total_units >= wave_size_lb:
                    break
        
        # Se não atingimos o LB, continuar adicionando pedidos mesmo ultrapassando UB
        if total_units < wave_size_lb:
            for idx in order_indices:
                if current_orders[idx] == 0:  # Se ainda não foi selecionado
                    current_orders[idx] = 1
                    total_units += float(order_units[idx])
                    if total_units >= wave_size_lb:
                        break
        
        # Calcular quais itens são necessários para os pedidos selecionados
        item_demand = cp.matmul(item_order_matrix, current_orders)
        
        # Selecionar corredores que contêm os itens necessários
        required_aisles = cp.zeros(n_aisles, dtype=cp.int32)
        
        # Primeiro, selecionar corredores com itens exclusivos
        for a_id in range(n_aisles):
            aisle_items = item_aisle_matrix[:, a_id]
            # Verificar se este corredor tem itens exclusivos que são demandados
            exclusive_items = cp.where(
                (item_demand > 0) & 
                (cp.sum(item_aisle_matrix > 0, axis=1) == 1) &
                (aisle_items > 0)
            )[0]
            
            if len(exclusive_items) > 0:
                required_aisles[a_id] = 1
        
        # Em seguida, selecionar corredores adicionais para cobrir todos os itens
        items_covered = cp.zeros_like(item_demand, dtype=cp.bool_)
        for a_id in range(n_aisles):
            if required_aisles[a_id] == 1:
                aisle_items = item_aisle_matrix[:, a_id]
                items_covered = items_covered | (aisle_items > 0)
        
        # Se ainda faltam itens, selecionar mais corredores
        uncovered_items = cp.where((item_demand > 0) & ~items_covered)[0]
        
        while len(uncovered_items) > 0:
            best_aisle = -1
            best_coverage = 0
            
            for a_id in range(n_aisles):
                if required_aisles[a_id] == 0:
                    coverage = cp.sum((item_aisle_matrix[:, a_id] > 0) & (item_demand > 0) & ~items_covered)
                    if coverage > best_coverage:
                        best_coverage = coverage
                        best_aisle = a_id
            
            if best_aisle == -1 or best_coverage == 0:
                break  # Não é possível cobrir mais itens
                
            required_aisles[best_aisle] = 1
            aisle_items = item_aisle_matrix[:, best_aisle]
            items_covered = items_covered | (aisle_items > 0)
            uncovered_items = cp.where((item_demand > 0) & ~items_covered)[0]
        
        # Verificar se todos os itens necessários estão cobertos
        all_covered = cp.all(items_covered | (item_demand == 0))
        
        # Calcular valor objetivo e F(q)
        num_aisles = cp.sum(required_aisles)
        if num_aisles == 0:  # Evitar divisão por zero
            # Selecionar pelo menos um corredor
            best_aisle = cp.argmax(cp.sum(item_aisle_matrix, axis=0))
            required_aisles[best_aisle] = 1
            num_aisles = 1
        
        # Verificar viabilidade: LB <= unidades <= UB E todos os itens cobertos
        is_feasible = (wave_size_lb <= total_units <= wave_size_ub) and all_covered
        
        # Calcular valores para o algoritmo de Dinkelbach
        objective_value = float(total_units / num_aisles) if is_feasible else 0.1  # Valor pequeno mas não zero
        f_q_value = float(total_units - q_value * num_aisles)
        
        print(f"GPU Iteration: Orders={cp.sum(current_orders)}, Aisles={num_aisles}, Units={total_units}")
        print(f"Feasible: {is_feasible}, Obj: {objective_value}, F(q): {f_q_value}")
        
        return current_orders, required_aisles, objective_value, f_q_value
    
    def _np_gpu_dinkelbach_iteration(self, q_value, penalties):
        """
        Executa uma iteração do método de Dinkelbach com otimização baseada em GPU.
        
        Args:
            q_value: Valor atual de q no algoritmo de Dinkelbach
            penalties: Dicionário com penalidades para restrições suaves
            
        Returns:
            tuple: (orders_mask, aisles_mask, objective_value, f_q_value)
        """
        if not self.is_initialized:
            self.initialize()
        
        # Extrair dados necessários
        item_order_matrix = self.gpu_data['item_order_matrix']
        item_aisle_matrix = self.gpu_data['item_aisle_matrix']
        order_units = self.gpu_data['order_units']
        wave_size_lb = self.gpu_data['wave_size_lb']
        wave_size_ub = self.gpu_data['wave_size_ub']
        n_orders = self.gpu_data['n_orders']
        n_aisles = self.gpu_data['n_aisles']
        
        # Inicializar máscara de pedidos e corredores
        orders_mask = cp.random.randint(0, 2, size=n_orders, dtype=cp.int32)
        
        # Aplicar iterações de melhoria local
        max_iterations = 100
        best_objective = float('-inf')
        best_orders_mask = None
        best_aisles_mask = None
        
        for iteration in range(max_iterations):
            # Verificar restrição de limites de tamanho de wave
            total_units = cp.sum(order_units * orders_mask)
            
            # Ajustar para satisfazer limites
            if total_units < wave_size_lb:
                # Adicionar mais pedidos se estiver abaixo do limite inferior
                potential_orders = cp.where((orders_mask == 0))[0]
                if len(potential_orders) > 0:
                    order_efficiency = order_units[potential_orders]
                    sorted_indices = cp.argsort(-order_efficiency)
                    
                    for idx in sorted_indices:
                        order_id = potential_orders[idx]
                        if total_units + order_units[order_id] <= wave_size_ub:
                            orders_mask[order_id] = 1
                            total_units += order_units[order_id]
                        
                        if total_units >= wave_size_lb:
                            break
            
            elif total_units > wave_size_ub:
                # Remover pedidos se estiver acima do limite superior
                selected_orders = cp.where(orders_mask == 1)[0]
                order_efficiency = order_units[selected_orders]
                sorted_indices = cp.argsort(order_efficiency)
                
                for idx in sorted_indices:
                    order_id = selected_orders[idx]
                    orders_mask[order_id] = 0
                    total_units -= order_units[order_id]
                    
                    if total_units <= wave_size_ub:
                        break
            
            # Calcular a demanda de itens com os pedidos selecionados
            item_demand = cp.matmul(item_order_matrix, orders_mask)
            
            # Determinar quais corredores são necessários
            aisles_mask = cp.zeros(n_aisles, dtype=cp.int32)
            items_to_cover = cp.where(item_demand > 0)[0]
            
            for item_id in items_to_cover:
                candidate_aisles = cp.where(item_aisle_matrix[item_id] > 0)[0]
                current_supply = cp.sum(item_aisle_matrix[item_id] * aisles_mask)
                
                if current_supply < item_demand[item_id]:
                    aisle_efficiency = item_aisle_matrix[item_id][candidate_aisles]
                    sorted_indices = cp.argsort(-aisle_efficiency)
                    
                    for idx in sorted_indices:
                        aisle_id = candidate_aisles[idx]
                        if aisles_mask[aisle_id] == 0:
                            aisles_mask[aisle_id] = 1
                            current_supply += item_aisle_matrix[item_id, aisle_id]
                        
                        if current_supply >= item_demand[item_id]:
                            break
            
            # Verificar se todas as demandas são atendidas
            item_supply = cp.matmul(item_aisle_matrix, aisles_mask)
            
            if not cp.all(item_demand <= item_supply):
                for item_id in cp.where(item_demand > item_supply)[0]:
                    candidate_aisles = cp.where(item_aisle_matrix[item_id] > 0)[0]
                    for aisle_id in candidate_aisles:
                        if aisles_mask[aisle_id] == 0:
                            aisles_mask[aisle_id] = 1
                            item_supply[item_id] += item_aisle_matrix[item_id, aisle_id]
                        
                        if item_supply[item_id] >= item_demand[item_id]:
                            break
            
            # Calcular o valor da função objetivo
            f_value = float(total_units)
            g_value = float(cp.sum(aisles_mask))
            
            if g_value > 0:
                f_q_value = f_value / g_value
                objective_value = f_value - q_value * g_value
            else:
                f_q_value = 0.0
                objective_value = float('-inf')
            
            # Verificar se esta solução é melhor
            if objective_value > best_objective and g_value > 0:
                best_objective = objective_value
                best_orders_mask = cp.copy(orders_mask)
                best_aisles_mask = cp.copy(aisles_mask)
            
            # Perturbação para próxima iteração
            flip_indices = cp.random.choice(n_orders, size=max(1, n_orders // 10), replace=False)
            orders_mask[flip_indices] = 1 - orders_mask[flip_indices]
        
        if best_orders_mask is None:
            return cp.zeros(n_orders, dtype=cp.int32), cp.zeros(n_aisles, dtype=cp.int32), 0.0, 0.0
        
        # Calcular valores finais
        total_units = cp.sum(order_units * best_orders_mask)
        num_aisles = cp.sum(best_aisles_mask)
        
        f_q_value = float(total_units / num_aisles) if num_aisles > 0 else 0.0
        objective_value = float(total_units - q_value * num_aisles)
        
        return best_orders_mask, best_aisles_mask, objective_value, f_q_value
        
    def compute_neighborhood_evaluation(self, base_orders, base_aisles, neighbor_candidates, mode='swap'):
        """
        Avalia múltiplos vizinhos em paralelo na GPU.
        
        Args:
            base_orders: Array binário da solução base (pedidos)
            base_aisles: Array binário da solução base (corredores)
            neighbor_candidates: Lista de tuplas (pedidos_a_remover, pedidos_a_adicionar)
            mode: Tipo de vizinhança ('swap', 'insert', 'remove')
        
        Returns:
            list: Avaliações de cada vizinho [(vizinho_idx, objective_value, is_feasible), ...]
        """
        if not self.is_initialized:
            self.initialize()
        
        # Converter para arrays GPU
        base_orders_gpu = cp.array(base_orders)
        base_aisles_gpu = cp.array(base_aisles)
        
        # Configurar arrays para armazenar resultados
        n_neighbors = len(neighbor_candidates)
        evaluations = []
        
        # Avaliar cada vizinho
        for idx, (orders_out, orders_in) in enumerate(neighbor_candidates):
            # Criar solução do vizinho
            neighbor_orders = base_orders_gpu.copy()
            
            # Aplicar alterações nos pedidos
            for o_id in orders_out:
                neighbor_orders[o_id] = 0
            for o_id in orders_in:
                neighbor_orders[o_id] = 1
            
            # Calcular corredores necessários
            item_demand = cp.matmul(self.gpu_data['item_order_matrix'], neighbor_orders)
            
            # Selecionar corredores minimizando o total
            required_aisles = cp.zeros_like(base_aisles_gpu)
            
            # Primeiro, selecionar corredores com itens exclusivos
            exclusive_aisles = self._compute_exclusive_aisles(item_demand)
            required_aisles = cp.logical_or(required_aisles, exclusive_aisles)
            
            # Selecionar mais corredores para cobrir demanda
            required_aisles = self._compute_additional_aisles(item_demand, required_aisles)
            
            # Verificar viabilidade
            constraints = self.check_constraints(neighbor_orders, required_aisles)
            is_feasible = constraints['lb_satisfied'] and constraints['ub_satisfied'] and constraints['supply_satisfied']
            
            # Calcular valor objetivo
            objective = 0.0
            if is_feasible:
                objective = self.compute_objective(neighbor_orders, required_aisles)
            
            evaluations.append((idx, float(objective), bool(is_feasible)))
        
        return evaluations

    def _compute_exclusive_aisles(self, item_demand):
        """Calcula quais corredores têm itens exclusivos que são demandados."""
        # Implementação otimizada para GPU
        n_items, n_aisles = self.gpu_data['item_aisle_matrix'].shape
        
        # Contar em quantos corredores cada item aparece
        item_aisle_counts = cp.sum(self.gpu_data['item_aisle_matrix'] > 0, axis=1)
        
        # Criar máscara para itens com demanda que aparecem em apenas um corredor
        exclusive_items_mask = (item_demand > 0) & (item_aisle_counts == 1)
        
        # Encontrar em qual corredor cada item exclusivo aparece
        exclusive_aisles = cp.zeros(n_aisles, dtype=cp.bool_)
        
        for item_idx in cp.where(exclusive_items_mask)[0]:
            # Encontrar o único corredor que contém este item
            aisle_idx = cp.where(self.gpu_data['item_aisle_matrix'][item_idx] > 0)[0]
            if len(aisle_idx) > 0:
                exclusive_aisles[aisle_idx[0]] = True
        
        return exclusive_aisles

    def batch_perturb(self, solution, num_perturbations=10, intensity=0.2):
        """Gera e avalia múltiplas perturbações em paralelo usando GPU."""
        if not self.is_initialized:
            self.initialize()
            
        import numpy as np
        import cupy as cp
        
        # Extrair dados da solução
        selected_orders = solution.selected_orders
        
        # Criar array binário representando pedidos selecionados
        order_mask = np.zeros(self.problem.n_orders, dtype=np.int32)
        for o in selected_orders:
            if 0 <= o < self.problem.n_orders:
                order_mask[o] = 1
        
        # Calcular número de pedidos a perturbar
        n_perturb = max(1, int(len(selected_orders) * intensity))
        
        # Gerar perturbações
        perturbed_masks = []
        for _ in range(num_perturbations):
            # Copiar máscara original
            mask = order_mask.copy()
            
            # Selecionar aleatoriamente pedidos para remover
            orders_to_flip = np.random.choice(
                np.where(mask == 1)[0], 
                size=min(n_perturb, sum(mask)), 
                replace=False
            )
            
            # Desligar pedidos removidos
            mask[orders_to_flip] = 0
            
            # Selecionar pedidos para adicionar
            available_orders = np.where(order_mask == 0)[0]
            if len(available_orders) > 0:
                orders_to_add = np.random.choice(
                    available_orders,
                    size=min(n_perturb, len(available_orders)),
                    replace=False
                )
                
                # Ligar pedidos adicionados
                mask[orders_to_add] = 1
            
            perturbed_masks.append(mask)
        
        # Avaliar perturbações em lote na GPU
        results = []
        order_units_gpu = cp.array(self.problem.order_units_array)
        
        for idx, mask in enumerate(perturbed_masks):
            mask_gpu = cp.array(mask)
            
            # Verificar limites de unidades
            total_units = float(cp.sum(order_units_gpu * mask_gpu))
            if not (self.problem.wave_size_lb <= total_units <= self.problem.wave_size_ub):
                results.append((0.0, None, False))
                continue
            
            # Calcular demanda de itens e corredores necessários
            item_demand = cp.matmul(self.gpu_data['item_order_matrix'], mask_gpu)
            items_needed = (item_demand > 0)
            
            # Selecionar corredores necessários
            aisle_mask = self._compute_required_aisles_gpu(items_needed)
            
            # Verificar cobertura de itens
            all_covered = self._verify_item_coverage_gpu(items_needed, aisle_mask)
            
            if all_covered:
                num_aisles = float(cp.sum(aisle_mask))
                objective = total_units / num_aisles if num_aisles > 0 else 0
                results.append((objective, aisle_mask.get(), True))
            else:
                results.append((0.0, None, False))
        
        # Encontrar a melhor perturbação viável
        best_value = solution.objective_value
        best_idx = -1
        
        for i, (value, aisles, is_feasible) in enumerate(results):
            if is_feasible and value > best_value:
                best_value = value
                best_idx = i
        
        if best_idx >= 0:
            # Criar nova solução
            new_orders = [i for i, v in enumerate(perturbed_masks[best_idx]) if v == 1]
            new_aisles = [i for i, v in enumerate(results[best_idx][1]) if v == 1]
            
            new_solution = self.problem.create_solution(new_orders, new_aisles)
            new_solution.is_feasible = True
            new_solution.objective_value = best_value
            
            return new_solution, best_value
        
        return solution, solution.objective_value

    def cleanup(self):
        """Libera memória GPU."""
        for key in list(self.gpu_data.keys()):
            if isinstance(self.gpu_data[key], cp.ndarray):
                self.gpu_data[key] = None
        
        # Força liberação de memória
        cp.get_default_memory_pool().free_all_blocks()
        self.is_initialized = False
        print("Memória GPU liberada")