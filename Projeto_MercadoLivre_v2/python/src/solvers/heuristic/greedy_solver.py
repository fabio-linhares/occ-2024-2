# -*- coding: utf-8 -*-

import time
import random
import numpy as np
from src.solvers.base_solver import BaseSolver
from src.models.solution import WaveOrderPickingSolution

class GreedySolver(BaseSolver):
    """
    Solver heurístico guloso para o problema de Wave Order Picking.
    """
    
    def __init__(self, problem, config=None):
        """
        Inicializa o solver heurístico guloso.
        
        Args:
            problem: O problema a ser resolvido
            config (dict, optional): Configurações para o solver
        """
        super().__init__(problem, config)
        self.seed = self.config.get('algorithm', {}).get('seed', 42)
        random.seed(self.seed)
    
    def solve(self, start_time=None):
        """
        Resolve o problema usando uma heurística gulosa melhorada.
        
        Args:
            start_time (float, opcional): Tempo de início da execução
            
        Returns:
            WaveOrderPickingSolution: A solução encontrada
        """
        start_time = start_time or time.time()
        
        print("Resolvendo com heurística gulosa melhorada...")
        
        # Tentar várias estratégias diferentes para encontrar uma solução viável
        strategies = [
            self._solve_density_based,      # Estratégia baseada em densidade
            self._solve_item_coverage_based, # Estratégia baseada em cobertura de itens
            self._solve_random_sampling,     # Estratégia com amostragem aleatória
            self._solve_aisle_first          # Estratégia que prioriza corredores
        ]
        
        best_solution = None
        best_objective = 0.0
        
        for i, strategy in enumerate(strategies):
            if self.check_timeout(start_time):
                print(f"Tempo limite atingido após testar {i} estratégias.")
                break
                
            print(f"Testando estratégia {i+1}/{len(strategies)}...")
            solution = strategy(start_time)
            
            if solution and solution.is_feasible and (not best_solution or solution.objective_value > best_objective):
                best_solution = solution
                best_objective = solution.objective_value
                print(f"Nova melhor solução encontrada: objetivo = {best_objective:.4f}")
        
        if best_solution:
            print(f"Melhor solução heurística: {best_objective:.4f}")
            return best_solution
        
        print("Nenhuma solução heurística viável encontrada.")
        return self.problem.create_solution([], [])
    
    def _solve_density_based(self, start_time):
        """Estratégia baseada em densidade (unidades por item)"""
        # Calcular densidade para cada pedido (unidades/itens)
        orders_with_data = []
        for o_id in range(self.problem.n_orders):
            order_data = self.problem.orders.get(o_id, {})
            units = sum(order_data.values())
            items = len(order_data)
            density = units / max(1, items)
            orders_with_data.append((o_id, units, density))
        
        # Ordenar por densidade decrescente
        orders_with_data.sort(key=lambda x: x[2], reverse=True)
        
        # Inicializar solução vazia
        selected_orders = []
        visited_aisles = set()
        total_units = 0
        
        # Adicionar pedidos em ordem de densidade até atingir o limite inferior
        for o_id, units, _ in orders_with_data:
            if self.check_timeout(start_time):
                return None
                
            # Se já atingimos o limite superior, não adicionar mais pedidos
            if total_units + units > self.problem.wave_size_ub:
                continue
                
            # Adicionar o pedido
            selected_orders.append(o_id)
            total_units += units
            
            # Atualizar corredores necessários
            for item_id, _ in self.problem.orders.get(o_id, {}).items():
                for aisle_id in self.problem.item_units_by_aisle.get(item_id, {}).keys():
                    visited_aisles.add(aisle_id)
            
            # Se atingimos o limite inferior, podemos parar
            if total_units >= self.problem.wave_size_lb:
                break
        
        # Se não atingimos o limite inferior, a solução não é viável
        if total_units < self.problem.wave_size_lb:
            # Tentar relaxar a condição de não ultrapassar o limite superior
            for o_id, units, _ in orders_with_data:
                if o_id in selected_orders:
                    continue
                    
                # Se adicionar este pedido vai exceder muito o limite, pular
                if total_units + units > self.problem.wave_size_ub * 1.2:  # Permitir 20% acima
                    continue
                    
                selected_orders.append(o_id)
                total_units += units
                
                # Atualizar corredores necessários
                for item_id, _ in self.problem.orders.get(o_id, {}).items():
                    for aisle_id in self.problem.item_units_by_aisle.get(item_id, {}).keys():
                        visited_aisles.add(aisle_id)
                
                if total_units >= self.problem.wave_size_lb:
                    break
        
        # Verificar se a solução é viável
        solution = self.problem.create_solution(selected_orders, list(visited_aisles))
        solution.is_feasible = self._is_solution_feasible(solution)
        
        if solution.is_feasible:
            solution.objective_value = self._compute_objective_function(solution)
            
        return solution
    
    def _solve_item_coverage_based(self, start_time):
        """Estratégia que prioriza cobrir itens com menor número de corredores"""
        # Mapeamento de itens para número de corredores que os contêm
        item_aisle_count = {}
        for item_id in self.problem.all_order_items:
            item_aisle_count[item_id] = len(self.problem.item_units_by_aisle.get(item_id, {}))
        
        # Ordenar pedidos por prioridade (pedidos com itens que aparecem em menos corredores)
        orders_with_scores = []
        for o_id in range(self.problem.n_orders):
            order_items = self.problem.orders.get(o_id, {})
            if not order_items:
                continue
                
            # Calcular pontuação (inversa da média do número de corredores por item)
            aisle_counts = [item_aisle_count.get(item_id, 1) for item_id in order_items]
            avg_aisle_count = sum(aisle_counts) / len(aisle_counts)
            score = 1.0 / max(0.001, avg_aisle_count)  # Evitar divisão por zero
            units = self.problem.order_units.get(o_id, 0)
            
            orders_with_scores.append((o_id, units, score))
        
        # Ordenar por pontuação decrescente
        orders_with_scores.sort(key=lambda x: x[2], reverse=True)
        
        # Inicializar solução
        selected_orders = []
        needed_items = {}  # Itens necessários para os pedidos selecionados
        visited_aisles = set()
        total_units = 0
        
        # Adicionar pedidos em ordem de prioridade
        for o_id, units, _ in orders_with_scores:
            if self.check_timeout(start_time):
                return None
                
            # Se já atingimos o limite superior, não adicionar mais pedidos
            if total_units + units > self.problem.wave_size_ub:
                continue
                
            # Adicionar o pedido
            selected_orders.append(o_id)
            total_units += units
            
            # Atualizar itens necessários
            for item_id, quantity in self.problem.orders.get(o_id, {}).items():
                needed_items[item_id] = needed_items.get(item_id, 0) + quantity
            
            # Verificar se atingimos o limite inferior
            if total_units >= self.problem.wave_size_lb:
                break
        
        # Se não atingimos o limite inferior, tentar adicionar mais pedidos
        if total_units < self.problem.wave_size_lb:
            # Tentar relaxar a condição de não ultrapassar o limite superior
            for o_id, units, _ in orders_with_scores:
                if o_id in selected_orders:
                    continue
                    
                selected_orders.append(o_id)
                total_units += units
                
                # Atualizar itens necessários
                for item_id, quantity in self.problem.orders.get(o_id, {}).items():
                    needed_items[item_id] = needed_items.get(item_id, 0) + quantity
                
                if total_units >= self.problem.wave_size_lb:
                    break
        
        # Selecionar corredores necessários (baseado nos itens necessários)
        item_coverage = {item_id: 0 for item_id in needed_items}
        aisle_scores = {}
        
        # Calcular pontuação para cada corredor (quantos itens novos ele cobre)
        for a_id in range(self.problem.n_aisles):
            aisle_items = self.problem.aisles.get(a_id, {})
            new_items_covered = 0
            
            for item_id in aisle_items:
                if item_id in needed_items and item_coverage.get(item_id, 0) < needed_items[item_id]:
                    new_items_covered += 1
            
            if new_items_covered > 0:
                aisle_scores[a_id] = new_items_covered
        
        # Adicionar corredores em ordem de pontuação até cobrir todos os itens
        while aisle_scores:
            if self.check_timeout(start_time):
                return None
                
            # Selecionar corredor com maior pontuação
            best_aisle = max(aisle_scores, key=aisle_scores.get)
            visited_aisles.add(best_aisle)
            
            # Atualizar cobertura de itens
            for item_id, quantity in self.problem.aisles.get(best_aisle, {}).items():
                if item_id in needed_items:
                    item_coverage[item_id] += quantity
            
            # Recalcular pontuações
            aisle_scores = {}
            all_covered = True
            
            for a_id in range(self.problem.n_aisles):
                if a_id in visited_aisles:
                    continue
                    
                aisle_items = self.problem.aisles.get(a_id, {})
                new_items_covered = 0
                
                for item_id in aisle_items:
                    if item_id in needed_items and item_coverage[item_id] < needed_items[item_id]:
                        new_items_covered += 1
                        all_covered = False
                
                if new_items_covered > 0:
                    aisle_scores[a_id] = new_items_covered
            
            # Se todos os itens estão cobertos, podemos parar
            if all_covered:
                break
        
        # Verificar se a solução é viável
        solution = self.problem.create_solution(selected_orders, list(visited_aisles))
        solution.is_feasible = self._is_solution_feasible(solution)
        
        if solution.is_feasible:
            solution.objective_value = self._compute_objective_function(solution)
            
        return solution
    
    def _solve_random_sampling(self, start_time):
        """Estratégia que faz múltiplas tentativas com seleção aleatória"""
        max_attempts = min(100, self.problem.n_orders * 2)  # Limitar número de tentativas
        best_solution = None
        best_objective = 0.0
        
        for attempt in range(max_attempts):
            if self.check_timeout(start_time):
                break
                
            # Determinar quantos pedidos selecionar
            target_orders = random.randint(max(1, self.problem.n_orders // 4), 
                                        min(self.problem.n_orders, 
                                            self.problem.n_orders // 2 + self.problem.n_orders // 4))
            
            # Selecionar pedidos aleatoriamente
            selected_orders = random.sample(range(self.problem.n_orders), target_orders)
            
            # Calcular unidades totais
            total_units = sum(self.problem.order_units.get(o, 0) for o in selected_orders)
            
            # Se estamos muito abaixo do limite inferior ou muito acima do superior, continuar
            if total_units < self.problem.wave_size_lb * 0.7 or total_units > self.problem.wave_size_ub * 1.3:
                continue
            
            # Encontrar itens necessários
            needed_items = {}
            for o_id in selected_orders:
                for item_id, quantity in self.problem.orders.get(o_id, {}).items():
                    needed_items[item_id] = needed_items.get(item_id, 0) + quantity
            
            # Selecionar corredores necessários
            visited_aisles = set()
            item_coverage = {item_id: 0 for item_id in needed_items}
            
            # Randomizar ordem dos corredores
            aisle_list = list(range(self.problem.n_aisles))
            random.shuffle(aisle_list)
            
            for a_id in aisle_list:
                aisle_items = self.problem.aisles.get(a_id, {})
                useful = False
                
                for item_id, quantity in aisle_items.items():
                    if item_id in needed_items and item_coverage[item_id] < needed_items[item_id]:
                        useful = True
                        break
                
                if useful:
                    visited_aisles.add(a_id)
                    for item_id, quantity in aisle_items.items():
                        if item_id in needed_items:
                            item_coverage[item_id] += quantity
            
            # Verificar se a solução é viável
            solution = self.problem.create_solution(selected_orders, list(visited_aisles))
            solution.is_feasible = self._is_solution_feasible(solution)
            
            if solution.is_feasible:
                solution.objective_value = self._compute_objective_function(solution)
                
                if solution.objective_value > best_objective:
                    best_solution = solution
                    best_objective = solution.objective_value
        
        return best_solution
    
    def _solve_aisle_first(self, start_time):
        """Estratégia que primeiro seleciona corredores eficientes e depois pedidos compatíveis"""
        # Calcular densidade de cada corredor (unidades disponíveis / número de itens)
        aisle_density = {}
        for a_id in range(self.problem.n_aisles):
            aisle_items = self.problem.aisles.get(a_id, {})
            if not aisle_items:
                continue
            total_units = sum(aisle_items.values())
            density = total_units / len(aisle_items) if aisle_items else 0
            aisle_density[a_id] = density
        
        # Ordenar corredores por densidade
        sorted_aisles = sorted(aisle_density.keys(), key=lambda a: aisle_density[a], reverse=True)
        
        # Selecionar alguns corredores promissores
        num_aisles_to_select = min(self.problem.n_aisles // 3 + 1, self.problem.n_aisles)
        candidate_aisles = sorted_aisles[:num_aisles_to_select]
        
        # Encontrar quais itens estão disponíveis nos corredores selecionados
        available_items = {}
        for a_id in candidate_aisles:
            for item_id, quantity in self.problem.aisles.get(a_id, {}).items():
                available_items[item_id] = available_items.get(item_id, 0) + quantity
        
        # Calcular compatibilidade de cada pedido com os itens disponíveis
        order_scores = {}
        for o_id in range(self.problem.n_orders):
            order_items = self.problem.orders.get(o_id, {})
            if not order_items:
                continue
                
            # Calcular quantos itens do pedido estão disponíveis nos corredores selecionados
            items_covered = sum(1 for item_id in order_items if item_id in available_items)
            coverage_ratio = items_covered / len(order_items) if order_items else 0
            units = self.problem.order_units.get(o_id, 0)
            
            # Pontuação: cobertura e unidades
            order_scores[o_id] = (coverage_ratio, units)
        
        # Ordenar pedidos por cobertura (decrescente) e depois por unidades (decrescente)
        sorted_orders = sorted(order_scores.keys(), 
                             key=lambda o: (order_scores[o][0], order_scores[o][1]), 
                             reverse=True)
        
        # Selecionar pedidos até atingir o limite inferior
        selected_orders = []
        total_units = 0
        
        for o_id in sorted_orders:
            if self.check_timeout(start_time):
                break
                
            units = self.problem.order_units.get(o_id, 0)
            
            # Se já vamos exceder o limite superior, pular este pedido
            if total_units + units > self.problem.wave_size_ub:
                continue
                
            # Adicionar o pedido
            selected_orders.append(o_id)
            total_units += units
            
            # Se atingimos o limite inferior, podemos parar
            if total_units >= self.problem.wave_size_lb:
                break
        
        # Se não atingimos o limite inferior, tentar adicionar mais pedidos
        if total_units < self.problem.wave_size_lb:
            for o_id in sorted_orders:
                if o_id in selected_orders:
                    continue
                    
                units = self.problem.order_units.get(o_id, 0)
                
                # Se exceder muito o limite superior, pular
                if total_units + units > self.problem.wave_size_ub * 1.1:
                    continue
                    
                selected_orders.append(o_id)
                total_units += units
                
                if total_units >= self.problem.wave_size_lb:
                    break
        
        # Inicializar needed_items ANTES de usá-lo
        needed_items = {}
        
        # Verificar quais corredores são realmente necessários
        for o_id in selected_orders:
            for item_id, quantity in self.problem.orders.get(o_id, {}).items():
                needed_items[item_id] = needed_items.get(item_id, 0) + quantity
        
        # Selecionar apenas os corredores necessários
        visited_aisles = []
        item_coverage = {item_id: 0 for item_id in needed_items}
        
        for a_id in range(self.problem.n_aisles):
            aisle_items = self.problem.aisles.get(a_id, {})
            adds_coverage = False
            
            for item_id, aisle_quantity in aisle_items.items():
                if item_id in needed_items and item_coverage.get(item_id, 0) < needed_items[item_id]:
                    adds_coverage = True
                    break
                    
            if adds_coverage:
                visited_aisles.append(a_id)
                
                # Atualizar cobertura de itens
                for item_id, aisle_quantity in aisle_items.items():
                    if item_id in item_coverage:
                        item_coverage[item_id] += aisle_quantity
        
        # Verificar se a solução é viável
        solution = self.problem.create_solution(selected_orders, visited_aisles)
        solution.is_feasible = self._is_solution_feasible(solution)
        
        if solution.is_feasible:
            solution.objective_value = self._compute_objective_function(solution)
            
        return solution