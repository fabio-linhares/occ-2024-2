#!/usr/bin/env bash

# Script para compilar e testar o projeto Mercado Livre
# Autor: GitHub Copilot
# Data: 25 de abril de 2025
# Uso: ./compila.sh [limpar] [testar] [executar] [ajuda] [paralelo=N]
# - limpar: Remove o diretório build antes de compilar
# - testar: Executa os testes após a compilação
# - executar: Executa o programa principal após a compilação
# - ajuda: Mostra esta mensagem de ajuda
# - paralelo=N: Define o nível de paralelismo para a compilação (0 = auto-detectar)

set -e  # Encerra o script se qualquer comando falhar

# Cores para melhorar a visualização
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Diretório base do projeto
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${PROJECT_DIR}/build"

# Parâmetros configuráveis
NUM_THREADS=$(nproc)
PARALLEL_LEVEL=0  # 0 = auto-detectar

# Função para exibir mensagens
print_msg() {
    echo -e "${2:-$BLUE}$1${NC}"
}

# Função para exibir mensagens de erro
error_msg() {
    echo -e "${RED}ERRO: $1${NC}" >&2
    exit 1
}

# Função para exibir ajuda
show_help() {
    echo "Uso: ./compila.sh [limpar] [testar] [executar] [ajuda] [paralelo=N]"
    echo "  limpar: Remove o diretório build antes de compilar"
    echo "  testar: Executa os testes após a compilação"
    echo "  executar: Executa o programa principal após a compilação"
    echo "  ajuda: Mostra esta mensagem de ajuda"
    echo "  paralelo=N: Define o nível de paralelismo para a compilação (0 = auto-detectar)"
}

# Parâmetros
CLEAN=0
RUN_TESTS=0
RUN_EXECUTABLE=0

# Processar argumentos
for arg in "$@"
do
    case $arg in
        limpar)
            CLEAN=1
            ;;
        testar)
            RUN_TESTS=1
            ;;
        executar)
            RUN_EXECUTABLE=1
            ;;
        ajuda)
            show_help
            exit 0
            ;;
        paralelo=*)
            PARALLEL_LEVEL="${arg#*=}"
            ;;
        *)
            echo "Argumento desconhecido: $arg"
            show_help
            exit 1
            ;;
    esac
done

# Verificar dependências
check_dependencies() {
    print_msg "Verificando dependências..."
    
    # Verificar CMake
    if ! command -v cmake &> /dev/null; then
        error_msg "CMake não encontrado. Por favor, instale-o antes de continuar."
    fi
    
    # Verificar compilador C++
    if ! command -v g++ &> /dev/null; then
        error_msg "Compilador g++ não encontrado. Por favor, instale-o antes de continuar."
    fi
    
    print_msg "Todas as dependências estão instaladas." "$GREEN"
}

# Criar diretórios necessários
create_directories() {
    print_msg "Criando diretórios necessários..."
    
    # Verificar se os diretórios existem e criar se necessário
    mkdir -p "${PROJECT_DIR}/src/config"
    mkdir -p "${PROJECT_DIR}/src/app"
    mkdir -p "${PROJECT_DIR}/include/config"
    mkdir -p "${PROJECT_DIR}/include/app"
    mkdir -p "${PROJECT_DIR}/test/config"
    
    print_msg "Diretórios criados com sucesso." "$GREEN"
}

# Limpar diretório build se solicitado
clean_build() {
    if [ $CLEAN -eq 1 ]; then
        print_msg "Limpando diretório build..."
        rm -rf "$BUILD_DIR"
        print_msg "Diretório build removido." "$GREEN"
    fi
}

# Executar CMake e compilar
build_project() {
    print_msg "Configurando projeto com CMake..."
    
    # Criar diretório build se não existir
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configurar CMake
    cmake ..
    
    # Compilar
    if [ "$PARALLEL_LEVEL" -eq 0 ]; then
        PARALLEL_LEVEL=$NUM_THREADS
    fi
    
    print_msg "Compilando projeto usando $PARALLEL_LEVEL threads..."
    cmake --build . -- -j$PARALLEL_LEVEL
    
    print_msg "Compilação concluída com sucesso!" "$GREEN"
}

# Executar testes
run_tests() {
    if [ $RUN_TESTS -eq 1 ]; then
        print_msg "Executando testes..."
        cd "$BUILD_DIR"
        
        # Executar testes e capturar o resultado
        if ctest . --output-on-failure; then
            print_msg "Todos os testes passaram!" "$GREEN"
        else
            print_msg "Alguns testes falharam. Verifique os detalhes acima." "$RED"
        fi
    fi
}

# Executar o programa principal
run_executable() {
    if [ $RUN_EXECUTABLE -eq 1 ]; then
        print_msg "Executando o programa principal..."
        cd "$BUILD_DIR"
        
        if [ -f "./MercadoLivre" ]; then
            ./MercadoLivre
        else
            print_msg "Executável 'MercadoLivre' não encontrado." "$RED"
        fi
    fi
}

# Execução principal
main() {
    print_msg "======== INICIANDO SCRIPT DE COMPILAÇÃO ========"
    
    # Mostrar diretório do projeto
    print_msg "Diretório do projeto: $PROJECT_DIR"
    
    # Executar as etapas
    check_dependencies
    create_directories
    clean_build
    build_project
    run_tests
    run_executable
    
    print_msg "======== SCRIPT DE COMPILAÇÃO CONCLUÍDO ========"
}

main