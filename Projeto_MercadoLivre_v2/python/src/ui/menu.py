import os
from src.ui.console import clear_screen

def load_menu_options(file_path):
    """Carrega opções de menu a partir de um arquivo txt."""
    menu_options = []
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            for line in file:
                # Ignora comentários e linhas vazias
                if line.strip() and not line.startswith('#'):
                    parts = line.strip().split(':')
                    # Verifica se a linha tem pelo menos uma parte (antes do ':')
                    if len(parts) >= 1:
                        title = parts[0].strip()
                        # Captura o que está após os dois pontos (se existir)
                        value = parts[1].strip() if len(parts) > 1 else ""
                        menu_options.append((title, value))
    except FileNotFoundError:
        print(f"Arquivo de menu não encontrado: {file_path}")
        print("Criando arquivo de menu padrão...")
        create_default_menu_file(file_path)
        return load_menu_options(file_path)
    
    # Adicionar "Sair" no início do menu (remova se já existir em outra posição)
    menu_options = [(option, value) for option, value in menu_options if option != "Sair"]
    menu_options.insert(0, ("Sair", ""))
    
    return menu_options

def create_default_menu_file(file_path):
    """Cria um arquivo de menu padrão."""
    directory = os.path.dirname(file_path)
    if directory:
        os.makedirs(directory, exist_ok=True)
    
    with open(file_path, 'w', encoding='utf-8') as file:
        file.write("# Arquivo de configuração do menu\n")
        file.write("# Formato: Título da opção:caminho_do_arquivo.md ou função\n")
        file.write("# - Se houver extensão (.md, .txt, etc), é interpretado como arquivo\n")
        file.write("# - Se não houver extensão, é interpretado como nome de função\n\n")
        file.write("Exibir informações do desafio:docs/desafio.md\n")
        file.write("Exibir informações do problema:docs/problema.md\n")
        file.write("Exibir informações da função objetivo e restrições:docs/funcao_objetivo.md\n")
        file.write("Exibir informações das instâncias:docs/instancias.md\n")
        file.write("Exibir informações de validação:docs/validacao.md\n")
        file.write("Resolver desafio:solve_challenge\n")

def display_menu(menu_options):
    """Exibe o menu e retorna a opção selecionada."""
    clear_screen()
    print("=" * 60)
    print("MERCADO LIVRE WAVE PICKING OPTIMIZATION CHALLENGE".center(60))
    print("=" * 60)
    print("Projeto de Otimização:".center(60))
    print("Picking (Inspirado no Desafio SBPO 2025)".center(60))
    print()
    print("Autores:".center(60))
    print("Fábio Linhares".center(60))
    print("Luryan Delevati".center(60))
    print("Hans Aragão".center(60))
    print("".center(60))
    print("Disciplina: Otimização Contínua e Combinatória".center(60))
    print("Professores:".center(60))
    print("Bruno Costa e Silva Nogueira".center(60))
    print("Rian Gabriel Santos Pinheiro".center(60))
    print("".center(60))
    print("Universidade Federal de Alagoas (UFAL)".center(60))
    print("Mestrado em Informática".center(60))
    print()
    
    print(f"\n\nOpções")
    print()
    # Exibe as demais opções começando de 1
    for i, (option, _) in enumerate(menu_options[1:], 1):
        print(f"{i}. {option}")
    
    # Exibe a opção Sair como 0
    print(f"\n0. {menu_options[0][0]}")
    
    print()
    while True:
        try:
            choice = int(input("Escolha uma opção: "))
            if 0 <= choice <= len(menu_options) - 1:
                return choice
            else:
                print(f"Por favor, escolha um número entre 0 e {len(menu_options) - 1}.")
        except ValueError:
            print("Por favor, digite um número válido.")