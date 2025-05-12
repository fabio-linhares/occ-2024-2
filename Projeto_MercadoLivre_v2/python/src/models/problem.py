import os
import time
import traceback
from collections import defaultdict
from src.models.solution import WaveOrderPickingSolution


# Importações base necessárias independentemente de CUDA
import numpy as np
import concurrent.futures  # Garante que está sempre importado para o fallback de multithread

_CUDA_IMPORT_SUCCESSFUL = False
cp = None

def _initialize_cupy_environment(config):
    global _CUDA_IMPORT_SUCCESSFUL, cp
    if _CUDA_IMPORT_SUCCESSFUL:
        return

    cuda_config = config.get('cuda', {})
    cuda_include_path = cuda_config.get('include_path')

    if cuda_include_path:
       # print(f"INFO: Tentando configurar o caminho de inclusão CUDA para CuPy: {cuda_include_path}")
        current_options = os.environ.get('CUPY_NVCC_OPTIONS', '')
        new_option = f"-I{cuda_include_path}"
        if new_option not in current_options:
            os.environ['CUPY_NVCC_OPTIONS'] = f"{current_options} {new_option}".strip()
           # print(f"INFO: CUPY_NVCC_OPTIONS definido como: {os.environ['CUPY_NVCC_OPTIONS']}")

    try:
        import cupy
        cp = cupy
        _CUDA_IMPORT_SUCCESSFUL = True
       # print("INFO: CuPy importado com sucesso.")
    except ImportError as e:
        _CUDA_IMPORT_SUCCESSFUL = False
        cp = None
        # Modificado para imprimir a mensagem de erro específica do ImportError
       # print(f"INFO: CuPy não pôde ser importado. CUDA não será usado. Erro: {e}")
        # Descomente a linha abaixo se desejar ver o traceback completo para o ImportError
        traceback.print_exc()
    except Exception as e:
        _CUDA_IMPORT_SUCCESSFUL = False
        cp = None
        print(f"ERRO: Erro inesperado ao importar CuPy: {type(e).__name__}: {str(e)}")
        traceback.print_exc()

class WaveOrderPickingProblem:
    """
    Representa o problema de Wave Order Picking do Mercado Livre.
    """
    
    def __init__(self, orders=None, aisles=None, n_items=0, wave_size_lb=0, wave_size_ub=0, config=None): # Adicionar config
        """Inicializa o problema."""
        self.config = config or {} # Armazenar config
        self.orders = orders or {}
        self.aisles = aisles or {}
        self.n_items = n_items
        self.wave_size_lb = wave_size_lb
        self.wave_size_ub = wave_size_ub
        
        self.n_orders = len(self.orders)
        self.n_aisles = len(self.aisles)
        
        self.all_order_items = set()
        self.item_units_by_aisle = defaultdict(dict)
        self.item_units_by_order = defaultdict(dict)
        self.order_units = {}

        # Inicializar o ambiente CuPy com base na configuração
        # Isso deve ser chamado antes de qualquer operação CuPy que possa disparar uma compilação JIT.
        _initialize_cupy_environment(self.config)
    
    def _preprocess(self):
        """Pré-processa os dados para facilitar os cálculos, tentando CUDA, depois multithreading, depois sequencial."""
        if self.n_orders == 0 or self.n_aisles == 0:
          #  print("INFO: Pulando pré-processamento: dados insuficientes (0 pedidos ou 0 corredores)")
            return

        start_time = time.time()
       # print(f"INFO: Iniciando pré-processamento para {self.n_orders} pedidos e {self.n_aisles} corredores...")

        processed_method = "Nenhum"

        if _CUDA_IMPORT_SUCCESSFUL:
            try:
        #        print("INFO: Tentando pré-processamento com CUDA...")
                self._preprocess_cuda()
                processed_method = "CUDA"
        #        print("INFO: Pré-processamento com CUDA bem-sucedido.")
            except Exception as e_cuda:
                print(f"AVISO: Erro durante o pré-processamento com CUDA: {type(e_cuda).__name__}: {str(e_cuda)}")
                if "libnvrtc.so" in str(e_cuda) or "nvrtc64_" in str(e_cuda) or "cuDNN" in str(e_cuda) or "cublas" in str(e_cuda):
                    print("DETALHE: Parece que uma biblioteca CUDA/GPU necessária (ex: NVRTC, cuDNN, cuBLAS) não foi encontrada ou falhou ao carregar.")
                    print("Verifique a instalação do CUDA Toolkit, drivers NVIDIA e bibliotecas GPU (cuDNN, etc.), e as variáveis de ambiente (ex: LD_LIBRARY_PATH).")
                elif "cudaErrorInsufficientDriver" in str(e_cuda):
                    print("DETALHE: A versão do driver NVIDIA instalada no seu sistema é muito antiga para a versão do CUDA Toolkit que está sendo usada (CUDA Runtime).")
                    print("Por favor, atualize os drivers da sua GPU NVIDIA para uma versão compatível com o CUDA Toolkit em uso.")
                # Se CUDA falhar, processed_method continua "Nenhum" para tentar o próximo método.
        
        if processed_method == "Nenhum": # Se CUDA não foi tentado (_CUDA_IMPORT_SUCCESSFUL = False) ou falhou
            try:
         #       print("INFO: Tentando pré-processamento com multithreading...")
                self._preprocess_multithread()
                processed_method = "Multithread"
        #        print("INFO: Pré-processamento com multithreading bem-sucedido.")
            except Exception as e_multi:
                print(f"AVISO: Erro durante o pré-processamento com multithreading: {type(e_multi).__name__}: {str(e_multi)}")
                traceback.print_exc() # Adicionado para melhor depuração
                # Se multithreading falhar, processed_method continua "Nenhum" para tentar o próximo.

        if processed_method == "Nenhum": # Se CUDA e Multithread falharam ou não foram tentados
            try:
        #        print("INFO: Executando pré-processamento sequencial como fallback final...")
                self._preprocess_sequential()
                processed_method = "Sequencial"
        #        print("INFO: Pré-processamento sequencial concluído.")
            except Exception as e_seq:
                print(f"ERRO CRÍTICO: Erro durante o pré-processamento sequencial: {type(e_seq).__name__}: {str(e_seq)}")
                traceback.print_exc()
                processed_method = "Falha Geral"
                print("ERRO CRÍTICO: Não foi possível completar o pré-processamento.")

        elapsed = time.time() - start_time
        if processed_method not in ["Nenhum", "Falha Geral"]:
            print(f"\nPré-processamento ({processed_method}) concluído em {elapsed:.2f} segundos.\n{len(self.all_order_items)} itens únicos processados.\n")
        elif processed_method == "Falha Geral":
            print(f"ERRO: Todos os métodos de pré-processamento falharam após {elapsed:.2f} segundos.")

    def _preprocess_cuda(self):
        """Implementação do pré-processamento usando CUDA para aceleração por GPU."""
        if not _CUDA_IMPORT_SUCCESSFUL or cp is None:
            raise RuntimeError("CuPy não está disponível para pré-processamento CUDA.")

        all_items = set()
        for order_items in self.orders.values():
            all_items.update(order_items.keys())
        self.all_order_items = all_items
        
        if not all_items:
            self.order_units = {o: 0 for o in self.orders.keys()}
            return

        max_item_id = max(all_items) + 1
        order_ids = list(self.orders.keys())
        aisle_ids = list(self.aisles.keys())
        
        item_order_map = cp.zeros((max_item_id, len(order_ids)), dtype=cp.int32)
        for idx, order_id in enumerate(order_ids):
            for item_id, quantity in self.orders[order_id].items():
                item_order_map[item_id, idx] = quantity
        
        item_aisle_map = cp.zeros((max_item_id, len(aisle_ids)), dtype=cp.int32)
        for idx, aisle_id in enumerate(aisle_ids):
            for item_id, quantity in self.aisles[aisle_id].items():
                item_aisle_map[item_id, idx] = quantity
        
        order_totals_gpu = cp.sum(item_order_map, axis=0)
        
        # Armazenar referências para matrizes GPU para reutilização
        self._gpu_item_order_map = item_order_map
        self._gpu_item_aisle_map = item_aisle_map
        
        # Ainda transferir para CPU para uso em funções não-GPU
        item_aisle_cpu = item_aisle_map.get()
        self.item_units_by_aisle.clear()
        for item_id_val in all_items:
            for aisle_idx, aisle_id_val in enumerate(aisle_ids):
                quantity = int(item_aisle_cpu[item_id_val, aisle_idx])
                if quantity > 0:
                    self.item_units_by_aisle[item_id_val][aisle_id_val] = quantity
        
        self.item_units_by_order.clear()
        item_order_cpu = item_order_map.get()
        for item_id_val in all_items:
            for order_idx, order_id_val in enumerate(order_ids):
                quantity = int(item_order_cpu[item_id_val, order_idx])
                if quantity > 0:
                    self.item_units_by_order[item_id_val][order_id_val] = quantity
        
        order_totals_cpu = order_totals_gpu.get()
        self.order_units = {order_id: int(order_totals_cpu[idx]) for idx, order_id in enumerate(order_ids)}

    def _preprocess_multithread(self):
        """Implementação do pré-processamento usando multithreading."""
        self.all_order_items.clear()
        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures_items = [executor.submit(lambda o_items: set(o_items.keys()), o_items) for o_items in self.orders.values()]
            for future in concurrent.futures.as_completed(futures_items):
                self.all_order_items.update(future.result())
        
        self.item_units_by_aisle.clear()
        def process_aisle_items(aisle_id_val, items_val):
            local_aisle_items = {}
            for item_id_val, quantity in items_val.items():
                local_aisle_items[item_id_val] = quantity
            return aisle_id_val, local_aisle_items

        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures_aisles = [executor.submit(process_aisle_items, aid, self.aisles[aid]) for aid in self.aisles.keys()]
            for future in concurrent.futures.as_completed(futures_aisles):
                aisle_id_val, processed_items = future.result()
                for item_id_val, quantity in processed_items.items():
                    self.item_units_by_aisle[item_id_val][aisle_id_val] = quantity
        
        self.item_units_by_order.clear()
        def process_order_items(order_id_val, items_val):
            local_order_items = {}
            for item_id_val, quantity in items_val.items():
                local_order_items[item_id_val] = quantity
            return order_id_val, local_order_items

        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures_orders = [executor.submit(process_order_items, oid, self.orders[oid]) for oid in self.orders.keys()]
            for future in concurrent.futures.as_completed(futures_orders):
                order_id_val, processed_items = future.result()
                for item_id_val, quantity in processed_items.items():
                    self.item_units_by_order[item_id_val][order_id_val] = quantity
        
        self.order_units.clear()
        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures_units = {executor.submit(lambda o_items: sum(o_items.values()), self.orders[oid]): oid for oid in self.orders.keys()}
            for future in concurrent.futures.as_completed(futures_units):
                oid = futures_units[future]
                self.order_units[oid] = future.result()
    
    def _preprocess_sequential(self):
        """Fallback para processamento sequencial caso os métodos paralelos falhem."""
        self.all_order_items.clear()
        for order_items in self.orders.values():
            self.all_order_items.update(order_items.keys())
        
        self.item_units_by_aisle.clear()
        for aisle_id, items in self.aisles.items():
            for item_id, quantity in items.items():
                self.item_units_by_aisle[item_id][aisle_id] = quantity
        
        self.item_units_by_order.clear()
        for order_id, items in self.orders.items():
            for item_id, quantity in items.items():
                self.item_units_by_order[item_id][order_id] = quantity
        
        self.order_units.clear()
        self.order_units = {o: sum(items.values()) for o, items in self.orders.items()}

    def read_input(self, input_file_path):
        self.orders = {}
        self.aisles = {}
        
        try:
            with open(input_file_path, 'r') as file:
                lines = file.readlines()
            
            if not lines:
                print(f"AVISO: Arquivo vazio: {input_file_path}")
                return self
            
            parts = lines[0].strip().split()
            if len(parts) < 3:
                print(f"AVISO: Formato de arquivo inválido na primeira linha: {input_file_path}")
                return self
                
            self.n_orders_file = int(parts[0])
            self.n_items = int(parts[1])
            self.n_aisles_file = int(parts[2])
            self.wave_size_lb = int(parts[3]) if len(parts) > 3 else 0
            self.wave_size_ub = int(parts[4]) if len(parts) > 4 else 0
            
            line_idx = 1
            
            for o_idx in range(self.n_orders_file):
                if line_idx >= len(lines): break
                order_parts = lines[line_idx].strip().split()
                line_idx += 1
                if not order_parts: continue
                try:
                    num_item_types_in_order = int(order_parts[0])
                    current_order_items = {}
                    for i in range(num_item_types_in_order):
                        item_id = int(order_parts[2*i + 1])
                        quantity = int(order_parts[2*i + 2])
                        current_order_items[item_id] = quantity
                    self.orders[o_idx] = current_order_items
                except (IndexError, ValueError) as e:
                    print(f"AVISO: Erro ao processar itens do pedido {o_idx} no arquivo {input_file_path}: {e}")
                    continue
            
            for a_idx in range(self.n_aisles_file):
                if line_idx >= len(lines): break
                aisle_parts = lines[line_idx].strip().split()
                line_idx += 1
                if not aisle_parts: continue
                try:
                    num_item_types_in_aisle = int(aisle_parts[0])
                    current_aisle_items = {}
                    for i in range(num_item_types_in_aisle):
                        item_id = int(aisle_parts[2*i + 1])
                        quantity = int(aisle_parts[2*i + 2])
                        current_aisle_items[item_id] = quantity
                    self.aisles[a_idx] = current_aisle_items
                except (IndexError, ValueError) as e:
                    print(f"AVISO: Erro ao processar itens do corredor {a_idx} no arquivo {input_file_path}: {e}")
                    continue

            # Última linha: LB, UB
            if line_idx >= len(lines):
                raise ValueError("Formato de arquivo inválido: Faltam dados de LB e UB.")
            self.wave_size_lb, self.wave_size_ub = map(int, lines[line_idx].split())
            # Adicione esta linha para depuração:
            # print(f"DEBUG: Lido LB={self.wave_size_lb}, UB={self.wave_size_ub} da linha: '{lines[line_idx]}'")

            # Atualizar n_orders e n_aisles ANTES de chamar _preprocess
            self.n_orders = len(self.orders)
            self.n_aisles = len(self.aisles)

            # Após ler todos os dados, realizar o pré-processamento
            self._preprocess()

           # print(f"Instância carregada: {self.n_orders} pedidos, {self.n_items} itens, {self.n_aisles} corredores.")
           # print(f"Limites de wave: LB={self.wave_size_lb}, UB={self.wave_size_ub}")
            
        except FileNotFoundError:
            print(f"ERRO: Arquivo não encontrado: {input_file_path}")
        except Exception as e:
            print(f"ERRO: Erro inesperado ao ler arquivo {input_file_path}: {str(e)}")
            traceback.print_exc()
        return self

    def write_output(self, solution, output_file_path):
        """Escreve a solução no formato esperado."""
        try:
            os.makedirs(os.path.dirname(output_file_path), exist_ok=True)
            with open(output_file_path, 'w') as f:
                f.write(f"{len(solution.selected_orders)}\n")
                for order_id in solution.selected_orders:
                    f.write(f"{order_id}\n")
                f.write(f"{len(solution.visited_aisles)}\n")
                for aisle_id in solution.visited_aisles:
                    f.write(f"{aisle_id}\n")
            return True
        except Exception as e:
            print(f"Erro ao escrever arquivo de saída {output_file_path}: {e}")
            return False

    def create_solution(self, selected_orders, visited_aisles):
        """Cria um objeto de solução."""
        total_units = sum(self.order_units.get(o, 0) for o in selected_orders)
        return WaveOrderPickingSolution(
            selected_orders=list(selected_orders), 
            visited_aisles=list(visited_aisles), 
            total_units=total_units
        )