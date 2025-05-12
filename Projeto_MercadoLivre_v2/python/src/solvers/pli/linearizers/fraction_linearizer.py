from abc import ABC, abstractmethod
import pulp
import time  # Adicionado: necessário para o método Dinkelbach

class FractionLinearizer(ABC):
    """
    Classe abstrata base para estratégias de linearização de função objetivo fracionária.
    """
    
    def __init__(self, problem, big_m=1000):
        """
        Inicializa o linearizador.
        
        Args:
            problem: O problema a ser resolvido
            big_m (int): Valor para constante Big-M
        """
        self.problem = problem
        self.big_m = big_m
    
    @abstractmethod
    def apply(self, model, variables):
        """
        Aplica a estratégia de linearização ao modelo.
        
        Args:
            model: O modelo de PLI
            variables (dict): Dicionário com as variáveis do modelo
            
        Returns:
            tuple: (modelo modificado, expressão da função objetivo)
        """
        pass


class CharnesCooperLinearizer(FractionLinearizer):
    """
    Implementa a linearização de Charnes-Cooper para função objetivo fracionária.
    Transforma max f(x)/g(x) em max f(y) s.a. g(y) = 1, y = tx, t > 0
    """
    
    def apply(self, model, variables):
        """
        Aplica a linearização de Charnes-Cooper ao modelo.
        
        Args:
            model: O modelo de PLI
            variables (dict): Dicionário com as variáveis do modelo
            
        Returns:
            tuple: (modelo modificado, expressão da função objetivo)
        """
        # Extrair variáveis do modelo
        x = variables['x']  # Variáveis de seleção de pedidos
        y = variables['y']  # Variáveis de seleção de corredores
        
        # Criar variável de transformação t > 0
        t = pulp.LpVariable("t", lowBound=0.00001, upBound=1.0)
        variables['t'] = t
        
        # Criar variáveis transformadas
        x_trans = {o: pulp.LpVariable(f"x_trans_{o}", lowBound=0, upBound=1.0) 
                  for o in range(self.problem.n_orders)}
        y_trans = {a: pulp.LpVariable(f"y_trans_{a}", lowBound=0, upBound=1.0) 
                  for a in range(self.problem.n_aisles)}
        
        variables['x_trans'] = x_trans
        variables['y_trans'] = y_trans
        
        # Adicionar restrições de linearização para o produto t * x[o]
        for o in range(self.problem.n_orders):
            # Restrições McCormick para linearizar x_trans[o] = t * x[o]
            model += x_trans[o] <= x[o], f"x_trans_upper1_{o}"  # Se x[o]=0, então x_trans[o]=0
            model += x_trans[o] <= t, f"x_trans_upper2_{o}"     # x_trans[o] não pode exceder t
            model += x_trans[o] >= t - (1 - x[o]), f"x_trans_lower_{o}"  # Se x[o]=1, então x_trans[o] >= t
            model += x_trans[o] >= 0, f"x_trans_non_neg_{o}"   # Não-negatividade
        
        # Adicionar restrições de linearização para o produto t * y[a]
        for a in range(self.problem.n_aisles):
            # Restrições McCormick para linearizar y_trans[a] = t * y[a]
            model += y_trans[a] <= y[a], f"y_trans_upper1_{a}"
            model += y_trans[a] <= t, f"y_trans_upper2_{a}"
            model += y_trans[a] >= t - (1 - y[a]), f"y_trans_lower_{a}"
            model += y_trans[a] >= 0, f"y_trans_non_neg_{a}"
        
        # Restrição que normaliza o denominador
        model += pulp.lpSum(y_trans[a] for a in range(self.problem.n_aisles)) == 1, "normalize_denom"
        
        # Construir expressão da função objetivo linearizada
        objective_expr = pulp.lpSum(
            self.problem.order_units[o] * x_trans[o] for o in range(self.problem.n_orders)
        )
        
        return model, objective_expr


class InverseVariableLinearizer(FractionLinearizer):
    """
    Implementa a linearização usando a variável inversa z = 1/|A'|
    e variáveis auxiliares w_o = z·x_o com inequações de McCormick.
    Esta é uma técnica estado-da-arte para linearizar funções objetivo fracionárias
    em problemas de otimização combinatória.
    """
    
    def apply(self, model, variables):
        """
        Aplica a linearização por variável inversa ao modelo.
        
        Args:
            model: O modelo de PLI
            variables (dict): Dicionário com as variáveis do modelo
            
        Returns:
            tuple: (modelo modificado, expressão da função objetivo)
        """
        # Extrair variáveis do modelo
        x = variables['x']  # Variáveis de seleção de pedidos
        y = variables['y']  # Variáveis de seleção de corredores
        
        # Criar nova variável z = 1/|A'|
        z = pulp.LpVariable("z", lowBound=0, upBound=1)
        variables['z'] = z
        
        # Criar variáveis auxiliares w_o = z·x_o
        # Estas variáveis representam a linearização do produto z·x_o
        w = {o: pulp.LpVariable(f"w_{o}", lowBound=0, upBound=1) for o in range(self.problem.n_orders)}
        variables['w'] = w
        
        # Garantir que pelo menos um corredor seja escolhido
        model += pulp.lpSum(y[a] for a in range(self.problem.n_aisles)) >= 1, "at_least_one_aisle"
        
        # Em vez da restrição não-linear, usamos variáveis binárias auxiliares
        # para modelar a relação z = 1/sum(y)
        
        # Limite máximo para considerar (para não criar muitas variáveis)
        max_aisles_to_model = min(20, self.problem.n_aisles)
        
        # Criar variáveis binárias para cada possível quantidade de corredores
        count_vars = {}
        for count in range(1, max_aisles_to_model + 1):
            count_vars[count] = pulp.LpVariable(f"count_{count}", cat=pulp.LpBinary)
        
        # Garantir que exatamente uma variável count seja selecionada
        model += pulp.lpSum(count_vars[count] for count in range(1, max_aisles_to_model + 1)) == 1, "one_count"
        
        # Conectar variáveis count com o número real de corredores selecionados
        total_y = pulp.lpSum(y[a] for a in range(self.problem.n_aisles))
        for count in range(1, max_aisles_to_model + 1):
            # Se count_vars[count] = 1, então total_y = count
            model += total_y <= count + self.big_m * (1 - count_vars[count]), f"count_upper_{count}"
            model += total_y >= count - self.big_m * (1 - count_vars[count]), f"count_lower_{count}"
            
            # Se count_vars[count] = 1, então z = 1/count
            model += z <= (1.0/count) + self.big_m * (1 - count_vars[count]), f"z_upper_{count}"
            model += z >= (1.0/count) - self.big_m * (1 - count_vars[count]), f"z_lower_{count}"
        
        # Adicionar inequações de McCormick para linearizar w_o = z·x_o
        for o in range(self.problem.n_orders):
            # w_o ≤ x_o (se x_o = 0, então w_o = 0)
            model += w[o] <= x[o], f"mccormick1_{o}"
            
            # w_o ≤ z (w_o não pode ser maior que z)
            model += w[o] <= z, f"mccormick2_{o}"
            
            # w_o ≥ z + x_o - 1 (garante que se x_o = 1 e z = 1, então w_o = 1)
            model += w[o] >= z + x[o] - 1, f"mccormick3_{o}"
            
            # w_o ≥ 0 (não negatividade)
            model += w[o] >= 0, f"mccormick4_{o}"
        
        # Construir expressão da função objetivo linearizada
        objective_expr = pulp.lpSum(
            self.problem.order_units[o] * w[o] for o in range(self.problem.n_orders)
        )
        
        return model, objective_expr


class DinkelbachLinearizer(FractionLinearizer):
    """
    Implementa o método de linearização de Dinkelbach com suporte a GPU.
    """
    
    def __init__(self, problem, big_m, max_iterations=20, tolerance=1e-4, use_gpu=False):
        """
        Inicializa o linearizador Dinkelbach.
        
        Args:
            problem: O problema a ser resolvido
            big_m: Valor para a constante big-M
            max_iterations: Número máximo de iterações Dinkelbach
            tolerance: Tolerância para convergência
            use_gpu: Flag para usar aceleração GPU
        """
        super().__init__(problem, big_m)
        self.max_iterations = max_iterations
        self.tolerance = tolerance
        self.use_gpu = use_gpu
        self.gpu_manager = None
        
        # Inicializar GPU Manager se necessário
        if self.use_gpu:
            from src.utils.gpu_manager import GPUManager
            self.gpu_manager = GPUManager(problem)
            self.gpu_manager.initialize()
    
    def apply(self, model, variables):
        """
        Aplica o método de Dinkelbach ao modelo.
        
        Args:
            model: O modelo de PLI
            variables: Dicionário com as variáveis do modelo
            
        Returns:
            tuple: (modelo modificado, expressão da função objetivo)
        """
        # Extrair variáveis do modelo
        x = variables['x']  # Variáveis de seleção de pedidos
        y = variables['y']  # Variáveis de seleção de corredores
        
        # Inicializar com q = 0 (estimativa inicial do valor objetivo)
        q_value = 0.0
        
        # Construir expressão da função objetivo linearizada
        # Na primeira iteração de Dinkelbach, maximizamos numerador - q * denominador
        objective_expr = pulp.lpSum(
            self.problem.order_units[o] * x[o] for o in range(self.problem.n_orders)
        ) - q_value * pulp.lpSum(y[a] for a in range(self.problem.n_aisles))
        
        # Não precisamos adicionar restrições especiais no método Dinkelbach
        # O algoritmo vai resolver o problema várias vezes, ajustando q
        
        return model, objective_expr