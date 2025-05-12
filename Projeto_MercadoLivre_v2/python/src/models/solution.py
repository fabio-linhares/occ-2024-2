class WaveOrderPickingSolution:
    """
    Representa uma solução para o problema de Wave Order Picking.
    Unifica as funcionalidades de WaveOrderPickingSolution e WaveOrderPickingSolution.
    """
    
    def __init__(self, selected_orders, visited_aisles, is_feasible=None, objective_value=None, total_units=0):
        """
        Inicializa uma solução.
        
        Args:
            selected_orders (list): Índices dos pedidos selecionados
            visited_aisles (list): Índices dos corredores visitados
            is_feasible (bool, optional): Indica se a solução é viável
            objective_value (float, optional): Valor da função objetivo
            total_units (int, optional): Total de unidades coletadas
        """
        self.selected_orders = selected_orders
        self.visited_aisles = visited_aisles
        self.is_feasible = is_feasible
        self.objective_value = objective_value
        self.total_units = total_units
    
    def set_feasibility(self, is_feasible):
        """Define se a solução é viável."""
        self.is_feasible = is_feasible
        return self
    
    def set_objective_value(self, value):
        """Define o valor da função objetivo."""
        self.objective_value = value
        return self
    
    def set_total_units(self, total_units):
        """Define o total de unidades coletadas."""
        self.total_units = total_units
        return self
    
    def to_output_format(self):
        """
        Converte a solução para o formato de saída do desafio.
        
        Returns:
            tuple: (pedidos_selecionados, corredores_visitados)
        """
        return self.selected_orders, self.visited_aisles
    
    def __str__(self):
        """Representação em string da solução."""
        return (f"WaveOrderPickingSolution(feasible={self.is_feasible}, "
                f"objective={self.objective_value:.4f if self.objective_value else 'None'}, "
                f"orders={len(self.selected_orders)}, "
                f"aisles={len(self.visited_aisles)}, "
                f"units={self.total_units})")