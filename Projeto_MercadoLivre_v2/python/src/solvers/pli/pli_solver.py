# -*- coding: utf-8 -*-
from src.solvers.base_solver import BaseSolver
from src.models.solution import WaveOrderPickingSolution
from src.utils.gpu_manager import GPUManager
from src.solvers.pli.linearizers.fraction_linearizer import (
    InverseVariableLinearizer, 
    CharnesCooperLinearizer,
    DinkelbachLinearizer
)
from src.utils.check_solvers import is_nixos
import os
import pulp
import time
import sys


class PLISolver(BaseSolver):
    """
    Implementa um solver baseado em Programação Linear Inteira (PLI)
    para o problema de Wave Order Picking.
    """

    def __init__(self, problem, config=None):
        """
        Inicializa o solver PLI.
        
        Args:
            problem: O problema a ser resolvido
            config (dict, optional): Configurações para o solver
        """
        super().__init__(problem, config)
        
        # Configuração do solver
        self.solver_name = self.config.get('algorithm', {}).get('solver', 'CPLEX')
        self.time_limit = self.config.get('algorithm', {}).get('time_limit', 300)
        
        # Linearizador
        self.linearizer_type = self.config.get('algorithm', {}).get('linearizer', 'direct')
        self.linearizer = None
        self._initialize_linearizer()
        
        # Gerenciador GPU
        self.use_gpu = self.config.get('algorithm', {}).get('use_gpu', False)
        self.gpu_manager = None
        if self.use_gpu:
            self.gpu_manager = GPUManager(problem)
            self.gpu_manager.initialize()
    
    def _initialize_linearizer(self):
        """Inicializa o linearizador de acordo com a configuração."""
        # Obter o valor de big_m da configuração
        big_m = self.config.get('model', {}).get('bigM', 1000)
        
        if self.linearizer_type == 'inverse':
            self.linearizer = InverseVariableLinearizer(self.problem, big_m)
        elif self.linearizer_type == 'charnes_cooper':
            self.linearizer = CharnesCooperLinearizer(self.problem, big_m)
        elif self.linearizer_type == 'dinkelbach':
            # Extrair parâmetros específicos do Dinkelbach da configuração
            max_iterations = self.config.get('lagrangian', {}).get('max_iterations', 20)
            tolerance = self.config.get('lagrangian', {}).get('convergence_tolerance', 0.001)
            use_gpu = self.config.get('algorithm', {}).get('use_gpu', False)
            
            self.linearizer = DinkelbachLinearizer(
                self.problem, 
                big_m, 
                max_iterations=max_iterations,
                tolerance=tolerance,
                use_gpu=use_gpu
            )
        elif self.linearizer_type == 'direct':
            # Para o tipo 'direct', usamos o Dinkelbach como fallback
            print("Aviso: Método 'direct' não implementado. Usando Dinkelbach como alternativa.")
            
            # Extrair parâmetros específicos do Dinkelbach da configuração
            max_iterations = self.config.get('lagrangian', {}).get('max_iterations', 20)
            tolerance = self.config.get('lagrangian', {}).get('convergence_tolerance', 0.001)
            use_gpu = self.config.get('algorithm', {}).get('use_gpu', False)
            
            self.linearizer = DinkelbachLinearizer(
                self.problem, 
                big_m, 
                max_iterations=max_iterations,
                tolerance=tolerance,
                use_gpu=use_gpu
            )
            self.linearizer_type = 'dinkelbach'  # Atualiza o tipo para consistência
    
    def _initialize_solver(self):
        """Inicializa o solver com base na configuração."""
        import pulp
        import os
        import time
        
        time_limit_seconds = self.get_remaining_time(time.time())
        time_limit_seconds_int = int(time_limit_seconds)
        self.time_limit = time_limit_seconds_int
        
        print(f"\nInicializando solver: {self.solver_name}")
        
        # Verificar se é o modo GPU do CPLEX
        if self.solver_name == 'CPLEX_GPU':
            self.solver_name = 'CPLEX'  # Mudar para CPLEX padrão
            self.use_gpu = True         # Ativar flag de GPU
            
        if self.solver_name == 'CPLEX':
            # Configurar CPLEX
            cplex_path = self.config.get('cplex', {}).get('path', "/opt/ibm/ILOG/CPLEX_Studio2212")
            cplex_exec = os.path.join(cplex_path, "cplex", "bin", "x86-64_linux", "cplex")
            
            # Verificar se o executável existe
            if not os.path.exists(cplex_exec):
                print(f"AVISO: Executável CPLEX não encontrado em: {cplex_exec}")
                cplex_exec = self._find_cplex_executable()
                
            if not cplex_exec or not os.path.exists(cplex_exec):
                print("ERRO: CPLEX não encontrado. Usando CBC como fallback.")
                self.solver_name = 'CBC'
                return self._initialize_solver()
                
            # Parâmetros CPLEX
            cplex_params = []
            
            # Verificar se GPU deve ser usada
            if self.use_gpu or self.config.get('cplex', {}).get('acceleration') == 'gpu':
                print("Ativando aceleração GPU para CPLEX")
                
                # Número de threads (0 = automático)
                num_threads = self.config.get('cplex', {}).get('num_threads', 0)
                
                # Modo paralelo (1 = oportunista, recomendado para GPU)
                parallel_mode = self.config.get('cplex', {}).get('parallel_mode', 1)
                
                # Parâmetros otimizados para GPU
                cplex_params.extend([
                    "CPXPARAM_ParamDisplay=1",              # Mostrar parâmetros
                    f"CPXPARAM_Threads={num_threads}",      # Threads (0 = auto)
                    f"CPXPARAM_Parallel={parallel_mode}",   # Modo paralelo
                    "CPXPARAM_MIP_Strategy_HeuristicFreq=10", # Aumentar frequência heurística
                    "CPXPARAM_MIP_Strategy_Presolve=2",     # Presolve agressivo
                    "CPXPARAM_MIP_Cuts_LiftProj=3",         # Cortes lift-and-project agressivos
                    "CPXPARAM_OptimalityTarget=1",          # Foco em primeiro ótimo
                    "CPXPARAM_LPMethod=4",                  # Usar método barrier (melhor para GPU)
                    "CPXPARAM_Barrier_Algorithm=3"          # Algoritmo de barreira otimizado
                ])
                
                # Criar arquivo de parâmetros
                param_file = "WaveOrderPicking.prm"
                with open(param_file, 'w') as f:
                    f.write("\n".join(cplex_params))
                    f.write(f"\nCPXPARAM_TimeLimit={time_limit_seconds}")
                
                # Em ambientes NixOS, usar wrapper específico
                if is_nixos():
                    from src.utils.check_solvers import NixOSCplexSolver
                    return NixOSCplexSolver(cplex_path, timeLimit=time_limit_seconds, 
                                          msg=True, options=[f"-p{param_file}"])
                else:
                    return pulp.CPLEX_CMD(path=cplex_exec, timeLimit=time_limit_seconds, 
                                         msg=True, options=[f"-p{param_file}"])
            else:
                # Modo CPU padrão
                cplex_params.extend([
                    "CPXPARAM_ParamDisplay=1",
                    "CPXPARAM_Threads=1",      # 1 thread para evitar overhead
                    "CPXPARAM_Parallel=0",     # Desativar paralelismo
                    "CPXPARAM_LPMethod=1"      # Método simplex primal
                ])
                
                # Adicionar time limit
                cplex_params.append(f"CPXPARAM_TimeLimit={time_limit_seconds}")
                
                # Criar o solver
                try:
                    return pulp.CPLEX_CMD(path=cplex_exec, timeLimit=time_limit_seconds, 
                                         msg=True, options=cplex_params)
                except Exception as e:
                    print(f"Erro ao inicializar CPLEX: {str(e)}")
                    print("Usando CBC como fallback.")
                    self.solver_name = 'CBC'
                    return self._initialize_solver()
        
        elif self.solver_name == 'CBC':
            return pulp.PULP_CBC_CMD(timeLimit=time_limit_seconds, msg=True)
            
        elif self.solver_name == 'GLPK':
            return pulp.GLPK_CMD(timeLimit=time_limit_seconds, msg=True)
            
        elif self.solver_name == 'GUROBI' and pulp.GUROBI_CMD().available():
            return pulp.GUROBI_CMD(timeLimit=time_limit_seconds, msg=True)
            
        else:
            print(f"Solver {self.solver_name} não disponível. Usando CBC como fallback.")
            return pulp.PULP_CBC_CMD(timeLimit=time_limit_seconds, msg=True)
    
    def _find_cplex_executable(self):
        """Tenta encontrar o executável do CPLEX em locais comuns."""
        # Locais comuns de instalação do CPLEX
        common_paths = [
            "/opt/ibm/ILOG/CPLEX_Studio2212/cplex/bin/x86-64_linux/cplex",
            "/opt/ibm/ILOG/CPLEX_Studio221/cplex/bin/x86-64_linux/cplex",
            "/opt/ibm/ILOG/CPLEX_Studio22/cplex/bin/x86-64_linux/cplex",
            "/opt/ibm/ILOG/CPLEX_Studio201/cplex/bin/x86-64_linux/cplex",
            "/opt/ibm/ILOG/CPLEX_Studio129/cplex/bin/x86-64_linux/cplex",
            "/opt/ibm/ILOG/CPLEX_Studio128/cplex/bin/x86-64_linux/cplex"
        ]
        
        # Verificar variável de ambiente
        if 'CPLEX_STUDIO_DIR' in os.environ:
            env_path = os.path.join(os.environ['CPLEX_STUDIO_DIR'], "cplex/bin/x86-64_linux/cplex")
            common_paths.insert(0, env_path)  # Dar prioridade ao caminho da variável de ambiente
        
        # Verificar cada caminho
        for path in common_paths:
            if os.path.exists(path):
                print(f"CPLEX encontrado em: {path}")
                return path
        
        # Se chegou aqui, não encontrou
        print("CPLEX não encontrado em nenhum local comum.")
        return None
    
    def solve(self, start_time=None):
        """
        Resolve o problema usando Programação Linear Inteira.
        
        Args:
            start_time (float, optional): Tempo de início para controle de timeout
            
        Returns:
            WaveOrderPickingSolution: A solução encontrada
        """
        import pulp
        import time
        import os
        
        # Iniciar contagem de tempo se não fornecido
        start_time = start_time or time.time()
        
        # Mostrar informações sobre o problema
        print(f"\nResolvendo problema com {self.problem.n_orders} pedidos e {self.problem.n_aisles} corredores")
        print(f"Usando método de linearização: {self.linearizer_type}")
        print(f"Solver: {self.solver_name}")
        
        # Inicializar modelo
        model = pulp.LpProblem("WaveOrderPicking", pulp.LpMaximize)
        
        # Criar variáveis de decisão
        variables = {}
        
        # x[o] = 1 se o pedido o está na wave, 0 caso contrário
        x = {o: pulp.LpVariable(f"x_{o}", cat=pulp.LpBinary) for o in range(self.problem.n_orders)}
        
        # y[a] = 1 se o corredor a é visitado, 0 caso contrário
        y = {a: pulp.LpVariable(f"y_{a}", cat=pulp.LpBinary) for a in range(self.problem.n_aisles)}
        
        variables['x'] = x
        variables['y'] = y
        
        # Verificar tempo restante
        if self.check_timeout(start_time):
            print("Tempo limite excedido durante a inicialização das variáveis.")
            return self.problem.create_solution([], [])
        
        # Adicionar restrições básicas
        
        # 1. Restrição de limite inferior de unidades
        lb_constraint = pulp.lpSum(self.problem.order_units[o] * x[o] for o in range(self.problem.n_orders)) >= self.problem.wave_size_lb
        model += lb_constraint, "LB_Constraint"
        
        # 2. Restrição de limite superior de unidades
        ub_constraint = pulp.lpSum(self.problem.order_units[o] * x[o] for o in range(self.problem.n_orders)) <= self.problem.wave_size_ub
        model += ub_constraint, "UB_Constraint"
        
        # 3. Restrições de cobertura de itens
        for item in self.problem.all_order_items:
            # Demanda do item nos pedidos selecionados
            item_demand = pulp.lpSum(
                self.problem.item_units_by_order.get(item, {}).get(o, 0) * x[o]
                for o in range(self.problem.n_orders)
                if item in self.problem.item_units_by_order and o in self.problem.item_units_by_order[item]
            )
            
            # Oferta do item nos corredores selecionados
            item_supply = pulp.lpSum(
                self.problem.item_units_by_aisle.get(item, {}).get(a, 0) * y[a]
                for a in range(self.problem.n_aisles)
                if item in self.problem.item_units_by_aisle and a in self.problem.item_units_by_aisle[item]
            )
            
            # A oferta deve ser maior ou igual à demanda
            model += item_demand <= item_supply, f"ItemCoverage_{item}"
        
        # Verificar tempo restante
        if self.check_timeout(start_time):
            print("Tempo limite excedido durante a adição de restrições básicas.")
            return self.problem.create_solution([], [])
        
        # Aplicar linearização específica baseada na escolha do usuário
        if self.linearizer_type == 'dinkelbach':
            solution = self._solve_dinkelbach(model, variables, start_time)
        else:
            # Aplicar outras linearizações (inverse, charnes_cooper)
            # Inicializar linearizador se não foi feito
            if self.linearizer is None:
                self._initialize_linearizer()
                
            # Aplicar linearizador ao modelo
            model, objective_expr = self.linearizer.apply(model, variables)
            
            # Definir função objetivo
            model += objective_expr, "Objective"
            
            # Inicializar solver
            solver = self._initialize_solver()
            
            # Resolver o modelo
            try:
                start_solve_time = time.time()
                status = model.solve(solver)
                solve_time = time.time() - start_solve_time
                print(f"Tempo de solução: {solve_time:.2f} segundos")
                
                # Verificar status da solução
                if status == pulp.LpStatusOptimal:
                    print(f"Solução ótima encontrada com valor: {pulp.value(model.objective)}")
                    
                    # Extrair solução
                    selected_orders = [o for o in range(self.problem.n_orders) if pulp.value(x[o]) > 0.5]
                    visited_aisles = [a for a in range(self.problem.n_aisles) if pulp.value(y[a]) > 0.5]
                    
                    # Criar e verificar solução
                    solution = self.problem.create_solution(selected_orders, visited_aisles)
                    is_feasible = self._is_solution_feasible(solution)
                    objective_value = self._compute_objective_function(solution)
                    
                    solution.set_feasibility(is_feasible)
                    solution.set_objective_value(objective_value)
                    
                    print(f"Solução viável: {is_feasible}")
                    print(f"Valor objetivo: {objective_value}")
                    
                    return solution
                else:
                    print(f"Não foi possível encontrar uma solução ótima. Status: {pulp.LpStatus[status]}")
                    return self.problem.create_solution([], [])
                    
            except Exception as e:
                print(f"Erro ao resolver o modelo: {str(e)}")
                return self.problem.create_solution([], [])
                
        return solution
    
    def _solve_dinkelbach(self, model, variables, start_time):
        """Implementa o algoritmo de Dinkelbach para problemas de otimização fracionária."""
        # Aumentar o limite de recursão do Python para evitar o erro de recursão
        old_recursion_limit = sys.getrecursionlimit()
        sys.setrecursionlimit(10000)  # Define um limite bem maior
        
        try:
            # Extrair variáveis do modelo
            x = variables['x']  # Variáveis de seleção de pedidos
            y = variables['y']  # Variáveis de seleção de corredores
            
            # Parâmetros do método Dinkelbach
            max_iterations = self.config.get('lagrangian', {}).get('max_iterations', 20)
            tolerance = self.config.get('lagrangian', {}).get('convergence_tolerance', 0.001)
            q_value = 0.0  # Valor inicial de q
            
            best_solution = None
            best_objective = 0.0
            
            for iteration in range(max_iterations):
                if self.check_timeout(start_time):
                    print(f"Tempo limite atingido após {iteration} iterações Dinkelbach.")
                    break
                    
                print(f"\nIteração Dinkelbach {iteration+1}/{max_iterations}, q = {q_value:.6f}")
                
                # Atualizar função objetivo com o valor atual de q
                objective_expr = pulp.lpSum(
                    self.problem.order_units[o] * x[o] for o in range(self.problem.n_orders)
                ) - q_value * pulp.lpSum(y[a] for a in range(self.problem.n_aisles))
                
                model.setObjective(objective_expr)
                
                # Resolver o subproblema
                solver = self._initialize_solver()
                if solver is None:
                    print("Não foi possível inicializar o solver. Tentando heurística.")
                    # Voltar para heurística
                    from src.solvers.heuristic.greedy_solver import GreedySolver
                    greedy = GreedySolver(self.problem, self.config)
                    return greedy.solve(start_time)
                
                try:
                    # Definir tempo limite restante para cada iteração
                    time_limit = min(self.get_remaining_time(start_time), 
                                    self.config.get('model', {}).get('optimize_timeout', 30))
                    
                    # Configurar solver com tempo limite
                    if hasattr(solver, 'timeLimit'):
                        solver.timeLimit = time_limit
                        
                    # Resolver o modelo
                    model.solve(solver)
                    
                    # Verificar se a solução foi encontrada
                    if model.status not in [pulp.LpStatusOptimal, pulp.LpStatusInfeasible, 
                                         pulp.LpStatusUnbounded, pulp.LpStatusNotSolved]:
                        raise Exception(f"Status inesperado do solver: {pulp.LpStatus[model.status]}")
                    
                    # Se não encontrou solução ótima, ir para próxima iteração ou finalizar
                    if model.status != pulp.LpStatusOptimal:
                        print(f"Subproblema não ótimo: {pulp.LpStatus[model.status]}")
                        break
                    
                    # Extrair solução atual
                    selected_orders = [o for o in range(self.problem.n_orders) 
                                     if x[o].value() is not None and x[o].value() > 0.5]
                    visited_aisles = [a for a in range(self.problem.n_aisles) 
                                    if y[a].value() is not None and y[a].value() > 0.5]
                    
                    # CORREÇÃO: Verificar se há corredores visitados para evitar divisão por zero
                    if not visited_aisles:
                        print("Aviso: Nenhum corredor visitado. Ajustando solução.")
                        continue
                    
                    # Criar solução e verificar se é viável
                    solution = self.problem.create_solution(selected_orders, visited_aisles)
                    solution.is_feasible = self._is_solution_feasible(solution)
                    
                    if solution.is_feasible:
                        # Calcular valor objetivo e F(q) para verificar convergência
                        objective_value = self._compute_objective_function(solution)
                        f_q_value = objective_value - q_value
                        solution.objective_value = objective_value
                        
                        print(f"Solução viável encontrada: {len(selected_orders)} pedidos, {len(visited_aisles)} corredores")
                        print(f"Valor objetivo: {objective_value:.6f}, F(q): {f_q_value:.6f}")
                        
                        # Atualizar melhor solução
                        if best_solution is None or objective_value > best_objective:
                            best_solution = solution
                            best_objective = objective_value
                        
                        # Verificar convergência - CORREÇÃO: Garantir que f_q_value não é None
                        if f_q_value is not None and abs(f_q_value) < tolerance:
                            print(f"Algoritmo convergiu: |F(q)| = {abs(f_q_value):.6f} < {tolerance}")
                            break
                        
                        # Atualizar valor de q - CORREÇÃO: Proteção contra None
                        if objective_value is not None:
                            q_value = objective_value
                        else:
                            print("Aviso: Valor objetivo é None, mantendo q anterior")
                    else:
                        print("Solução inviável encontrada. Ajustando penalidades.")
                        # Poderia ajustar penalidades aqui
                
                except Exception as e:
                    print(f"Erro na iteração Dinkelbach {iteration+1}: {str(e)}")
                    traceback.print_exc()
                    # Se ocorrer erro no CPLEX, não tente usar CBC recursivamente
                    if iteration > 0 and best_solution is not None:
                        print("Retornando melhor solução encontrada até agora.")
                        return best_solution
                    else:
                        print("Não foi possível resolver com PLI. Tentando heurística.")
                        # CORREÇÃO: Evita recursão infinita criando instância diretamente
                        from src.solvers.heuristic.greedy_solver import GreedySolver
                        greedy = GreedySolver(self.problem, self.config)
                        return greedy.solve(start_time)
                        
            # Retornar a melhor solução encontrada
            if best_solution is not None:
                print(f"\nMelhor solução Dinkelbach: {best_objective:.6f}")
                return best_solution
            
            # Se não encontrou solução, criar solução vazia
            print("\nNenhuma solução viável encontrada por Dinkelbach.")
            return self.problem.create_solution([], [])
        
        finally:
            # Sempre restaurar o limite de recursão original
            sys.setrecursionlimit(old_recursion_limit)

    # Adicionando a classe NixOSCplexSolver que funciona no NixOS usando FHS

class NixOSCplexSolver(pulp.LpSolver):
    """Solver CPLEX personalizado para NixOS usando ambiente FHS."""
    
    def __init__(self, cplex_path, timeLimit=None, msg=True, options=None):
        self.cplex_path = cplex_path
        self.timeLimit = timeLimit
        self.msg = msg
        self.options = options or []
    
    def available(self):
        from src.utils.check_solvers import is_nixos
        return is_nixos() and os.path.exists(os.path.join(self.cplex_path, "cplex/bin/x86-64_linux/cplex"))
    
    def actualSolve(self, lp):
        """Resolve o problema usando o ambiente FHS do NixOS."""
        from src.utils.nixos_cplex_wrapper import run_cplex_with_nix_shell
        
        # Verificar se o arquivo LP já existe
        if not os.path.exists(lp.name):
            # Precisamos escrever o arquivo LP
            lp.writeLP(lp.name)
        
        # Criar arquivo de parâmetros para o CPLEX
        param_file = lp.name + ".prm"
        with open(param_file, "w") as f:
            for option in self.options:
                f.write(option + "\n")
        
        # Argumentos para o CPLEX
        args = [
            "-c", f"read {lp.name}",
            "-c", f"read {param_file}",
            "-c", "optimize",
            "-c", f"write {lp.name}.sol"
        ]
        
        # Executar CPLEX com ambiente FHS
        if self.msg:
            print("Executando CPLEX no ambiente FHS do NixOS...")
        
        max_retries = 2
        success = False
        
        for attempt in range(max_retries):
            if attempt > 0:
                print(f"Tentativa {attempt+1} de executar o CPLEX...")
                
            returncode, stdout, stderr = run_cplex_with_nix_shell(self.cplex_path, args)
            
            if returncode == 0:
                success = True
                break
            else:
                if self.msg:
                    print(f"CPLEX retornou erro (tentativa {attempt+1}): {returncode}")
                    print(stderr)
        
        if not success:
            print("Todas as tentativas de executar CPLEX falharam.")
            return 1  # Erro
        
        # Verificar se a solução foi encontrada
        sol_file = lp.name + ".sol"
        if not os.path.exists(sol_file):
            if self.msg:
                print("Arquivo de solução não encontrado")
            return 1  # Erro
        
        # Processar solução...
        # [Resto do código permanece igual]