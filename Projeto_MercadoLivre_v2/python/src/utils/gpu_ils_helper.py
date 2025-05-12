import numpy as np
import time

# Variáveis globais para controle do CuPy
cp = None
_CUDA_IMPORT_SUCCESSFUL = False

def initialize_gpu():
    """Inicializa o ambiente GPU."""
    global cp, _CUDA_IMPORT_SUCCESSFUL
    
    if _CUDA_IMPORT_SUCCESSFUL:
        return True
    
    try:
        import cupy as cp
        _CUDA_IMPORT_SUCCESSFUL = True
        print("GPU inicializada com sucesso via CuPy")
        return True
    except ImportError:
        print("CuPy não está disponível. Operações GPU desabilitadas.")
        cp = None
        return False
    except Exception as e:
        print(f"Erro ao inicializar GPU: {str(e)}")
        cp = None
        return False

class GPUILSHelper:
    """Classe auxiliar para operações GPU no ILS."""
    
    def __init__(self, problem):
        """Inicializa o helper GPU."""
        self.problem = problem
        self.use_gpu = initialize_gpu()
        self.gpu_data = {}
        
        if self.use_gpu:
            self._initialize_gpu_data()
    
    def _initialize_gpu_data(self):
        """Inicializa estruturas de dados na GPU."""
        if not self.use_gpu or not cp:
            return
        
        # Criar array de unidades por pedido na GPU
        # Correção: criar um array a partir do dicionário order_units existente
        order_units_array = np.zeros(self.problem.n_orders, dtype=np.float32)
        for o_id, units in self.problem.order_units.items():
            if 0 <= o_id < self.problem.n_orders:
                order_units_array[o_id] = units
        
        self.gpu_data['order_units'] = cp.array(order_units_array, dtype=cp.float32)
        
        # Criar matriz de itens por pedidos
        item_order_matrix = np.zeros((self.problem.n_items, self.problem.n_orders), dtype=np.float32)
        for order_id, items in self.problem.orders.items():
            for item_id, qty in items.items():
                item_order_matrix[item_id, order_id] = qty
        
        self.gpu_data['item_order_matrix'] = cp.array(item_order_matrix)
        
        # Criar matriz de itens por corredores
        item_aisle_matrix = np.zeros((self.problem.n_items, self.problem.n_aisles), dtype=np.float32)
        for aisle_id, items in self.problem.aisles.items():
            for item_id, qty in items.items():
                item_aisle_matrix[item_id, aisle_id] = qty
        
        self.gpu_data['item_aisle_matrix'] = cp.array(item_aisle_matrix)
        
        # Adicionar outros dados necessários
        self.gpu_data['n_orders'] = self.problem.n_orders
        self.gpu_data['n_aisles'] = self.problem.n_aisles
        self.gpu_data['n_items'] = self.problem.n_items
        self.gpu_data['wave_size_lb'] = self.problem.wave_size_lb
        self.gpu_data['wave_size_ub'] = self.problem.wave_size_ub
    
    def evaluate_solution_batch(self, order_masks):
        """
        Avalia várias soluções em paralelo usando GPU.
        
        Args:
            order_masks: Lista de arrays binários (0/1) de pedidos selecionados
            
        Returns:
            list: Lista de (objective_value, aisle_mask, is_feasible)
        """
        if not self.use_gpu or not cp:
            return None
        
        # Converter para tensor GPU
        order_tensor = cp.array(order_masks)
        batch_size = order_tensor.shape[0]
        
        # Calcular unidades totais para cada solução
        total_units = cp.matmul(order_tensor, self.gpu_data['order_units'])
        
        # Verificar limites LB e UB
        lb_satisfied = total_units >= self.gpu_data['wave_size_lb']
        ub_satisfied = total_units <= self.gpu_data['wave_size_ub']
        
        # Calcular demanda de itens para cada solução
        item_demand = cp.matmul(order_tensor, self.gpu_data['item_order_matrix'].T)
        
        # Determinar quais corredores são necessários para cada solução
        # Inicializar matriz de corredores (solução x corredor)
        aisle_masks = cp.zeros((batch_size, self.gpu_data['n_aisles']), dtype=cp.int32)
        
        # Calcular quais corredores são necessários para cada item em cada solução
        for i in range(batch_size):
            # Otimizar seleção de corredores para a solução i
            needed_items = item_demand[i] > 0
            
            if cp.any(needed_items):
                for a in range(self.gpu_data['n_aisles']):
                    # Verificar se este corredor tem itens necessários
                    has_needed_items = cp.any(self.gpu_data['item_aisle_matrix'][:, a] * needed_items)
                    if has_needed_items:
                        aisle_masks[i, a] = 1
        
        # Calcular oferta de itens com os corredores selecionados
        item_supply = cp.matmul(aisle_masks, self.gpu_data['item_aisle_matrix'].T)
        
        # Verificar se a oferta cobre a demanda
        supply_satisfied = cp.all((item_demand == 0) | (item_supply >= item_demand), axis=1)
        
        # Uma solução é viável se: LB <= unidades <= UB E oferta >= demanda
        feasible = lb_satisfied & ub_satisfied & supply_satisfied
        
        # Calcular número de corredores para cada solução
        num_aisles = cp.sum(aisle_masks, axis=1)
        
        # Calcular valor objetivo (evitar divisão por zero)
        objective_values = cp.zeros(batch_size, dtype=cp.float32)
        valid_indices = (num_aisles > 0) & feasible
        if cp.any(valid_indices):
            objective_values[valid_indices] = total_units[valid_indices] / num_aisles[valid_indices]
        
        # Transferir resultados para CPU
        return list(zip(
            objective_values.get(),
            aisle_masks.get(),
            feasible.get()
        ))
    
    def batch_perturb(self, solution, num_perturbations=10, intensity=0.2):
        """
        Gera e avalia várias perturbações em paralelo usando GPU.
        
        Args:
            solution: Solução atual
            num_perturbations: Número de perturbações para gerar
            intensity: Intensidade da perturbação
            
        Returns:
            tuple: (melhor_solução, valor_objetivo)
        """
        if not self.use_gpu or not cp:
            return solution, solution.objective_value
        
        try:
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
                    size=min(n_perturb, np.sum(mask)),  # Evitar tentar selecionar mais que o disponível
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
        
            # Avaliar perturbações em lote
            results = self.evaluate_solution_batch(perturbed_masks)
        
            if not results:
                return solution, solution.objective_value
        
            # Encontrar a melhor perturbação viável
            best_value = solution.objective_value
            best_mask_idx = -1
        
            for i, (value, _, is_feasible) in enumerate(results):
                if is_feasible and value > best_value:
                    best_value = value
                    best_mask_idx = i
        
            # Se encontramos uma perturbação melhor, criar nova solução
            if best_mask_idx >= 0:
                best_mask = perturbed_masks[best_mask_idx]
                best_aisle_mask = results[best_mask_idx][1]
                
                # Criar listas de pedidos e corredores
                new_selected_orders = [o for o in range(self.problem.n_orders) if best_mask[o] == 1]
                new_visited_aisles = [a for a in range(self.problem.n_aisles) if best_aisle_mask[a] == 1]
                
                # Criar nova solução
                new_solution = self.problem.create_solution(new_selected_orders, new_visited_aisles)
                new_solution.is_feasible = True  # Já validamos que é viável
                new_solution.objective_value = best_value
                
                return new_solution, best_value
        
        except Exception as e:
            print(f"Erro ao perturbar com GPU: {str(e)}")
            # Falhar graciosamente para CPU em caso de erros
            return solution, solution.objective_value
        
        return solution, solution.objective_value
    
    def cleanup(self):
        """Libera recursos GPU."""
        if self.use_gpu and cp:
            for key in list(self.gpu_data.keys()):
                if isinstance(self.gpu_data[key], cp.ndarray):
                    self.gpu_data[key] = None
            
            # Forçar liberação de memória
            cp.get_default_memory_pool().free_all_blocks()
            print("Memória GPU liberada")