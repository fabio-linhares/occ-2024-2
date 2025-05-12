import os

def clear_screen():
    """Limpa a tela do console."""
    os.system('cls' if os.name == 'nt' else 'clear')

def display_markdown_file(file_path):
    """Exibe o conteúdo de um arquivo markdown no console."""
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            md_content = file.read()
            print(md_content)
            
        input("\nPressione Enter para continuar...")
    except FileNotFoundError:
        print(f"Arquivo não encontrado: {file_path}")
        input("\nPressione Enter para continuar...")