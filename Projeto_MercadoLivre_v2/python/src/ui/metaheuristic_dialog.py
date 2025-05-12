from src.ui.config_dialog import ConfigDialog

class MetaheuristicDialog(ConfigDialog):
    """Diálogo para configuração de meta-heurísticas."""
    
    def __init__(self, config):
        super().__init__(config)
        self.title = "CONFIGURAÇÃO DE META-HEURÍSTICAS"
        
        # Garantir que a seção meta_heuristic existe na configuração
        self._ensure_section('meta_heuristic')
    
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
    
    def display(self):
        """Exibe o diálogo de configuração."""
        super().display()
        
        # 1. Método de meta-heurística
        print("\nTipo de meta-heurística:")
        print("1. ILS (Iterated Local Search - recomendado)")
        print("   - Combina busca local intensiva com perturbações estratégicas")
        print("   - Ideal para problemas de grande escala com múltiplos mínimos locais")
        print("2. Simulated Annealing")
        print("   - Inspirado no processo físico de recozimento de metais")
        print("   - Aceita movimentos piores com probabilidade controlada para escapar de mínimos locais")
        print("3. GRASP puro")
        print("   - Construção gulosa aleatorizada seguida de busca local")
        print("   - Bom para construir soluções rapidamente com qualidade moderada")
        
        method_choice = self._get_choice("Escolha o método (1-3)", 1)
        metaheuristic_methods = {
            1: "ils",
            2: "simulated_annealing",
            3: "grasp"
        }
        self.config['meta_heuristic']['method'] = metaheuristic_methods[method_choice]
        
        # 2. Parâmetros gerais
        print("\nParâmetros gerais:")
        
        # Iterações
        max_iterations = self.config['meta_heuristic'].get('max_iterations', 100)
        print("• Número máximo de iterações - Define quantas iterações o algoritmo executará")
        print("  Valores maiores aumentam chances de encontrar boas soluções mas aumentam o tempo de execução")
        iter_input = input(f"Número máximo de iterações [{max_iterations}]: ").strip()
        if iter_input:
            self.config['meta_heuristic']['max_iterations'] = int(iter_input)
        
        # Iterações sem melhoria
        max_no_improvement = self.config['meta_heuristic'].get('max_iterations_without_improvement', 20)
        print("• Iterações sem melhoria - Critério de parada quando não há progresso")
        print("  Valores menores economizam tempo quando o algoritmo estagna")
        no_imp_input = input(f"Máximo de iterações sem melhoria [{max_no_improvement}]: ").strip()
        if no_imp_input:
            self.config['meta_heuristic']['max_iterations_without_improvement'] = int(no_imp_input)
        
        # 3. Parâmetros específicos do ILS
        if method_choice == 1:
            print("\nConfiguração do ILS:")
            
            # Perturbação
            perturb = self.config['meta_heuristic'].get('perturbation_strength', 0.2)
            print("• Intensidade da perturbação - Determina o grau de modificação da solução atual")
            print("  Valores maiores exploram mais o espaço de busca, valores menores são mais conservadores")
            print("  Intervalo recomendado: 0.1 (pequenas mudanças) a 0.5 (grandes modificações)")
            perturb_input = input(f"Intensidade da perturbação (0.0-1.0) [{perturb}]: ").strip()
            if perturb_input:
                try:
                    perturb_val = float(perturb_input)
                    if 0.0 <= perturb_val <= 1.0:
                        self.config['meta_heuristic']['perturbation_strength'] = perturb_val
                    else:
                        print("Valor deve estar entre 0.0 e 1.0. Usando valor padrão.")
                except ValueError:
                    print("Valor inválido. Usando valor padrão.")
            
            # TABU
            tabu_tenure = self.config['meta_heuristic'].get('tabu_tenure', 10)
            print("• Tamanho da lista TABU - Evita revisitar soluções recentes")
            print("  Valores maiores evitam ciclagem mas podem impedir retorno a boas regiões")
            print("  Recomendado: 7-15 para problemas médios, 20-30 para problemas grandes")
            tabu_input = input(f"Tamanho da lista TABU [{tabu_tenure}]: ").strip()
            if tabu_input:
                self.config['meta_heuristic']['tabu_tenure'] = int(tabu_input)
        
        # 4. Parâmetros do GRASP (usado no ILS e GRASP puro)
        if method_choice in [1, 3]:
            print("\nParâmetros do GRASP:")
            
            # Alpha (grau de aleatoriedade)
            alpha = self.config['meta_heuristic'].get('alpha', 0.2)
            print("• Parâmetro alpha - Controla o balanço entre guloso (0.0) e aleatório (1.0)")
            print("  Valores menores favorecem soluções de melhor qualidade inicial")
            print("  Valores maiores aumentam diversidade mas podem gerar soluções iniciais piores")
            alpha_input = input(f"Parâmetro alpha (0.0-1.0) [{alpha}]: ").strip()
            if alpha_input:
                try:
                    alpha_val = float(alpha_input)
                    if 0.0 <= alpha_val <= 1.0:
                        self.config['meta_heuristic']['alpha'] = alpha_val
                    else:
                        print("Valor deve estar entre 0.0 e 1.0. Usando valor padrão.")
                except ValueError:
                    print("Valor inválido. Usando valor padrão.")
            
            # RCL
            rcl_size = self.config['meta_heuristic'].get('rcl_size', 5)
            print("• Tamanho da RCL (Lista Restrita de Candidatos) - Número de candidatos considerados")
            print("  Valores maiores aumentam diversidade mas reduzem qualidade média das escolhas")
            print("  Recomendado: 3-5 para problemas pequenos, 5-10 para problemas maiores")
            rcl_input = input(f"Tamanho da RCL [{rcl_size}]: ").strip()
            if rcl_input:
                self.config['meta_heuristic']['rcl_size'] = int(rcl_input)
        
        # 5. Uso de GPU
        print("\nAceleração por GPU:")
        print("• Uso de GPU pode acelerar significativamente problemas grandes")
        print("  Recomendado ativar apenas se sua GPU tiver pelo menos 4GB de memória")
        print("  A aceleração é mais efetiva em instâncias com milhares de pedidos/corredores")
        use_gpu = self.config.get('algorithm', {}).get('use_gpu', False)
        gpu_choice = self._get_boolean("Usar aceleração GPU (recomendado para instâncias grandes)", use_gpu)
        self.config['algorithm']['use_gpu'] = gpu_choice
        
        # 6. Parâmetros avançados
        print("\nParâmetros adicionais:")
        
        # Debug (mostra progresso detalhado)
        debug = self.config['meta_heuristic'].get('debug', False)
        print("• Modo debug - Exibe informações detalhadas durante a execução")
        print("  Útil para entender o comportamento do algoritmo, mas pode reduzir desempenho")
        debug_choice = self._get_boolean("Ativar modo debug (mostra progresso detalhado)", debug)
        self.config['meta_heuristic']['debug'] = debug_choice
        
        # Semente aleatória
        seed = self.config['meta_heuristic'].get('seed', 42)
        print("• Semente aleatória - Permite reproduzir resultados entre execuções")
        print("  O mesmo valor sempre produzirá a mesma sequência de números aleatórios")
        seed_input = input(f"Semente aleatória (para reprodutibilidade) [{seed}]: ").strip()
        if seed_input:
            self.config['meta_heuristic']['seed'] = int(seed_input)
        
        print("\nParâmetros configurados com sucesso.")
        return self.config