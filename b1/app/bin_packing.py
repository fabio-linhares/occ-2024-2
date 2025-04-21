"""
Módulo de Bin Packing
====================

Este módulo implementa as estruturas de dados e algoritmos para resolver 
o problema de Bin Packing usando diversas heurísticas e meta-heurísticas.
"""

import random
import time
import math
import numpy as np
from dataclasses import dataclass
from typing import List, Set, Dict, Tuple, Optional, Any, Union
import matplotlib as plt


@dataclass
class Item:
    """Representa um item para o problema de bin packing."""
    id: int
    size: float
    
    def __eq__(self, other):
        if not isinstance(other, Item):
            return False
        return self.id == other.id
    
    def __hash__(self):
        return hash(self.id)


class Bin:
    """Representa um bin (recipiente) para o problema de bin packing."""
    
    def __init__(self, id: int, capacity: float = 1.0):
        self.id = id
        self.items = set()  # Conjunto de itens no bin
        self.current_load = 0.0
        self.capacity = capacity
    
    def can_fit(self, item: Item) -> bool:
        """Verifica se um item cabe neste bin."""
        return self.current_load + item.size <= self.capacity
    
    def add_item(self, item: Item) -> bool:
        """Adiciona um item ao bin se houver espaço."""
        if not self.can_fit(item):
            return False
        
        self.items.add(item)
        self.current_load += item.size
        return True
    
    def remove_item(self, item_id: int) -> Optional[Item]:
        """Remove um item do bin e retorna o item removido."""
        for item in self.items:
            if item.id == item_id:
                self.items.remove(item)
                self.current_load -= item.size
                return item
        return None
    
    def get_occupation(self) -> float:
        """Retorna a ocupação do bin (entre 0 e 1)."""
        return self.current_load / self.capacity
    
    def __str__(self):
        return f"Bin {self.id} - Ocupação: {self.current_load:.2f}/{self.capacity:.2f}"


class Solution:
    """Representa uma solução para o problema de bin packing."""
    
    def __init__(self, alpha: float = 0.1):
        self.bins = []
        self.next_bin_id = 0
        self.alpha = alpha  # Parâmetro de penalização para a função de avaliação
    
    def create_bin(self) -> Bin:
        """Cria um novo bin e o adiciona à solução."""
        new_bin = Bin(self.next_bin_id)
        self.bins.append(new_bin)
        self.next_bin_id += 1
        return new_bin
    
    def evaluate(self) -> float:
        """
        Calcula a função de avaliação da solução.
        
        A função considera dois componentes:
        1. Número de bins utilizados (componente principal)
        2. Balanceamento de carga dos bins (componente secundário)
        """
        # Componente principal: número de bins
        main_component = len(self.bins)
        
        # Componente secundário: balanceamento de carga
        secondary_component = 0.0
        for bin in self.bins:
            occupation = bin.get_occupation()
            secondary_component += (1.0 - occupation) ** 2
        
        # Função de avaliação combinada
        return main_component + self.alpha * secondary_component
    
    def __str__(self):
        result = f"Solução com {len(self.bins)} bins - Avaliação: {self.evaluate():.4f}\n"
        for bin in self.bins:
            result += f"  {bin}\n"
        return result
    
    def copy(self) -> 'Solution':
        """Cria uma cópia profunda da solução atual."""
        new_solution = Solution(self.alpha)
        new_solution.next_bin_id = self.next_bin_id
        
        # Mapeamento de ids de bins para novos bins
        bin_mapping = {}
        
        # Cria novos bins
        for original_bin in self.bins:
            new_bin = Bin(original_bin.id, original_bin.capacity)
            new_solution.bins.append(new_bin)
            bin_mapping[original_bin.id] = new_bin
            
            # Adiciona os itens ao novo bin
            for item in original_bin.items:
                new_item = Item(item.id, item.size)
                new_bin.add_item(new_item)
        
        return new_solution

    def verify(self) -> bool:
        """
        Verifica se a solução é válida:
        1. Cada item cabe em seu bin
        2. Nenhum bin está com sobrecarga
        
        Returns:
            bool: True se a solução for válida, False caso contrário
        """
        for bin in self.bins:
            # Verificar se o bin não excede a capacidade
            if bin.current_load > bin.capacity:
                return False
            
            # Verificar se cada item realmente cabe no bin
            for item in bin.items:
                if item.size > bin.capacity:
                    return False
        
        return True


class LocalSearch:
    """Implementa a busca local para o problema de bin packing."""
    
    def __init__(self, time_limit_seconds: int):
        self.time_limit = time_limit_seconds
    
    def run(self, initial_solution: Solution) -> Tuple[Solution, List[Tuple[int, int, float]]]:
        """
        Executa a busca local a partir de uma solução inicial.
        
        Args:
            initial_solution: Solução inicial para a busca local.
            
        Returns:
            A melhor solução encontrada dentro do limite de tempo e o histórico de melhoria.
        """
        current_solution = initial_solution.copy()
        best_solution = current_solution.copy()
        
        # Marca o tempo de início
        start_time = time.time()
        
        improved = True
        iterations = 0
        
        # Histórico de melhoria para visualização
        history = []
        history.append((iterations, len(best_solution.bins), best_solution.evaluate()))
        
        print("Iniciando busca local...")
        
        while improved:
            # Verifica se o tempo limite foi atingido
            current_time = time.time()
            elapsed_time = current_time - start_time
            
            if elapsed_time >= self.time_limit:
                print(f"Tempo limite atingido após {iterations} iterações.")
                break
            
            improved = False
            iterations += 1
            
            # Encontra o melhor movimento
            best_move, best_delta = self._find_best_move(current_solution)
            
            # Se encontrou um movimento de melhoria
            if best_move and best_delta < 0:
                # Aplica o movimento
                if self._apply_move(current_solution, best_move):
                    improved = True
                    
                    # Verifica se a nova solução é melhor que a melhor encontrada até agora
                    current_eval = current_solution.evaluate()
                    best_eval = best_solution.evaluate()
                    
                    if current_eval < best_eval:
                        best_solution = current_solution.copy()
                        print(f"Nova melhor solução encontrada na iteração {iterations} "
                              f"com {len(best_solution.bins)} bins.")
                        
                        # Adiciona ao histórico
                        history.append((iterations, len(best_solution.bins), best_solution.evaluate()))
        
        print(f"Busca local concluída após {iterations} iterações.")
        
        return best_solution, history
    
    def _find_best_move(self, solution: Solution) -> Tuple[Optional[Dict], float]:
        """
        Busca o melhor movimento na vizinhança da solução atual.
        
        Considera dois tipos de movimentos:
        1. Movimento de realocação: mover um item de um bin para outro
        2. Movimento de troca: trocar itens entre dois bins
        
        Returns:
            Uma tupla contendo o melhor movimento e seu delta de avaliação.
            Se não houver movimento de melhoria, retorna (None, 0).
        """
        best_delta = 0  # Inicializa com 0 para aceitar apenas movimentos de melhoria
        best_move = None
        
        # Explora todos os movimentos possíveis
        for i, bin1 in enumerate(solution.bins):
            # 1. Movimento de realocação
            for item in bin1.items:
                for j, bin2 in enumerate(solution.bins):
                    if i == j:
                        continue  # Não realoca no mesmo bin
                    
                    # Verifica se o item cabe no bin de destino
                    if bin2.can_fit(item):
                        # Avalia o movimento
                        temp_solution = solution.copy()
                        temp_bin1 = temp_solution.bins[i]
                        temp_bin2 = temp_solution.bins[j]
                        
                        # Remove o item do bin de origem
                        removed_item = temp_bin1.remove_item(item.id)
                        if not removed_item:
                            continue
                        
                        # Adiciona ao bin de destino
                        temp_bin2.add_item(removed_item)
                        
                        # Calcula o delta (variação na função de avaliação)
                        before_eval = solution.evaluate()
                        after_eval = temp_solution.evaluate()
                        delta = after_eval - before_eval
                        
                        if delta < best_delta:
                            best_delta = delta
                            best_move = {
                                'type': 'relocate',
                                'bin_from': i,
                                'bin_to': j,
                                'item_id': item.id
                            }
            
            # 2. Movimento de troca
            for j in range(i + 1, len(solution.bins)):
                bin2 = solution.bins[j]
                
                for item1 in bin1.items:
                    for item2 in bin2.items:
                        # Verifica se a troca é viável
                        if (bin1.current_load - item1.size + item2.size <= bin1.capacity and
                            bin2.current_load - item2.size + item1.size <= bin2.capacity):
                            
                            # Avalia o movimento
                            temp_solution = solution.copy()
                            temp_bin1 = temp_solution.bins[i]
                            temp_bin2 = temp_solution.bins[j]
                            
                            # Remove os itens dos bins originais
                            removed_item1 = temp_bin1.remove_item(item1.id)
                            removed_item2 = temp_bin2.remove_item(item2.id)
                            
                            if not (removed_item1 and removed_item2):
                                continue
                            
                            # Adiciona os itens aos bins trocados
                            temp_bin1.add_item(removed_item2)
                            temp_bin2.add_item(removed_item1)
                            
                            # Calcula o delta
                            before_eval = solution.evaluate()
                            after_eval = temp_solution.evaluate()
                            delta = after_eval - before_eval
                            
                            if delta < best_delta:
                                best_delta = delta
                                best_move = {
                                    'type': 'swap',
                                    'bin_1': i,
                                    'bin_2': j,
                                    'item_1': item1.id,
                                    'item_2': item2.id
                                }
        
        return best_move, best_delta
    
    def _apply_move(self, solution: Solution, move: Dict) -> bool:
        """Aplica um movimento à solução."""
        if move['type'] == 'relocate':
            bin_from = solution.bins[move['bin_from']]
            bin_to = solution.bins[move['bin_to']]
            
            # Remove o item do bin de origem
            item = bin_from.remove_item(move['item_id'])
            if not item:
                return False
            
            # Adiciona ao bin de destino
            return bin_to.add_item(item)
            
        elif move['type'] == 'swap':
            bin1 = solution.bins[move['bin_1']]
            bin2 = solution.bins[move['bin_2']]
            
            # Remove os itens dos bins
            item1 = bin1.remove_item(move['item_1'])
            item2 = bin2.remove_item(move['item_2'])
            
            if not (item1 and item2):
                return False
            
            # Adiciona os itens trocando os bins
            success = bin1.add_item(item2) and bin2.add_item(item1)
            
            if not success:
                # Reverte se não conseguir adicionar
                bin1.add_item(item1)
                bin2.add_item(item2)
                return False
                
            return True
            
        return False


def first_fit_decreasing(items: List[Item], max_bins: Optional[int] = None) -> Solution:
    """
    Implementa o algoritmo First Fit Decreasing para construir uma solução inicial.
    
    Este algoritmo ordena os itens por tamanho (do maior para o menor) e então
    os aloca no primeiro bin onde eles couberem.
    
    Args:
        items: Lista de itens a serem alocados.
        max_bins: Número máximo de bins permitidos. Se None, não há limite.
        
    Returns:
        Solução construída pelo algoritmo FFD.
    """
    solution = Solution()
    
    # Ordena os itens por tamanho (decrescente)
    sorted_items = sorted(items, key=lambda x: x.size, reverse=True)
    
    for item in sorted_items:
        placed = False
        
        # Tenta colocar o item em um bin existente
        for bin in solution.bins:
            if bin.can_fit(item):
                bin.add_item(item)
                placed = True
                break
        
        # Se não conseguir colocar em nenhum bin existente e não exceder o limite, cria um novo
        if not placed:
            if max_bins is not None and len(solution.bins) >= max_bins:
                # Se atingiu o limite de bins, tenta forçar no último bin (pode violar a capacidade)
                solution.bins[-1].items.add(item)
                solution.bins[-1].current_load += item.size
            else:
                new_bin = solution.create_bin()
                new_bin.add_item(item)
    
    return solution


def read_instance(file_path: str) -> List[Item]:
    """
    Lê uma instância do problema de bin packing a partir de um arquivo.
    
    Formato do arquivo:
    - Primeira linha: número de itens (n)
    - Próximas n linhas: tamanho de cada item (entre 0 e 1)
    
    Args:
        file_path: Caminho para o arquivo da instância.
        
    Returns:
        Lista de itens lidos do arquivo.
    """
    items = []
    
    try:
        with open(file_path, 'r') as f:
            lines = f.readlines()
            
            # Primeira linha contém o número de itens
            n = int(lines[0].strip())
            
            # Próximas n linhas contêm os tamanhos dos itens
            for i in range(1, n + 1):
                if i < len(lines):
                    size = float(lines[i].strip())
                    items.append(Item(i - 1, size))
    
    except Exception as e:
        print(f"Erro ao ler o arquivo: {e}")
    
    return items


def load_instance_from_buffer(file_buffer) -> List[Item]:
    """
    Lê uma instância do problema de bin packing a partir de um buffer de arquivo.
    
    Args:
        file_buffer: Buffer de arquivo (como o retornado pelo st.file_uploader)
        
    Returns:
        Lista de itens lidos do buffer.
    """
    items = []
    item_id = 0
    
    for line in file_buffer:
        try:
            size = float(line.decode('utf-8').strip())
            if 0 <= size <= 1:
                items.append(Item(item_id, size))
                item_id += 1
        except ValueError:
            continue
    
    return items


def save_solution(solution: Solution, instance_name: str, time_limit: int, file_path: str):
    """
    Salva a solução em um arquivo de texto.
    
    Args:
        solution: Solução a ser salva.
        instance_name: Nome da instância.
        time_limit: Tempo limite em segundos.
        file_path: Caminho para o arquivo de saída.
    """
    try:
        with open(file_path, 'w') as f:
            # Escreve os resultados
            f.write(f"Instância: {instance_name}\n")
            f.write(f"Tempo limite: {time_limit} segundos\n")
            f.write(f"Número de bins: {len(solution.bins)}\n")
            f.write(f"Valor da função de avaliação: {solution.evaluate():.4f}\n\n")
            
            # Escreve os detalhes de cada bin
            for i, bin in enumerate(solution.bins):
                f.write(f"Bin {i} (Ocupação: {bin.current_load:.4f}): ")
                
                for item in sorted(bin.items, key=lambda x: x.id):
                    f.write(f"{item.id}({item.size:.4f}) ")
                
                f.write("\n")
        
        print(f"Resultados salvos em: {file_path}")
        
    except Exception as e:
        print(f"Erro ao salvar o arquivo: {e}")


def generate_random_instance(n: int, min_size: float = 0.1, max_size: float = 0.7) -> List[Item]:
    """
    Gera uma instância aleatória do problema de bin packing.
    
    Args:
        n: Número de itens.
        min_size: Tamanho mínimo dos itens.
        max_size: Tamanho máximo dos itens.
        
    Returns:
        Lista de itens gerados aleatoriamente.
    """
    items = []
    for i in range(n):
        size = round(random.uniform(min_size, max_size), 2)
        items.append(Item(i, size))
    
    return items


def verificar_viabilidade_inicial(itens: List[Item], capacidade: float, num_bins: int) -> Tuple[bool, int]:
    """
    Verifica se o número de bins é suficiente para uma solução viável.
    
    Args:
        itens: Lista de itens.
        capacidade: Capacidade de cada bin.
        num_bins: Número de bins disponíveis.
        
    Returns:
        Tupla com (viabilidade, número mínimo de bins teórico).
    """
    soma_total = sum(item.size for item in itens)
    min_bins_teorico = math.ceil(soma_total / capacidade)
    
    if num_bins < min_bins_teorico:
        return False, min_bins_teorico
    return True, min_bins_teorico


# Adicionando a classe da meta-heurística híbrida
class HybridMetaheuristic:
    """Implementa uma meta-heurística híbrida para o problema de bin packing."""
    
    def __init__(self, time_limit_seconds: int, use_sa: bool = True, 
                 improvement: str = "best", alpha: float = 0.1):
        """
        Inicializa a meta-heurística híbrida.
        
        Args:
            time_limit_seconds: Tempo limite em segundos.
            use_sa: Se deve usar Simulated Annealing para aceitar soluções piores.
            improvement: Estratégia de melhoria ('best' ou 'first').
            alpha: Peso para o componente de balanceamento na função de avaliação.
        """
        self.time_limit = time_limit_seconds
        self.use_sa = use_sa
        self.improvement = improvement
        self.alpha = alpha
        
        # Parâmetros de SA
        self.initial_temp = 10.0
        self.cooling_rate = 0.95
        self.temp_min = 0.01
        
        # Parâmetros de ILS
        self.perturbation_strength_min = 0.1
        self.perturbation_strength_max = 0.5
    
    def run(self, items: List[Item], max_bins: Optional[int] = None) -> Tuple[Solution, dict, List[int]]:
        """
        Executa a meta-heurística híbrida para o problema de bin packing.
        
        Args:
            items: Lista de itens a serem alocados.
            max_bins: Número máximo de bins permitidos. Se None, não há limite.
            
        Returns:
            Tupla contendo (melhor solução, estatísticas, histórico).
        """
        start_time = time.time()
        
        # Solução inicial usando First Fit Decreasing
        current_solution = first_fit_decreasing(items, max_bins)
        best_solution = current_solution.copy()
        
        current_value = len(current_solution.bins)
        best_value = current_value
        
        iterations = 0
        non_improving_iterations = 0
        temperature = self.initial_temp
        history = [current_value]
        
        # Estatísticas
        stats = {
            "iterations": 0,
            "restarts": 0,
            "perturbations": 0,
            "improvements": 0,
            "time_to_best": 0,
            "acceptance_rate": 0,
            "solutions_evaluated": 0,
            "sa_acceptances": 0
        }
        
        solutions_tried = 0
        sa_acceptances = 0
        
        print(f"Iniciando meta-heurística híbrida com {current_value} bins...")
        
        # Critério de parada: tempo limite
        while time.time() - start_time < self.time_limit:
            iterations += 1
            stats["iterations"] = iterations
            improved = False
            
            # Busca Local
            neighbors = self._get_neighbors(current_solution, max_bins)
            stats["solutions_evaluated"] += len(neighbors)
            solutions_tried += len(neighbors)
            
            if not neighbors:
                # Se não houver vizinhos, perturbar a solução atual
                perturbation_strength = self.perturbation_strength_min + (
                    self.perturbation_strength_max - self.perturbation_strength_min
                ) * (non_improving_iterations / 50)
                perturbation_strength = min(self.perturbation_strength_max, perturbation_strength)
                
                current_solution = self._perturb_solution(current_solution, perturbation_strength, temperature)
                current_value = len(current_solution.bins)
                stats["perturbations"] += 1
                continue
            
            if self.improvement == "best":
                # Best improvement: avaliar todos os vizinhos e selecionar o melhor
                best_neighbor = None
                best_neighbor_value = float('inf')
                
                for neighbor in neighbors:
                    neighbor_value = len(neighbor.bins)
                    
                    # Se o número de bins for menor, ou igual com melhor balanceamento
                    if (neighbor_value < best_neighbor_value or 
                        (neighbor_value == best_neighbor_value and 
                         neighbor.evaluate() < (best_neighbor.evaluate() if best_neighbor else float('inf')))):
                        best_neighbor = neighbor
                        best_neighbor_value = neighbor_value
                
                if best_neighbor_value < current_value:
                    current_solution = best_neighbor
                    current_value = best_neighbor_value
                    improved = True
                    stats["improvements"] += 1
                    non_improving_iterations = 0
                    
                    if current_value < best_value:
                        best_solution = current_solution.copy()
                        best_value = current_value
                        stats["time_to_best"] = time.time() - start_time
                        print(f"Nova melhor solução encontrada na iteração {iterations} com {best_value} bins.")
                elif self.use_sa and self._accept_solution(current_value, best_neighbor_value, temperature):
                    # Aceitar solução pior baseado em critério SA
                    current_solution = best_neighbor
                    current_value = best_neighbor_value
                    stats["sa_acceptances"] += 1
                    sa_acceptances += 1
                    non_improving_iterations += 1
                else:
                    non_improving_iterations += 1
            else:  # First improvement
                # First improvement: selecionar o primeiro vizinho que melhora a solução
                for neighbor in neighbors:
                    neighbor_value = len(neighbor.bins)
                    
                    if neighbor_value < current_value:
                        current_solution = neighbor
                        current_value = neighbor_value
                        improved = True
                        stats["improvements"] += 1
                        non_improving_iterations = 0
                        
                        if current_value < best_value:
                            best_solution = current_solution.copy()
                            best_value = current_value
                            stats["time_to_best"] = time.time() - start_time
                            print(f"Nova melhor solução encontrada na iteração {iterations} com {best_value} bins.")
                        
                        break
                    elif self.use_sa and self._accept_solution(current_value, neighbor_value, temperature):
                        # Aceitar solução pior baseado em critério SA
                        current_solution = neighbor
                        current_value = neighbor_value
                        stats["sa_acceptances"] += 1
                        sa_acceptances += 1
                        non_improving_iterations += 1
                        break
                
                if not improved:
                    non_improving_iterations += 1
            
            history.append(current_value)
            
            # Resfriamento (SA) - reduzir temperatura
            temperature = max(self.temp_min, temperature * self.cooling_rate)
            
            # Estratégia de reinício: quando muitas iterações sem melhoria
            if non_improving_iterations > 50:
                # Reiniciar de uma solução inicial diferente
                current_solution = first_fit_decreasing(items, max_bins)
                current_value = len(current_solution.bins)
                temperature = self.initial_temp  # Reset temperatura
                non_improving_iterations = 0
                stats["restarts"] += 1
                print(f"Reiniciando busca na iteração {iterations}...")
        
        # Calcular taxa de aceitação do SA
        if solutions_tried > 0:
            stats["acceptance_rate"] = (sa_acceptances / solutions_tried) * 100
        
        print(f"Meta-heurística concluída após {iterations} iterações.")
        print(f"Melhor solução: {best_value} bins (avaliação: {best_solution.evaluate():.4f})")
        
        return best_solution, stats, history
    
    def _get_neighbors(self, solution: Solution, max_bins: Optional[int] = None) -> List[Solution]:
        """
        Gera os vizinhos de uma solução.
        
        Considera dois tipos de movimentos:
        1. Realocar um item de um bin para outro
        2. Trocar dois itens entre bins diferentes
        
        Args:
            solution: Solução atual.
            max_bins: Número máximo de bins permitidos.
            
        Returns:
            Lista de soluções vizinhas.
        """
        neighbors = []
        
        # 1. Movimento de realocação: mover um item de um bin para outro
        for i, bin1 in enumerate(solution.bins):
            for item in bin1.items:
                for j, bin2 in enumerate(solution.bins):
                    if i == j:
                        continue  # Não realoca no mesmo bin
                    
                    # Verifica se o item cabe no bin de destino
                    if bin2.can_fit(item):
                        # Cria uma nova solução com o movimento
                        new_solution = solution.copy()
                        new_bin1 = new_solution.bins[i]
                        new_bin2 = new_solution.bins[j]
                        
                        # Remove o item do bin de origem
                        removed_item = new_bin1.remove_item(item.id)
                        if not removed_item:
                            continue
                        
                        # Adiciona ao bin de destino
                        if new_bin2.add_item(removed_item):
                            # Se o bin de origem ficou vazio, remove-o
                            if len(new_bin1.items) == 0:
                                new_solution.bins.pop(i)
                            
                            neighbors.append(new_solution)
        
        # 2. Movimento de troca: trocar dois itens entre bins
        for i, bin1 in enumerate(solution.bins):
            for j in range(i + 1, len(solution.bins)):
                bin2 = solution.bins[j]
                
                for item1 in bin1.items:
                    for item2 in bin2.items:
                        # Verifica se a troca é viável
                        if (bin1.current_load - item1.size + item2.size <= bin1.capacity and
                            bin2.current_load - item2.size + item1.size <= bin2.capacity):
                            
                            # Cria uma nova solução com o movimento
                            new_solution = solution.copy()
                            new_bin1 = new_solution.bins[i]
                            new_bin2 = new_solution.bins[j]
                            
                            # Remove os itens dos bins originais
                            removed_item1 = new_bin1.remove_item(item1.id)
                            removed_item2 = new_bin2.remove_item(item2.id)
                            
                            if not (removed_item1 and removed_item2):
                                continue
                            
                            # Adiciona os itens aos bins trocados
                            if new_bin1.add_item(removed_item2) and new_bin2.add_item(removed_item1):
                                neighbors.append(new_solution)
        
        return neighbors
    
    def _perturb_solution(self, solution: Solution, perturbation_strength: float = 0.3, 
                           temperature: float = 1.0) -> Solution:
        """
        Perturba a solução atual para escapar de ótimos locais.
        Combina técnicas de ILS e Simulated Annealing.
        
        Args:
            solution: Solução atual.
            perturbation_strength: Intensidade da perturbação (0.0 a 1.0).
            temperature: Temperatura atual (influencia a intensidade da perturbação).
            
        Returns:
            Solução perturbada.
        """
        if not solution.bins:
            return solution
        
        new_solution = solution.copy()
        n_bins = len(new_solution.bins)
        
        if n_bins <= 1:
            return new_solution
        
        # Calcular número de perturbações baseado na força e temperatura
        n_perturbations = max(1, int(perturbation_strength * temperature * n_bins))
        
        for _ in range(n_perturbations):
            # Escolher tipo de perturbação
            perturbation_type = random.choices(
                ["move", "swap", "redistribute", "merge_split"],
                weights=[0.4, 0.3, 0.2, 0.1],
                k=1
            )[0]
            
            if perturbation_type == "move" and n_bins > 1:
                # Mover um item aleatório de um bin para outro
                self._perturb_move(new_solution)
            elif perturbation_type == "swap" and n_bins > 1:
                # Trocar itens entre dois bins aleatórios
                self._perturb_swap(new_solution)
            elif perturbation_type == "redistribute" and n_bins > 1:
                # Redistribuir todos os itens de um bin
                self._perturb_redistribute(new_solution)
            elif perturbation_type == "merge_split" and n_bins > 1:
                # Tentar mesclar dois bins e depois dividir em dois novamente
                self._perturb_merge_split(new_solution)
        
        # Remover bins vazios
        new_solution.bins = [bin for bin in new_solution.bins if bin.items]
        
        return new_solution
    
    def _perturb_move(self, solution: Solution) -> None:
        """Perturbação: move um item aleatório de um bin para outro."""
        n_bins = len(solution.bins)
        source_bin_idx = random.randint(0, n_bins - 1)
        source_bin = solution.bins[source_bin_idx]
        
        if not source_bin.items:
            return
            
        target_bin_idx = random.randint(0, n_bins - 1)
        while target_bin_idx == source_bin_idx:
            target_bin_idx = random.randint(0, n_bins - 1)
        
        target_bin = solution.bins[target_bin_idx]
        
        # Escolhe um item aleatório para mover
        item = random.choice(list(source_bin.items))
        source_bin.remove_item(item.id)
        
        # Tenta adicionar ao bin destino, se não couber, cria um novo
        if not target_bin.add_item(item):
            new_bin = solution.create_bin()
            new_bin.add_item(item)
    
    def _perturb_swap(self, solution: Solution) -> None:
        """Perturbação: troca dois itens entre bins."""
        n_bins = len(solution.bins)
        bin1_idx = random.randint(0, n_bins - 1)
        bin2_idx = random.randint(0, n_bins - 1)
        
        bin1 = solution.bins[bin1_idx]
        bin2 = solution.bins[bin2_idx]
        
        if not bin1.items or not bin2.items or bin1_idx == bin2_idx:
            return
        
        # Escolhe itens aleatórios para trocar
        item1 = random.choice(list(bin1.items))
        item2 = random.choice(list(bin2.items))
        
        # Verifica viabilidade da troca
        if (bin1.current_load - item1.size + item2.size <= bin1.capacity and
            bin2.current_load - item2.size + item1.size <= bin2.capacity):
            bin1.remove_item(item1.id)
            bin2.remove_item(item2.id)
            bin1.add_item(item2)
            bin2.add_item(item1)
    
    def _perturb_redistribute(self, solution: Solution) -> None:
        """Perturbação: redistribui todos os itens de um bin."""
        n_bins = len(solution.bins)
        bin_idx = random.randint(0, n_bins - 1)
        source_bin = solution.bins[bin_idx]
        
        if not source_bin.items:
            return
        
        # Coleta itens para redistribuir
        items_to_redistribute = list(source_bin.items)
        source_bin.items.clear()
        source_bin.current_load = 0.0
        
        # Tenta redistribuir em bins existentes
        for item in items_to_redistribute:
            placed = False
            bin_indices = list(range(n_bins))
            random.shuffle(bin_indices)
            
            for idx in bin_indices:
                if idx == bin_idx:
                    continue
                
                target_bin = solution.bins[idx]
                if target_bin.can_fit(item):
                    target_bin.add_item(item)
                    placed = True
                    break
            
            if not placed:
                # Se não couber em nenhum, decide se coloca no bin original ou cria novo
                if source_bin.can_fit(item):
                    source_bin.add_item(item)
                else:
                    new_bin = solution.create_bin()
                    new_bin.add_item(item)
    
    def _perturb_merge_split(self, solution: Solution) -> None:
        """Perturbação: tenta mesclar dois bins e depois dividir em dois novamente."""
        n_bins = len(solution.bins)
        bin1_idx = random.randint(0, n_bins - 1)
        bin2_idx = random.randint(0, n_bins - 1)
        
        if bin1_idx == bin2_idx:
            return
        
        bin1 = solution.bins[bin1_idx]
        bin2 = solution.bins[bin2_idx]
        
        # Juntar todos os itens
        combined_items = list(bin1.items) + list(bin2.items)
        
        # Limpar bins originais
        bin1.items.clear()
        bin1.current_load = 0.0
        bin2.items.clear()
        bin2.current_load = 0.0
        
        # Realocar itens usando FFD
        combined_items.sort(key=lambda x: x.size, reverse=True)
        
        for item in combined_items:
            # Tentar bin1 primeiro
            if bin1.can_fit(item):
                bin1.add_item(item)
            # Senão, tentar bin2
            elif bin2.can_fit(item):
                bin2.add_item(item)
            # Se não couber em nenhum, criar novo bin
            else:
                new_bin = solution.create_bin()
                new_bin.add_item(item)
    
    def _accept_solution(self, current_value: int, new_value: int, temperature: float) -> bool:
        """
        Decide se aceita uma nova solução baseado na diferença de valor e temperatura.
        Implementa o critério de Metropolis do Simulated Annealing.
        """
        if new_value <= current_value:  # Solução melhor (menos bins)
            return True
        
        # Probabilidade de aceitar solução pior
        delta = new_value - current_value
        probability = np.exp(-delta / temperature)
        return random.random() < probability


# Funções adicionais para os algoritmos de Bin Packing clássicos
def first_fit(items: List[Item], max_bins: int) -> Optional[Solution]:
    """
    Implementa o algoritmo First Fit para bin packing.
    Aloca cada item no primeiro bin onde ele cabe.
    """
    solution = Solution()
    
    # Cria os bins vazios
    for i in range(max_bins):
        solution.create_bin()
    
    for item in items:
        placed = False
        for bin in solution.bins:
            if bin.can_fit(item):
                bin.add_item(item)
                placed = True
                break
        
        if not placed:
            return None  # Não foi possível alocar todos os itens
    
    return solution


def best_fit(items: List[Item], max_bins: int) -> Optional[Solution]:
    """
    Implementa o algoritmo Best Fit para bin packing.
    Aloca cada item no bin mais cheio onde ele ainda cabe.
    """
    solution = Solution()
    
    # Cria os bins vazios
    for i in range(max_bins):
        solution.create_bin()
    
    for item in items:
        best_bin = None
        min_espaco_restante = float('inf')
        
        for bin in solution.bins:
            espaco_restante = bin.capacity - bin.current_load
            if item.size <= espaco_restante < min_espaco_restante:
                best_bin = bin
                min_espaco_restante = espaco_restante
        
        if best_bin is None:
            return None  # Não foi possível alocar todos os itens
        
        best_bin.add_item(item)
    
    return solution


def worst_fit(items: List[Item], max_bins: int) -> Optional[Solution]:
    """
    Implementa o algoritmo Worst Fit para bin packing.
    Aloca cada item no bin mais vazio.
    """
    solution = Solution()
    
    # Cria os bins vazios
    for i in range(max_bins):
        solution.create_bin()
    
    for item in items:
        worst_bin = None
        max_espaco_restante = -1
        
        for bin in solution.bins:
            espaco_restante = bin.capacity - bin.current_load
            if item.size <= espaco_restante and espaco_restante > max_espaco_restante:
                worst_bin = bin
                max_espaco_restante = espaco_restante
        
        if worst_bin is None:
            return None  # Não foi possível alocar todos os itens
        
        worst_bin.add_item(item)
    
    return solution


def executar_heuristica(itens: List[Item], capacidade: float, num_bins: int, 
                        estrategia: str) -> Dict[str, Any]:
    """
    Executa a heurística selecionada para resolver o problema de bin packing.
    
    Args:
        itens: Lista de itens.
        capacidade: Capacidade de cada bin.
        num_bins: Número máximo de bins.
        estrategia: Nome da estratégia ('First Fit', 'Best Fit', 'Worst Fit').
        
    Returns:
        Dicionário com os resultados da execução.
    """
    # Verificar viabilidade antes de executar
    viavel, min_bins = verificar_viabilidade_inicial(itens, capacidade, num_bins)
    
    if not viavel:
        return {
            "sucesso": False,
            "mensagem": f"Número de bins insuficiente. Com base na soma dos itens, são necessários pelo menos {min_bins} bins."
        }
    
    # Selecionar o algoritmo apropriado
    if estrategia == "First Fit":
        solution = first_fit(itens, num_bins)
    elif estrategia == "Best Fit":
        solution = best_fit(itens, num_bins)
    elif estrategia == "Worst Fit":
        solution = worst_fit(itens, num_bins)
    else:
        return {
            "sucesso": False,
            "mensagem": f"Estratégia '{estrategia}' não implementada."
        }
    
    # Verificar se a solução é válida
    if solution is None:
        return {
            "sucesso": False,
            "mensagem": "Não foi possível encontrar uma solução com o número de bins fornecido."
        }
    
    # Contar quantos bins foram realmente usados
    bins_usados = sum(1 for bin in solution.bins if bin.current_load > 0)
    
    return {
        "sucesso": True,
        "mensagem": f"Solução encontrada usando {bins_usados} de {num_bins} bins.",
        "solution": solution,
        "bins_usados": bins_usados
    }