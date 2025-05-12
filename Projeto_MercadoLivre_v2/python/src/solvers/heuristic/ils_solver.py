import time
import random
import numpy as np
import copy
import math
from src.solvers.base_solver import BaseSolver
from src.solvers.heuristic.vnd import VND

class ILSSolver(BaseSolver):
    """
    Implementação de Iterated Local Search (ILS) com elementos de 
    TABU e GRASP para o problema de Wave Order Picking.
    """
    
    def __init__(self, problem, config=None):
        """Inicializa o solver ILS."""
        super().__init__(problem, config)
        
        # Configurações do ILS
        meta_config = self.config.get('meta_heuristic', {})
        self.vnd = VND(problem, config)
        
        # Lista TABU
        self.tabu_list = []
        self.tabu_hashes = set()
        self.tabu_tenure = meta_config.get('tabu_tenure', 10)
        
        # Parâmetros do ILS
        self.max_iterations = meta_config.get('max_iterations', 100)
        self.max_iterations_without_improvement = meta_config.get('max_iterations_without_improvement', 20)
        self.perturbation_strength = meta_config.get('perturbation_strength', 0.2)
        
        # Parâmetros do GRASP
        self.alpha = meta_config.get('alpha', 0.2)
        self.rcl_size = meta_config.get('rcl_size', 5)
        
        # Semente para reprodutibilidade
        self.seed = self.config.get('algorithm', {}).get('seed', 42)
        random.seed(self.seed)
        np.random.seed(self.seed)
        
        # GPU
        self.use_gpu = self.config.get('algorithm', {}).get('use_gpu', False)
        self.gpu_manager = None
        
        # Inicializar GPU Manager se necessário
        if self.use_gpu:
            try:
                from src.utils.gpu_manager import GPUManager
                self.gpu_manager = GPUManager(problem)
                self.gpu_manager.initialize()
                print("GPU habilitada para ILS")
            except Exception as e:
                print(f"Erro ao inicializar GPU: {str(e)}")
                self.use_gpu = False
        
        # Debug
        self.debug = meta_config.get('debug', False)
    
    def solve(self, start_time=None):
        """Resolve o problema usando ILS com memória TABU e reinicialização GRASP."""
        
        # Inicializar contagem de tempo se não fornecido
        start_time = start_time or time.time()
        
        # Inicializar estruturas TABU
        self.tabu_list = []
        self.tabu_hashes = set()
        
        # Inicializar GPU se configurado
        if self.use_gpu and not hasattr(self, 'gpu_manager'):
            try:
                from src.utils.gpu_manager import GPUManager
                self.gpu_manager = GPUManager(self.problem)
                self.gpu_manager.initialize()
                print("GPU inicializada com sucesso via CuPy")
            except Exception as e:
                print(f"Erro ao inicializar GPU: {str(e)}")
                self.use_gpu = False
        
        # Construção inicial (GRASP)
        if self.debug:
            print("Construindo solução inicial com GRASP...")
        
        current_solution = self._construct_initial_solution(start_time)
        
        if not current_solution or not current_solution.is_feasible:
            if self.debug:
                print("Primeira construção falhou. Tentando construção alternativa...")
            # Tentar uma abordagem diferente
            current_solution = self._construct_alternative_solution(start_time)
        
        # AQUI É O PONTO CRÍTICO - VERIFICAR SE A SOLUÇÃO ALTERNATIVA TAMBÉM FALHOU
        if not current_solution or not current_solution.is_feasible:
            # Ambas as abordagens falharam, não continuar com o algoritmo
            print("\nNão foi possível encontrar uma solução viável para esta instância")
            # Retornar uma solução vazia ou a melhor solução inviável encontrada
            if current_solution:
                return current_solution
            else:
                return self.problem.create_solution([], [])
        
        # Melhor solução encontrada até o momento
        best_solution = copy.deepcopy(current_solution)
        iterations_without_improvement = 0
        
        # Processo principal do ILS
        for iteration in range(self.max_iterations):
            # Verificar timeout
            if self.check_timeout(start_time):
                if self.debug:
                    print(f"Timeout atingido após {iteration} iterações.")
                break
            
            # Perturbação (aumenta com iterações sem melhoria)
            intensity = self.perturbation_strength * (1 + 0.1 * iterations_without_improvement)
            perturbed_solution = self._perturb(current_solution, intensity)
            
            # Verificar se solução perturbada é TABU
            is_tabu = self._is_tabu(perturbed_solution)
            
            if is_tabu:
                continue
            
            # Busca local (VND)
            improved_solution = self.vnd.search(perturbed_solution)
            
            # Atualizar solução atual e melhor solução
            if improved_solution.is_feasible and improved_solution.objective_value > current_solution.objective_value:
                current_solution = improved_solution
                self._update_tabu_list(current_solution)
                iterations_without_improvement = 0
                
                if improved_solution.objective_value > best_solution.objective_value:
                    best_solution = copy.deepcopy(improved_solution)
                    if self.debug:
                        print(f"Nova melhor solução: {best_solution.objective_value:.2f}")
            else:
                iterations_without_improvement += 1
            
            # Verificar critério de parada
            if iterations_without_improvement >= self.max_iterations_without_improvement:
                if self.debug:
                    print(f"Parando após {iterations_without_improvement} iterações sem melhoria.")
                break
        
        # Liberar recursos GPU
        if self.use_gpu and hasattr(self, 'gpu_manager') and self.gpu_manager:
            self.gpu_manager.cleanup()
        
        return best_solution
    
    def _construct_initial_solution(self, start_time, alpha=None):
        """Constrói uma solução inicial usando GRASP."""
        import random
        import math
        
        alpha = alpha or self.alpha
        
        # Limitar alpha entre 0.05 e 0.95
        alpha = max(0.05, min(0.95, alpha))
        
        # Calcular valor para cada pedido
        order_values = []
        for o_id in range(self.problem.n_orders):
            units = self.problem.order_units.get(o_id, 0)
            items = len(self.problem.orders.get(o_id, {}))
            # Considerar tanto unidades quanto variedade de itens
            value = units / (1 + math.log(1 + items)) if items > 0 else 0
            order_values.append((o_id, value, units))
        
        # Ordenar por valor decrescente
        order_values.sort(key=lambda x: x[1], reverse=True)
        
        # Lista restrita de candidatos (RCL)
        rcl_size = max(3, int(len(order_values) * alpha))
        
        # Construir solução
        selected_orders = []
        total_units = 0
        
        # Fase construtiva gulosa com aleatoriedade
        while total_units < self.problem.wave_size_lb and order_values:
            # Criar RCL com os melhores candidatos
            rcl = order_values[:rcl_size]
            
            # Escolher aleatoriamente da RCL
            chosen_idx = random.randint(0, len(rcl) - 1)
            chosen = rcl[chosen_idx]
            
            # Adicionar à solução
            selected_orders.append(chosen[0])
            total_units += chosen[2]
            
            # Remover da lista de candidatos
            order_values = [o for o in order_values if o[0] != chosen[0]]
        
        # Se ainda não atingimos o limite mínimo, adicionar mais pedidos
        if total_units < self.problem.wave_size_lb:
            remaining = [o for o in range(self.problem.n_orders) if o not in selected_orders]
            random.shuffle(remaining)
            
            for o_id in remaining:
                if self.check_timeout(start_time):
                    break
                    
                units = self.problem.order_units.get(o_id, 0)
                if total_units + units <= self.problem.wave_size_ub:
                    selected_orders.append(o_id)
                    total_units += units
                    
                    if total_units >= self.problem.wave_size_lb:
                        break
        
        # Calcular corredores necessários
        if self.use_gpu and hasattr(self, 'gpu_manager') and self.gpu_manager:
            if not hasattr(self, 'local_search'):
                from src.solvers.heuristic.local_search import LocalSearch
                self.local_search = LocalSearch(self.problem, self.config)
            visited_aisles = self.local_search._calculate_required_aisles_gpu(selected_orders)
        else:
            # Cálculo básico de corredores necessários
            needed_items = {}
            for o_id in selected_orders:
                for item_id, quantity in self.problem.orders.get(o_id, {}).items():
                    needed_items[item_id] = needed_items.get(item_id, 0) + quantity
            
            # Selecionar corredores que contêm os itens necessários
            visited_aisles = []
            for a_id in range(self.problem.n_aisles):
                aisle_items = self.problem.aisles.get(a_id, {})
                for item_id in needed_items:
                    if item_id in aisle_items:
                        visited_aisles.append(a_id)
                        break
        
        # Criar solução
        solution = self.problem.create_solution(selected_orders, visited_aisles)
        solution.set_total_units(total_units)
        
        # Verificar viabilidade
        solution.is_feasible = self._is_solution_feasible(solution)
        if solution.is_feasible:
            solution.objective_value = self._compute_objective_function(solution)
        
        return solution
    
    def _construct_alternative_solution(self, start_time):
        """Constrói uma solução alternativa usando um método diferente."""
        # Usar algoritmo guloso puro como fallback
        from src.solvers.heuristic.greedy_solver import GreedySolver
        print("Resolvendo com heurística gulosa melhorada...")
        greedy = GreedySolver(self.problem, self.config)
        solution = greedy.solve(start_time)
        
        if solution and solution.is_feasible:
            if self.debug:
                print(f"Solução gulosa viável encontrada: {solution.objective_value:.2f}")
        else:
            # Imprimir somente uma vez esta mensagem
            print("Nenhuma solução heurística viável encontrada.")
        
        return solution
    
    def _perturb(self, solution, intensity):
        """Perturba uma solução com intensidade variável."""
        
        # Verificar se a solução é viável - se não for, não tente perturbar
        if not solution.is_feasible:
            if self.debug:
                print("Tentando perturbar solução inviável. Retornando solução original.")
            return solution
        
        # Determinar número de operações baseado na intensidade
        n_orders = len(solution.selected_orders)
        n_operations = max(1, int(n_orders * intensity))
        
        # Criar cópia da solução para não modificar a original
        perturbed = copy.deepcopy(solution)
        
        # Usar GPU se disponível para gerar múltiplas perturbações e avaliar em paralelo
        if self.use_gpu and hasattr(self, 'gpu_manager') and self.gpu_manager:
            try:
                from src.utils.gpu_ils_helper import GPUILSHelper
                helper = GPUILSHelper(self.problem)
                return helper.batch_perturb(perturbed, 10, intensity)[0]
            except Exception as e:
                if self.debug:
                    print(f"Erro ao perturbar com GPU: {str(e)}")
                    print("Usando perturbação CPU como fallback")
        
        # Implementação CPU (fallback)
        selected_orders = list(perturbed.selected_orders)
        all_orders = list(range(self.problem.n_orders))
        unselected_orders = [o for o in all_orders if o not in selected_orders]
        
        for _ in range(n_operations):
            # Remover alguns pedidos
            if selected_orders:
                n_remove = min(n_operations, len(selected_orders))
                to_remove = random.sample(selected_orders, n_remove)
                for o in to_remove:
                    selected_orders.remove(o)
                    unselected_orders.append(o)
            
            # Adicionar alguns pedidos
            if unselected_orders:
                n_add = min(n_operations, len(unselected_orders))
                to_add = random.sample(unselected_orders, n_add)
                for o in to_add:
                    selected_orders.append(o)
                    unselected_orders.remove(o)
        
        # Recalcular corredores necessários
        if hasattr(self, 'local_search'):
            visited_aisles = self.local_search._calculate_required_aisles(selected_orders)
        else:
            # Método alternativo para encontrar corredores
            visited_aisles = []
            needed_items = {}
            
            # Identificar itens necessários
            for o_id in selected_orders:
                for item_id, quantity in self.problem.orders.get(o_id, {}).items():
                    needed_items[item_id] = needed_items.get(item_id, 0) + quantity
            
            # Selecionar corredores necessários
            for aisle_id in range(self.problem.n_aisles):
                aisle_items = self.problem.aisles.get(aisle_id, {})
                for item_id in aisle_items:
                    if item_id in needed_items and needed_items[item_id] > 0:
                        visited_aisles.append(aisle_id)
                        break
        
        perturbed.selected_orders = selected_orders
        perturbed.visited_aisles = visited_aisles
        
        # Verificar viabilidade
        perturbed.is_feasible = self._is_solution_feasible(perturbed)
        if perturbed.is_feasible:
            perturbed.objective_value = self._compute_objective_function(perturbed)
        
        # Verificar se a solução perturbada é viável, caso contrário, criar uma solução viável aleatoriamente
        if not perturbed.is_feasible:
            # Tentar construir uma solução viável aleatória
            remaining_time = float('inf')  # Sem limite de tempo aqui
            random_solution = self._construct_initial_solution(time.time(), alpha=0.5)
            if random_solution and random_solution.is_feasible:
                return random_solution
            
            # Se falhar, retornar a solução original
            return solution

        return perturbed
    
    def _is_tabu(self, solution):
        """Verifica se uma solução é TABU (similar a soluções já visitadas)."""
        # Criar hash da solução baseado nos pedidos selecionados
        solution_hash = hash(tuple(sorted(solution.selected_orders)))
        
        # Verificar se o hash está na lista TABU
        if solution_hash in self.tabu_hashes:
            return True
            
        # Verificar similaridade com soluções na lista TABU
        for tabu_orders in self.tabu_list:
            # Calcular coeficiente de Jaccard (medida de similaridade entre conjuntos)
            intersection = len(set(solution.selected_orders).intersection(set(tabu_orders)))
            union = len(set(solution.selected_orders).union(set(tabu_orders)))
            
            similarity = intersection / union if union > 0 else 0
            
            # Se a similaridade for muito alta, considerar TABU
            if similarity > 0.9:  # Limiar de 90% de similaridade
                return True
        
        return False

    def _update_tabu_list(self, solution):
        """Atualiza a lista TABU com a solução atual."""
        # Adicionar hash da solução
        solution_hash = hash(tuple(sorted(solution.selected_orders)))
        self.tabu_hashes.add(solution_hash)
        
        # Adicionar pedidos à lista TABU
        self.tabu_list.append(list(solution.selected_orders))
        
        # Limitar tamanho da lista TABU
        while len(self.tabu_list) > self.tabu_tenure:
            self.tabu_list.pop(0)
        
        # Limitar tamanho do conjunto de hashes
        if len(self.tabu_hashes) > self.tabu_tenure * 3:
            # Manter apenas os hashes mais recentes
            excess = len(self.tabu_hashes) - self.tabu_tenure * 2
            self.tabu_hashes = set(list(self.tabu_hashes)[excess:])

    def batch_perturb(self, solution, num_perturbations=10, intensity=0.2):
        """Gera e avalia múltiplas perturbações em paralelo usando GPU."""
        if not self.is_initialized:
            self.initialize()
        
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
                size=n_perturb, 
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
        
        # Avaliar perturbações em lote usando GPU
        results = self.evaluate_solution_batch(perturbed_masks)
        
        # Selecionar a melhor perturbação viável
        best_value = solution.objective_value
        best_mask_idx = -1
        
        for i, (value, aisle_mask, is_feasible) in enumerate(results):
            if is_feasible and value > best_value:
                best_value = value
                best_mask_idx = i
        
        # Retornar a melhor solução encontrada
        if best_mask_idx >= 0:
            return self._create_solution_from_masks(
                perturbed_masks[best_mask_idx], 
                results[best_mask_idx][1], 
                best_value
            )
        
        return solution

    def evaluate_solution_batch(self, perturbed_masks):
        """
        Avalia múltiplas soluções em paralelo usando GPU.
        
        Args:
            perturbed_masks: Lista de máscaras binárias representando soluções
            
        Returns:
            list: Lista de tuplas (valor_objetivo, máscara_corredores, é_viável)
        """
        if not hasattr(self, 'gpu_manager') or not self.gpu_manager:
            # Fallback para CPU se não tiver GPU
            results = []
            for mask in perturbed_masks:
                # Converter máscara para lista de pedidos
                selected_orders = [i for i, val in enumerate(mask) if val == 1]
                
                # Calcular corredores necessários
                if hasattr(self, 'local_search'):
                    visited_aisles = self.local_search._calculate_required_aisles(selected_orders)
                else:
                    from src.solvers.heuristic.local_search import LocalSearch
                    local_search = LocalSearch(self.problem, self.config)
                    visited_aisles = local_search._calculate_required_aisles(selected_orders)
                
                # Criar solução
                solution = self.problem.create_solution(selected_orders, visited_aisles)
                solution.is_feasible = self._is_solution_feasible(solution)
                
                # Calcular valor objetivo se viável
                if solution.is_feasible:
                    solution.objective_value = self._compute_objective_function(solution)
                    aisle_mask = np.zeros(self.problem.n_aisles, dtype=np.int32)
                    for a in visited_aisles:
                        aisle_mask[a] = 1
                    results.append((solution.objective_value, aisle_mask, True))
                else:
                    results.append((0.0, None, False))
            
            return results
        
        # Usar GPU para avaliação em lote
        import cupy as cp
        
        # Preparar dados GPU
        masks_gpu = [cp.array(mask) for mask in perturbed_masks]
        results = []
        
        for mask_gpu in masks_gpu:
            # Calcular demanda de itens
            item_demand = cp.matmul(self.gpu_manager.gpu_data['item_order_matrix'], mask_gpu)
            
            # Calcular corredores necessários
            required_aisles = cp.zeros(self.problem.n_aisles, dtype=cp.int32)
            
            # Determinar quais itens têm demanda
            items_with_demand = (item_demand > 0)
            
            # Corredor exclusivo - itens que só aparecem em um corredor
            item_aisle_matrix = self.gpu_manager.gpu_data['item_aisle_matrix']
            item_aisle_count = cp.sum(item_aisle_matrix > 0, axis=1)
            exclusive_items = items_with_demand & (item_aisle_count == 1)
            
            # Adicionar corredores exclusivos
            for item_idx in cp.where(exclusive_items)[0]:
                aisle_idx = cp.where(item_aisle_matrix[item_idx] > 0)[0]
                if len(aisle_idx) > 0:
                    required_aisles[aisle_idx[0]] = 1
            
            # Cobrir os itens restantes
            covered_items = cp.zeros_like(items_with_demand, dtype=cp.bool_)
            for a_idx in cp.where(required_aisles)[0]:
                covered_items = covered_items | ((item_aisle_matrix[:, a_idx] > 0) & items_with_demand)
            
            # Adicionar mais corredores até cobrir todos os itens
            while cp.any(items_with_demand & ~covered_items):
                best_aisle = -1
                best_coverage = 0
                
                for a_idx in range(self.problem.n_aisles):
                    if required_aisles[a_idx] == 1:
                        continue
                        
                    new_covered = ((item_aisle_matrix[:, a_idx] > 0) & items_with_demand & ~covered_items)
                    coverage = cp.sum(new_covered)
                    
                    if coverage > best_coverage:
                        best_coverage = coverage
                        best_aisle = a_idx
                
                if best_aisle == -1 or best_coverage == 0:
                    break
                    
                required_aisles[best_aisle] = 1
                covered_items = covered_items | ((item_aisle_matrix[:, best_aisle] > 0) & items_with_demand)
            
            # Verificar viabilidade
            total_units = float(cp.sum(self.gpu_manager.gpu_data['order_units'] * mask_gpu))
            is_feasible = (
                self.problem.wave_size_lb <= total_units <= self.problem.wave_size_ub and
                cp.all(items_with_demand == covered_items)
            )
            
            # Calcular valor objetivo
            if is_feasible:
                total_aisles = float(cp.sum(required_aisles))
                obj_value = self.problem.calc_objective_value(total_units, total_aisles)
                results.append((obj_value, required_aisles.get(), True))
            else:
                results.append((0.0, None, False))
        
        return results

    def _create_solution_from_masks(self, orders_mask, aisles_mask, objective_value):
        """
        Cria um objeto Solution a partir de máscaras binárias.
        
        Args:
            orders_mask: Máscara binária dos pedidos selecionados
            aisles_mask: Máscara binária dos corredores necessários
            objective_value: Valor da função objetivo
            
        Returns:
            WaveOrderPickingSolution: Objeto de solução
        """
        # Converter máscaras para listas
        selected_orders = [i for i, val in enumerate(orders_mask) if val == 1]
        visited_aisles = [i for i, val in enumerate(aisles_mask) if val == 1]
        
        # Criar solução
        solution = self.problem.create_solution(selected_orders, visited_aisles)
        solution.is_feasible = True  # Já verificamos viabilidade
        solution.objective_value = objective_value
        
        # Calcular total de unidades
        total_units = 0
        for o_id in selected_orders:
            total_units += self.problem.order_units.get(o_id, 0)
        solution.set_total_units(total_units)
        
        return solution