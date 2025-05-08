#!/usr/bin/env bash

# Script para compilar e testar o projeto MercadoLivre_v2
# Uso: ./compila.sh [limpar] [testar] [executar] [ajuda] [paralelo=N] [relatorio] [instalar] [depurar]

# Cores para melhorar a visualização
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Diretório base do projeto
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${PROJECT_DIR}/build"
CMAKE_BUILD_TYPE="Release"

# Parâmetros configuráveis
NUM_THREADS=$(nproc)
PARALLEL_LEVEL=0  # 0 = auto-detectar

# Banner do projeto
show_banner() {
    echo -e "${BOLD}${BLUE}╔══════════════════════════════════════════════════════════════════════╗"
    echo -e "║                                                                      ║"
    echo -e "║   ${YELLOW}Projeto MercadoLivre v2 - SBPO 2025${BLUE}                                ║"
    echo -e "║   ${GREEN}Sistema de Otimização de Waves para Pedidos${BLUE}                        ║"
    echo -e "║                                                                      ║"
    echo -e "╚══════════════════════════════════════════════════════════════════════╝${NC}"
}

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
    echo -e "${BOLD}${CYAN}=== AJUDA DO SCRIPT DE COMPILAÇÃO ===${NC}"
    echo "Uso: ./compila.sh [opções]"
    echo ""
    echo "Opções disponíveis:"
    echo -e "  ${YELLOW}limpar${NC}        - Remove o diretório build antes de compilar"
    echo -e "  ${YELLOW}testar${NC}        - Executa os testes após a compilação"
    echo -e "  ${YELLOW}executar${NC}      - Executa o programa principal após a compilação"
    echo -e "  ${YELLOW}ajuda${NC}         - Mostra esta mensagem de ajuda"
    echo -e "  ${YELLOW}paralelo=N${NC}    - Define o nível de paralelismo para a compilação (0 = auto-detectar)"
    echo -e "  ${YELLOW}relatorio${NC}     - Gera um relatório do projeto após a compilação"
    echo -e "  ${YELLOW}instalar${NC}      - Instala o programa no sistema (requer sudo)"
    echo -e "  ${YELLOW}depurar${NC}       - Compila em modo de depuração"
    echo ""
    echo "Exemplos:"
    echo -e "  ${GREEN}./compila.sh limpar testar${NC}       - Limpa, compila e testa o projeto"
    echo -e "  ${GREEN}./compila.sh executar${NC}            - Compila e executa o projeto"
}

# Parâmetros
CLEAN=0
RUN_TESTS=0
RUN_EXECUTABLE=0
GENERATE_REPORT=0
INSTALL_PROGRAM=0

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
            show_banner
            show_help
            exit 0
            ;;
        paralelo=*)
            PARALLEL_LEVEL="${arg#*=}"
            ;;
        relatorio)
            GENERATE_REPORT=1
            ;;
        instalar)
            INSTALL_PROGRAM=1
            ;;
        depurar)
            CMAKE_BUILD_TYPE="Debug"
            ;;
        *)
            echo -e "${RED}Argumento desconhecido: $arg${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Verificar dependências
check_dependencies() {
    print_msg "Verificando dependências..." "${YELLOW}"
    
    # Verificar CMake
    if ! command -v cmake &> /dev/null; then
        error_msg "CMake não encontrado. Por favor, instale-o antes de continuar."
    fi
    
    # Verificar compilador C++
    if ! command -v g++ &> /dev/null; then
        error_msg "Compilador g++ não encontrado. Por favor, instale-o antes de continuar."
    fi
    
    print_msg "Todas as dependências estão instaladas." "${GREEN}"
}

# Criar diretórios necessários
create_directories() {
    print_msg "Criando diretórios necessários..." "${YELLOW}"
    
    # Verificar se os diretórios existem e criar se necessário
    mkdir -p "${PROJECT_DIR}/src"
    mkdir -p "${PROJECT_DIR}/include"
    mkdir -p "${PROJECT_DIR}/test"
    mkdir -p "${PROJECT_DIR}/data/input"
    mkdir -p "${PROJECT_DIR}/data/output"
    
    print_msg "Diretórios criados com sucesso." "${GREEN}"
}

# Limpar diretório build se solicitado
clean_build() {
    if [ $CLEAN -eq 1 ]; then
        print_msg "Limpando diretório build..." "${YELLOW}"
        rm -rf "$BUILD_DIR"
        print_msg "Diretório build removido." "${GREEN}"
    fi
}

# Build do projeto
build_project() {
    print_msg "Configurando projeto com CMake (modo: $CMAKE_BUILD_TYPE)..." "${YELLOW}"
    
    # Criar diretório build se não existir
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Limpar arquivo de erros anterior se existir
    ERROS_FILE="${PROJECT_DIR}/erros_compilacao"
    > "$ERROS_FILE"
    
    print_msg "Redirecionando erros para: $ERROS_FILE" "${YELLOW}"
    
    # Configurar CMake com redirecionamento de erros
    cmake -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" .. 2>> "$ERROS_FILE" || { 
        print_msg "Falha na configuração do CMake. Verifique $ERROS_FILE" "${RED}"; 
        grep -i -E "error:|erro:|fatal:" "$ERROS_FILE"
        exit 1; 
    }
    
    # Compilar com redirecionamento de erros
    if [ "$PARALLEL_LEVEL" -eq 0 ]; then
        PARALLEL_LEVEL=$NUM_THREADS
    fi
    
    print_msg "Compilando projeto usando $PARALLEL_LEVEL threads..." "${YELLOW}"
    cmake --build . -- -j"$PARALLEL_LEVEL" 2>> "$ERROS_FILE" || {
        print_msg "Falha na compilação do projeto. Verifique $ERROS_FILE" "${RED}"; 
        grep -i -E "error:|erro:|fatal:" "$ERROS_FILE"
        exit 1;
    }
    
    # Verificar se houve erros reais (não apenas avisos)
    if grep -q -i "error:" "$ERROS_FILE" || grep -q -i "erro:" "$ERROS_FILE" || grep -q -i "fatal:" "$ERROS_FILE"; then
        print_msg "⚠️  Ocorreram erros durante a compilação. Verifique o arquivo: $ERROS_FILE" "${RED}"
        # Exibir os erros encontrados
        grep -i -E "error:|erro:|fatal:" "$ERROS_FILE"
    else
        if [ -s "$ERROS_FILE" ]; then
            print_msg "Compilação concluída com sucesso, mas com avisos. Verifique $ERROS_FILE para detalhes." "${YELLOW}"
        else
            print_msg "Compilação concluída com sucesso (sem erros nem avisos)!" "${GREEN}"
        fi
    fi
}

# Executar testes
run_tests() {
    if [ $RUN_TESTS -eq 1 ]; then
        print_msg "Executando testes..." "${YELLOW}"
        cd "$BUILD_DIR"
        
        # Executar testes e capturar o resultado
        if ctest . --output-on-failure; then
            print_msg "✓ Todos os testes passaram!" "${GREEN}"
        else
            print_msg "✗ Alguns testes falharam. Verifique os detalhes acima." "${RED}"
        fi
    fi
}

# Executar o programa principal
run_executable() {
    if [ $RUN_EXECUTABLE -eq 1 ]; then
        print_msg "Executando o programa principal..." "${YELLOW}"
        cd "$BUILD_DIR"
        
        if [ -f "./MercadoLivre_v2" ]; then
            echo -e "${PURPLE}=================================================${NC}"
            ./MercadoLivre_v2
            echo -e "${PURPLE}=================================================${NC}"
        else
            print_msg "Executável 'MercadoLivre_v2' não encontrado." "${RED}"
        fi
    fi
}

# Execução principal
main() {
    show_banner
    print_msg "======== INICIANDO SCRIPT DE COMPILAÇÃO ========" "${CYAN}"
    
    # Mostrar diretório do projeto
    print_msg "Diretório do projeto: $PROJECT_DIR" "${PURPLE}"
    print_msg "Modo de compilação: $CMAKE_BUILD_TYPE" "${PURPLE}"
    
    # Executar as etapas
    check_dependencies
    create_directories
    clean_build
    build_project
    run_tests
    run_executable
    
    print_msg "======== SCRIPT DE COMPILAÇÃO CONCLUÍDO ========" "${CYAN}"
}

main