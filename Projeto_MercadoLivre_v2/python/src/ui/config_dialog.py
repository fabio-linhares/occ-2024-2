# Nenhuma importação é necessária neste arquivo, pois não usa bibliotecas externas

class ConfigDialog:
    """Classe base para diálogos de configuração."""
    
    def __init__(self, config):
        """
        Inicializa o diálogo de configuração.
        
        Args:
            config (dict): Configuração atual
        """
        self.config = config
        self.title = "CONFIGURAÇÃO"
    
    def display(self):
        """
        Exibe o diálogo de configuração.
        
        Returns:
            dict: Configuração atualizada
        """
        print("\n" + "="*80)
        print(self.title.center(80))
        print("="*80)
        return self.config
    
    def _ensure_section(self, section):
        """
        Garante que uma seção exista na configuração.
        
        Args:
            section (str): Nome da seção
        """
        if section not in self.config:
            self.config[section] = {}
            
    def _get_choice(self, prompt, default):
        """
        Obtém uma escolha numérica do usuário.
        
        Args:
            prompt (str): Mensagem para o usuário
            default (int): Valor padrão caso o usuário não insira nada
            
        Returns:
            int: Escolha do usuário
        """
        try:
            choice = int(input(f"{prompt} [{default}]: ").strip() or default)
            return choice
        except ValueError:
            return default
    
    def _get_boolean(self, prompt, default):
        """
        Obtém uma escolha booleana do usuário.
        
        Args:
            prompt (str): Mensagem para o usuário
            default (bool): Valor padrão caso o usuário não insira nada
            
        Returns:
            bool: Escolha do usuário
        """
        choice = input(f"{prompt} [{'S' if default else 'n'}]: ").strip().lower() or ("s" if default else "n")
        return choice.startswith("s")


class PLIConfigDialog(ConfigDialog):
    """Diálogo para configuração de parâmetros do solver PLI."""
    
    def __init__(self, config):
        super().__init__(config)
        self.title = "CONFIGURAÇÃO DO MODELO PLI"
        
        # Garantir que o caminho do CPLEX existe na configuração
        self._ensure_section('cplex')
        if 'path' not in self.config['cplex']:
            self.config['cplex']['path'] = "/opt/ibm/ILOG/CPLEX_Studio2212"
        
        # Configurar aceleração GPU como padrão, se não estiver definido
        if 'acceleration' not in self.config['cplex']:
            self.config['cplex']['acceleration'] = 'auto'
    
    def _parse_float(self, input_str, default_value):
        """
        Analisa uma string para um valor float, com tratamento de formatos incomuns.
        
        Args:
            input_str (str): String a ser convertida
            default_value (float): Valor padrão caso a conversão falhe
            
        Returns:
            float: Valor convertido ou valor padrão
        """
        if not input_str.strip():
            return default_value
            
        try:
            # Remover zeros à esquerda antes do ponto decimal
            if '.' in input_str:
                parts = input_str.split('.')
                parts[0] = parts[0].lstrip('0')
                if not parts[0]:
                    parts[0] = '0'
                cleaned_input = '.'.join(parts)
            else:
                cleaned_input = input_str.lstrip('0')
                if not cleaned_input:
                    cleaned_input = '0'
                    
            return float(cleaned_input)
        except ValueError:
            print(f"Valor inválido: '{input_str}'. Usando valor padrão: {default_value}")
            return default_value
    
    def display(self):
        """Exibe o diálogo de configuração."""
        super().display()
        
        # 1. Método de linearização
        print("\nMétodo de linearização:")
        print("1. Variável Inversa (z = 1/|A'|)")
        print("2. Charnes-Cooper")
        print("3. Dinkelbach Iterativo")
        
        method_choice = self._get_choice("Escolha o método (1-3)", 1)
        linearization_methods = {
            1: "inverse",
            2: "charnes_cooper",
            3: "dinkelbach"
        }
        self.config['algorithm']['linearizer'] = linearization_methods[method_choice]
        
        # 2. Tipo de restrições
        print("\nTipo de restrições:")
        print("1. Restrições Rígidas")
        print("2. Restrições Suaves (com penalidades)")
        
        constraints_choice = self._get_choice("Escolha o tipo (1-2)", 2)
        self.config['constraints']['soft_constraints'] = (constraints_choice == 2)
        
        # 3. Configurações de penalidades (se usar restrições suaves)
        if constraints_choice == 2:
            print("\nConfigurações de penalidades para restrições suaves:")
            
            # Garantir que a seção de penalidades existe
            self._ensure_section('penalties')
            
            # Obter valores atuais ou padrão
            lb_penalty = self.config['penalties'].get('lb_penalty', 200.0)
            ub_penalty = self.config['penalties'].get('ub_penalty', 200.0)
            coverage_penalty = self.config['penalties'].get('item_coverage_penalty', 800.0)
            
            # Solicitar novos valores
            lb_input = input(f"Penalidade para violação do LB [{lb_penalty}]: ").strip()
            if lb_input:
                self.config['penalties']['lb_penalty'] = self._parse_float(lb_input, lb_penalty)
                
            ub_input = input(f"Penalidade para violação do UB [{ub_penalty}]: ").strip()
            if ub_input:
                self.config['penalties']['ub_penalty'] = self._parse_float(ub_input, ub_penalty)
                
            coverage_input = input(f"Penalidade para violação de cobertura de itens [{coverage_penalty}]: ").strip()
            if coverage_input:
                self.config['penalties']['item_coverage_penalty'] = self._parse_float(coverage_input, coverage_penalty)
        
        # 4. Configurações para o método Dinkelbach (se selecionado)
        if method_choice == 3:
            print("\nConfigurações para o método de Dinkelbach:")
            
            # Garantir que a seção lagrangiana existe
            self._ensure_section('lagrangian')
            
            # Obter valores atuais ou padrão
            max_iter = self.config['lagrangian'].get('max_iterations', 20)
            tolerance = self.config['lagrangian'].get('convergence_tolerance', 0.001)
            
            # Solicitar novos valores
            max_iter_input = input(f"Número máximo de iterações [{max_iter}]: ").strip()
            if max_iter_input:
                self.config['lagrangian']['max_iterations'] = int(max_iter_input)
                
            tol_input = input(f"Tolerância de convergência [{tolerance}]: ").strip()
            if tol_input:
                self.config['lagrangian']['convergence_tolerance'] = self._parse_float(tol_input, tolerance)
        
        # 5. Parâmetros do modelo
        print("\nParâmetros do modelo:")
        
        # Garantir que a seção do modelo existe
        self._ensure_section('model')
        
        # Obter valor atual ou padrão para Big-M
        big_m = self.config['model'].get('bigM', 1000)
        
        # Solicitar novo valor
        big_m_input = input(f"Valor de Big-M para restrições e linearizações [{big_m}]: ").strip()
        if big_m_input:
            self.config['model']['bigM'] = int(big_m_input)
        
        # 6. Configurar o solver
        print("\nSolver a utilizar:")
        print("1. CPLEX (recomendado)")
        print("2. CBC")
        print("3. GLPK")
        print("4. GUROBI (se disponível)")
        print("5. HiGHS (experimental)")
        
        solver_choice = self._get_choice("Escolha o solver (1-5)", 1)
        
        solver_map = {
            1: "CPLEX",
            2: "CBC",
            3: "GLPK", 
            4: "GUROBI",
            5: "HIGHS"
        }
        
        solver_name = solver_map[solver_choice]
        self.config['algorithm']['solver'] = solver_name
        
        # Para CPLEX, configurar opções adicionais
        if solver_choice == 1:
            # Verificar caminho do CPLEX
            cplex_path = self.config['cplex'].get('path', "/opt/ibm/ILOG/CPLEX_Studio2212")
            path_input = input(f"Caminho de instalação do CPLEX [{cplex_path}]: ").strip()
            if path_input:
                self.config['cplex']['path'] = path_input
            
            # Opções de aceleração
            print("\nOpções de aceleração CPLEX:")
            print("1. Automático (deixar CPLEX decidir)")
            print("2. Apenas CPU (padrão)")
            print("3. GPU (experimental)")
            
            accel_choice = self._get_choice("Escolha o modo de aceleração (1-3)", 1)
            accel_map = {
                1: "auto",
                2: "cpu",
                3: "gpu"
            }
            self.config['cplex']['acceleration'] = accel_map[accel_choice]
            
            # Se escolheu GPU, configurar opções adicionais
            if accel_choice == 3:
                # Configurar threads para GPU
                threads = self.config['cplex'].get('num_threads', 0)
                threads_input = input(f"Número de threads para GPU (0=automático) [{threads}]: ").strip()
                if threads_input:
                    try:
                        self.config['cplex']['num_threads'] = int(threads_input)
                    except ValueError:
                        print("Valor inválido. Usando configuração automática de threads.")
                
                # Configurar modo paralelo
                parallel_mode = self.config['cplex'].get('parallel_mode', 0)
                mode_input = input(f"Modo paralelo (-1=oportunista, 0=auto, 1=determinístico) [{parallel_mode}]: ").strip()
                if mode_input:
                    try:
                        self.config['cplex']['parallel_mode'] = int(mode_input)
                    except ValueError:
                        print("Valor inválido. Usando modo automático.")
        
        # 7. Tempo limite
        current_time_limit = self.config.get('algorithm', {}).get('max_runtime', 600)
        time_limit_str = input(f"\nTempo limite para o solver (segundos) [{current_time_limit}]: ").strip()
        if time_limit_str:
            try:
                time_limit = float(time_limit_str)
                if time_limit > 0:
                    self.config['algorithm']['max_runtime'] = time_limit
            except ValueError:
                print(f"Valor inválido. Mantendo o tempo limite de {current_time_limit} segundos.")
        
        print("\nParâmetros configurados com sucesso.")
        
        # Confirmar configurações (uma única vez)
        response = input("\nContinuar com estas configurações? (S/n): ").strip().lower()
        # Trata entrada vazia como 'sim'
        if response == '':
            response = 's'
        if response not in ['s', 'sim']:
            print("\nOperação cancelada pelo usuário.")
            return None  # Retornar None para indicar cancelamento
        
        return self.config