import os
import sys
import glob
import time
import datetime
from prettytable import PrettyTable
from termcolor import colored
from src.models.problem import WaveOrderPickingProblem
from src.models.solution import WaveOrderPickingSolution
from src.utils.validator import SolutionValidator

class AdvancedSolutionValidator:
    """
    Validador avançado de soluções para o problema de Wave Order Picking.
    """

    # Valores BOV oficiais para cada instância
    BOVs_OFICIAIS = {
        "instance_0001": 15.00,
        "instance_0002": 2.00,
        "instance_0003": 12.00,
        "instance_0004": 3.50,
        "instance_0005": 177.88,
        "instance_0006": 691.00,
        "instance_0007": 392.25,
        "instance_0008": 162.94,
        "instance_0009": 4.42,
        "instance_0010": 16.79,
        "instance_0011": 16.85,
        "instance_0012": 11.25,
        "instance_0013": 117.38,
        "instance_0014": 181.64,
        "instance_0015": 149.33,
        "instance_0016": 85.00,
        "instance_0017": 36.50,
        "instance_0018": 117.20,
        "instance_0019": 202.00,
        "instance_0020": 5.00
    }

    # Símbolos para formatação do console
    BORDA_ES = "┌"
    BORDA_SD = "┐"
    BORDA_V = "│"
    BORDA_EJ = "├"
    BORDA_DJ = "┤"
    BORDA_DS = "└"
    BORDA_ID = "┘"

    @staticmethod
    def linhaHorizontal(largura):
        """Gera uma linha horizontal com a largura especificada."""
        return "─" * largura

    @staticmethod
    def cabecalho(texto):
        """Gera um cabeçalho formatado."""
        linha = "=" * (len(texto) + 4)
        return f"\n{linha}\n= {texto} =\n{linha}\n"

    @staticmethod
    def gerarNomeArquivoComTimestamp():
        """Gera um nome de arquivo com timestamp atual."""
        now = datetime.datetime.now()
        timestamp = now.strftime("%d%m%y-%H%M")
        return f"validation_log_{timestamp}.txt"

    @staticmethod
    def lerArquivoSolucao(arquivo_solucao):
        """
        Lê um arquivo de solução e retorna os pedidos e corredores.
        """
        try:
            with open(arquivo_solucao, 'r') as file:
                linhas = file.readlines()
                
                # Lê número de pedidos
                num_pedidos = int(linhas[0].strip())
                
                # Lê IDs dos pedidos (um por linha)
                pedidos_wave = []
                for i in range(1, num_pedidos + 1):
                    if i < len(linhas):
                        pedidos_wave.append(int(linhas[i].strip()))
                
                # Lê número de corredores
                if num_pedidos + 1 < len(linhas):
                    num_corredores = int(linhas[num_pedidos + 1].strip())
                else:
                    raise ValueError("Arquivo de solução incompleto: número de corredores ausente")
                
                # Lê IDs dos corredores (um por linha)
                corredores_wave = []
                for i in range(num_pedidos + 2, num_pedidos + 2 + num_corredores):
                    if i < len(linhas):
                        corredores_wave.append(int(linhas[i].strip()))
                
                return pedidos_wave, corredores_wave
        except Exception as e:
            raise ValueError(f"Erro ao ler arquivo de solução: {str(e)}")

    @staticmethod
    def validarRestricoes(problem, selected_orders, visited_aisles, logfile, nome_instancia):
        """
        Valida as restrições do problema para uma dada solução e retorna um relatório detalhado.
        """
        validacao_aprovada = True

        # Criar objeto Solution
        solution = problem.create_solution(selected_orders, visited_aisles)

        # 1. Validação dos IDs dos pedidos
        logfile.write("  1. Validação dos IDs dos pedidos: ")
        pedidos_validos = True
        for pedido_id in selected_orders:
            if pedido_id < 0 or pedido_id >= problem.n_orders:
                logfile.write("Reprovada\n")
                logfile.write(f"     Erro: ID de pedido inválido: {pedido_id} (intervalo válido: 0-{problem.n_orders-1})\n")
                validacao_aprovada = False
                pedidos_validos = False
                break
        if pedidos_validos:
            logfile.write("Aprovada\n")

        # 2. Validação dos IDs dos corredores
        logfile.write("  2. Validação dos IDs dos corredores: ")
        corredores_validos = True
        for corredor_id in visited_aisles:
            if corredor_id < 0 or corredor_id >= problem.n_aisles:
                logfile.write("Reprovada\n")
                logfile.write(f"     Erro: ID de corredor inválido: {corredor_id} (intervalo válido: 0-{problem.n_aisles-1})\n")
                validacao_aprovada = False
                corredores_validos = False
                break
        if corredores_validos:
            logfile.write("Aprovada\n")

        # 3. Validação do número total de unidades na wave
        logfile.write("  3. Validação do número total de unidades na wave: ")
        total_unidades = sum(problem.order_units.get(o, 0) for o in selected_orders)
        logfile.write(f"Total de unidades na wave: {total_unidades}, Limites LB e UB: {problem.wave_size_lb} - {problem.wave_size_ub}: ")

        unidades_validas = True
        if total_unidades < problem.wave_size_lb or total_unidades > problem.wave_size_ub:
            logfile.write("Reprovada\n")
            logfile.write(f"     Erro: Número total de unidades ({total_unidades}) fora dos limites LB e UB ({problem.wave_size_lb} - {problem.wave_size_ub})\n")
            validacao_aprovada = False
            unidades_validas = False
        if unidades_validas:
            logfile.write("Aprovada\n")

        # 4. Validação de estoque suficiente
        logfile.write("  4. Validação de estoque suficiente: ")
        
        # Calcular estoque disponível nos corredores selecionados
        estoque_disponivel = {}
        for corredor_id in visited_aisles:
            for item_id, quantidade in problem.aisles.get(corredor_id, {}).items():
                estoque_disponivel[item_id] = estoque_disponivel.get(item_id, 0) + quantidade
        
        # Verificar se há estoque suficiente para todos os pedidos
        estoque_suficiente = True
        for pedido_id in selected_orders:
            for item_id, quantidade_solicitada in problem.orders.get(pedido_id, {}).items():
                if estoque_disponivel.get(item_id, 0) < quantidade_solicitada:
                    logfile.write("Reprovada\n")
                    logfile.write(f"     Erro: Estoque insuficiente para o item {item_id} no pedido {pedido_id}\n")
                    logfile.write(f"       Quantidade solicitada: {quantidade_solicitada}\n")
                    logfile.write(f"       Estoque disponível: {estoque_disponivel.get(item_id, 0)}\n")
                    validacao_aprovada = False
                    estoque_suficiente = False
                    break
            if not estoque_suficiente:
                break
        
        if estoque_suficiente:
            logfile.write("Aprovada\n")

        # 5. Cálculo e comparação do valor objetivo
        valor_objetivo = SolutionValidator.calculate_objective(solution)
        logfile.write(f"  5. Valor objetivo (BOV): {valor_objetivo:.2f}\n")
        
        # Comparar com o BOV oficial
        nome_instancia_lower = nome_instancia.lower()
        if nome_instancia_lower in AdvancedSolutionValidator.BOVs_OFICIAIS:
            bov_oficial = AdvancedSolutionValidator.BOVs_OFICIAIS[nome_instancia_lower]
            diferenca = valor_objetivo - bov_oficial
            percentual = (diferenca / bov_oficial) * 100.0 if bov_oficial > 0 else 0.0
            
            logfile.write(f"     BOV oficial: {bov_oficial:.2f}\n")
            logfile.write(f"     Diferença: {diferenca:.2f}")
            
            if diferenca > 0:
                logfile.write(f" (+{percentual:.2f}% acima do BOV oficial)\n")
            elif diferenca < 0:
                logfile.write(f" ({percentual:.2f}% abaixo do BOV oficial)\n")
            else:
                logfile.write(" (igual ao BOV oficial)\n")
        else:
            logfile.write("     BOV oficial não disponível para esta instância\n")

        return validacao_aprovada, valor_objetivo

    @staticmethod
    def validar_resultados(config):
        """
        Interface principal para validação de resultados.
        """
        os.system('cls' if os.name == 'nt' else 'clear')
        print(AdvancedSolutionValidator.cabecalho("VALIDAÇÃO DE RESULTADOS"))

        largura = 80
        ss = []
        ss.append(f"{AdvancedSolutionValidator.BORDA_ES}{AdvancedSolutionValidator.linhaHorizontal(largura-2)}{AdvancedSolutionValidator.BORDA_SD}")
        ss.append(f"{AdvancedSolutionValidator.BORDA_V} {colored('CONFIGURAÇÕES DE VALIDAÇÃO', 'cyan', attrs=['bold'])}{' ' * (largura-33)}{AdvancedSolutionValidator.BORDA_V}")
        ss.append(f"{AdvancedSolutionValidator.BORDA_EJ}{AdvancedSolutionValidator.linhaHorizontal(largura-2)}{AdvancedSolutionValidator.BORDA_DJ}")
        
        # Diretórios de entrada e saída
        diretorio_entrada = config["paths"]["instances_dir"]
        diretorio_solucoes = config["paths"]["results_dir"]
        
        ss.append(f"{AdvancedSolutionValidator.BORDA_V} {colored('• Diretório de entrada:', 'cyan')} {diretorio_entrada}{' ' * (largura-25-len(diretorio_entrada))}{AdvancedSolutionValidator.BORDA_V}")
        ss.append(f"{AdvancedSolutionValidator.BORDA_V} {colored('• Diretório de soluções:', 'cyan')} {diretorio_solucoes}{' ' * (largura-26-len(diretorio_solucoes))}{AdvancedSolutionValidator.BORDA_V}")
        ss.append(f"{AdvancedSolutionValidator.BORDA_DS}{AdvancedSolutionValidator.linhaHorizontal(largura-2)}{AdvancedSolutionValidator.BORDA_ID}")
        
        for linha in ss:
            print(linha)
        print("\n")
        
        # Verificar diretórios
        if not os.path.exists(diretorio_entrada):
            print(colored("Erro: Diretório de entrada não existe.", "red"))
            input("\nPressione Enter para continuar...")
            return
            
        if not os.path.exists(diretorio_solucoes):
            print(colored("Erro: Diretório de soluções não existe.", "red"))
            input("\nPressione Enter para continuar...")
            return
            
        # Selecionar subdiretório/instância específica ou validar todas
        print("Escolha o modo de validação:")
        print("1. Validar todas as instâncias")
        print("2. Validar um subdiretório específico")
        print("3. Validar uma instância específica")
        
        modo = input("\nEscolha uma opção (1-3): ").strip()
        
        pares_arquivos = []
        
        if modo == "1":
            # Validar todas as instâncias
            for subdir in os.listdir(diretorio_entrada):
                subdir_path = os.path.join(diretorio_entrada, subdir)
                if os.path.isdir(subdir_path):
                    for entrada in glob.glob(os.path.join(subdir_path, "instance_*.txt")):
                        nome_arquivo = os.path.basename(entrada)
                        nome_sem_ext = os.path.splitext(nome_arquivo)[0]
                        
                        # Caminhos correspondentes para solução
                        solucao = os.path.join(diretorio_solucoes, subdir, f"{nome_sem_ext}_result.txt")
                        
                        if os.path.exists(solucao):
                            pares_arquivos.append((entrada, solucao, nome_sem_ext))
        
        elif modo == "2":
            # Validar um subdiretório específico
            subdirs = [d for d in os.listdir(diretorio_entrada) 
                      if os.path.isdir(os.path.join(diretorio_entrada, d))]
            
            if not subdirs:
                print(colored("Nenhum subdiretório encontrado.", "red"))
                input("\nPressione Enter para continuar...")
                return
                
            print("\nSubdiretórios disponíveis:")
            for i, subdir in enumerate(subdirs, 1):
                print(f"{i}. {subdir}/")
                
            try:
                idx = int(input("\nEscolha um subdiretório (número): ")) - 1
                if idx < 0 or idx >= len(subdirs):
                    print(colored("Opção inválida.", "red"))
                    input("\nPressione Enter para continuar...")
                    return
                    
                subdir = subdirs[idx]
                subdir_path = os.path.join(diretorio_entrada, subdir)
                
                for entrada in glob.glob(os.path.join(subdir_path, "instance_*.txt")):
                    nome_arquivo = os.path.basename(entrada)
                    nome_sem_ext = os.path.splitext(nome_arquivo)[0]
                    
                    # Caminhos correspondentes para solução
                    solucao = os.path.join(diretorio_solucoes, subdir, f"{nome_sem_ext}_result.txt")
                    
                    if os.path.exists(solucao):
                        pares_arquivos.append((entrada, solucao, nome_sem_ext))
                
            except ValueError:
                print(colored("Entrada inválida.", "red"))
                input("\nPressione Enter para continuar...")
                return
                
        elif modo == "3":
            # Validar uma instância específica
            subdirs = [d for d in os.listdir(diretorio_entrada) 
                      if os.path.isdir(os.path.join(diretorio_entrada, d))]
            
            if not subdirs:
                print(colored("Nenhum subdiretório encontrado.", "red"))
                input("\nPressione Enter para continuar...")
                return
                
            print("\nSubdiretórios disponíveis:")
            for i, subdir in enumerate(subdirs, 1):
                print(f"{i}. {subdir}/")
                
            try:
                idx = int(input("\nEscolha um subdiretório (número): ")) - 1
                if idx < 0 or idx >= len(subdirs):
                    print(colored("Opção inválida.", "red"))
                    input("\nPressione Enter para continuar...")
                    return
                    
                subdir = subdirs[idx]
                subdir_path = os.path.join(diretorio_entrada, subdir)
                
                instancias = sorted(glob.glob(os.path.join(subdir_path, "instance_*.txt")))
                
                if not instancias:
                    print(colored("Nenhuma instância encontrada neste subdiretório.", "red"))
                    input("\nPressione Enter para continuar...")
                    return
                    
                print("\nInstâncias disponíveis:")
                for i, inst in enumerate(instancias, 1):
                    print(f"{i}. {os.path.basename(inst)}")
                    
                idx_inst = int(input("\nEscolha uma instância (número): ")) - 1
                if idx_inst < 0 or idx_inst >= len(instancias):
                    print(colored("Opção inválida.", "red"))
                    input("\nPressione Enter para continuar...")
                    return
                    
                entrada = instancias[idx_inst]
                nome_arquivo = os.path.basename(entrada)
                nome_sem_ext = os.path.splitext(nome_arquivo)[0]
                
                # Caminhos correspondentes para solução
                solucao = os.path.join(diretorio_solucoes, subdir, f"{nome_sem_ext}_result.txt")
                
                if os.path.exists(solucao):
                    pares_arquivos.append((entrada, solucao, nome_sem_ext))
                else:
                    print(colored(f"Solução não encontrada para {nome_arquivo}.", "red"))
                    input("\nPressione Enter para continuar...")
                    return
                
            except ValueError:
                print(colored("Entrada inválida.", "red"))
                input("\nPressione Enter para continuar...")
                return
        
        else:
            print(colored("Opção inválida.", "red"))
            input("\nPressione Enter para continuar...")
            return
            
        if not pares_arquivos:
            print(colored("Nenhum par de arquivos encontrado para validação.", "red"))
            input("\nPressione Enter para continuar...")
            return
            
        # Criar diretório de log se não existir
        log_dir = os.path.join(diretorio_solucoes, "logs")
        os.makedirs(log_dir, exist_ok=True)
        
        # Gerar arquivo de log
        arquivo_log = os.path.join(log_dir, AdvancedSolutionValidator.gerarNomeArquivoComTimestamp())
        
        print(f"\nValidando {len(pares_arquivos)} pares de arquivos...\n")
        
        with open(arquivo_log, 'w', encoding='utf-8') as logfile:
            logfile.write("=== Relatório de Validação dos Resultados ===\n\n")
            
            resultados = {
                'aprovados': 0,
                'reprovados': 0,
                'erros': 0,
                'total': len(pares_arquivos),
                'bovs': []
            }
            
            for i, (entrada, solucao, nome_sem_ext) in enumerate(pares_arquivos, 1):
                print(f"[{i}/{len(pares_arquivos)}] Validando: {os.path.basename(entrada)}")
                
                logfile.write(f"Arquivo de entrada: {entrada}\n")
                logfile.write(f"Arquivo de solução: {solucao}\n")
                
                try:
                    # Carregar o problema
                    problem = WaveOrderPickingProblem(config=config)
                    problem.read_input(entrada)
                    
                    # Ler arquivo de solução
                    selected_orders, visited_aisles = AdvancedSolutionValidator.lerArquivoSolucao(solucao)
                    
                    # Validar a solução
                    validada, bov = AdvancedSolutionValidator.validarRestricoes(
                        problem, selected_orders, visited_aisles, logfile, nome_sem_ext
                    )
                    
                    if validada:
                        resultados['aprovados'] += 1
                    else:
                        resultados['reprovados'] += 1
                        
                    resultados['bovs'].append((nome_sem_ext, bov, validada))
                    
                    logfile.write(f"Validação: {'Aprovada' if validada else 'Reprovada'}\n")
                    
                except Exception as e:
                    logfile.write(f"Erro ao validar: {str(e)}\n")
                    logfile.write("Validação: Reprovada (erro)\n")
                    resultados['erros'] += 1
                    
                logfile.write("----------------------------------------\n")
            
            # Resumo final
            logfile.write("\n=== Resumo da Validação ===\n")
            logfile.write(f"Total de instâncias: {resultados['total']}\n")
            logfile.write(f"Soluções aprovadas: {resultados['aprovados']} ({resultados['aprovados']/resultados['total']*100:.1f}%)\n")
            logfile.write(f"Soluções reprovadas: {resultados['reprovados']} ({resultados['reprovados']/resultados['total']*100:.1f}%)\n")
            logfile.write(f"Erros de validação: {resultados['erros']} ({resultados['erros']/resultados['total']*100:.1f}%)\n\n")
            
            # Tabela de BOVs
            if resultados['bovs']:
                logfile.write("=== Valores Objetivo (BOVs) ===\n")
                table = PrettyTable()
                table.field_names = ["Instância", "BOV Calculado", "BOV Oficial", "Diferença", "Status"]
                
                for nome, bov, validada in sorted(resultados['bovs']):
                    bov_oficial = AdvancedSolutionValidator.BOVs_OFICIAIS.get(nome.lower(), "N/A")
                    
                    if bov_oficial != "N/A":
                        diferenca = bov - bov_oficial
                        perc = f"({diferenca/bov_oficial*100:+.2f}%)" if bov_oficial > 0 else ""
                        dif_str = f"{diferenca:+.2f} {perc}"
                    else:
                        dif_str = "N/A"
                        
                    status = "Viável" if validada else "Inviável"
                    
                    table.add_row([
                        nome, 
                        f"{bov:.2f}", 
                        f"{bov_oficial if bov_oficial != 'N/A' else 'N/A'}", 
                        dif_str,
                        status
                    ])
                
                logfile.write(table.get_string())
        
        # Exibir resumo na tela
        print("\n" + "=" * 60)
        print(colored("RESUMO DA VALIDAÇÃO", "cyan", attrs=["bold"]).center(60))
        print("=" * 60)
        print(f"Total de instâncias: {resultados['total']}")
        print(f"Soluções aprovadas: {colored(str(resultados['aprovados']), 'green')} ({resultados['aprovados']/resultados['total']*100:.1f}%)")
        print(f"Soluções reprovadas: {colored(str(resultados['reprovados']), 'red')} ({resultados['reprovados']/resultados['total']*100:.1f}%)")
        print(f"Erros de validação: {colored(str(resultados['erros']), 'yellow')} ({resultados['erros']/resultados['total']*100:.1f}%)")
        
        print("\n" + colored(f"Relatório detalhado salvo em: {arquivo_log}", "green", attrs=["bold"]))
        input("\nPressione Enter para continuar...")