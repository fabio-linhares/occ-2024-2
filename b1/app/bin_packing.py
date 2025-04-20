"""
Módulo de Bin Packing
====================

Este módulo implementa as estruturas de dados e algoritmos para resolver 
o problema de Bin Packing usando uma meta-heurística de busca local.
"""

import random
import time
import numpy as np
from dataclasses import dataclass
from typing import List, Set, Dict, Tuple, Optional


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


class LocalSearch:
    """Implementa a busca local para o problema de bin packing."""
    
    def __init__(self, time_limit_seconds: int):
        self.time_limit = time_limit_seconds
    
    def run(self, initial_solution: Solution) -> Solution:
        """
        Executa a busca local a partir de uma solução inicial.
        
        Args:
            initial_solution: Solução inicial para a busca local.
            
        Returns:
            A melhor solução encontrada dentro do limite de tempo.
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


def first_fit_decreasing(items: List[Item]) -> Solution:
    """
    Implementa o algoritmo First Fit Decreasing para construir uma solução inicial.
    
    Este algoritmo ordena os itens por tamanho (do maior para o menor) e então
    os aloca no primeiro bin onde eles couberem.
    
    Args:
        items: Lista de itens a serem alocados.
        
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
        
        # Se não conseguir colocar em nenhum bin existente, cria um novo
        if not placed:
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