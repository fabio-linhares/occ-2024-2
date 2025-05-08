#!/usr/bin/env bash
# filepath: compila.sh

# Definir cores para melhor legibilidade
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Função para exibir mensagens com formatação
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCESSO]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[AVISO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERRO]${NC} $1"
}

# Função para verificar erros
check_error() {
    if [ $? -ne 0 ]; then
        log_error "$1"
        exit 1
    fi
}

# Diretório do projeto (onde está o script)
PROJECT_DIR=$(pwd)
BUILD_DIR="${PROJECT_DIR}/build"

# Processar argumentos
CLEAN=0
REBUILD=0
DEBUG=0

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -c|--clean) CLEAN=1 ;;
        -r|--rebuild) REBUILD=1 ;;
        -d|--debug) DEBUG=1 ;;
        -h|--help) 
            echo "Uso: ./compila.sh [opções]"
            echo "Opções:"
            echo "  -c, --clean    Limpar diretório de build"
            echo "  -r, --rebuild  Reconstruir completamente (clean + build)"
            echo "  -d, --debug    Compilar em modo Debug (com informações de depuração)"
            echo "  -h, --help     Mostrar esta ajuda"
            exit 0
            ;;
        *) echo "Opção desconhecida: $1"; exit 1 ;;
    esac
    shift
done

# Limpar se solicitado
if [ $CLEAN -eq 1 ] || [ $REBUILD -eq 1 ]; then
    log_info "Limpando diretório de build..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        check_error "Falha ao limpar diretório de build"
    fi
    
    if [ $CLEAN -eq 1 ]; then
        log_success "Limpeza concluída."
        exit 0
    fi
fi

# Criar diretório de build se não existir
if [ ! -d "$BUILD_DIR" ]; then
    log_info "Criando diretório de build..."
    mkdir -p "$BUILD_DIR"
    check_error "Falha ao criar diretório de build"
fi

# Ir para o diretório de build
cd "$BUILD_DIR"
check_error "Falha ao navegar para diretório de build"

# Configurar o projeto com CMake
log_info "Configurando projeto com CMake..."

# Definir tipo de build
BUILD_TYPE="Release"
if [ $DEBUG -eq 1 ]; then
    BUILD_TYPE="Debug"
    log_info "Modo de compilação: Debug"
else
    log_info "Modo de compilação: Release"
fi

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
check_error "Falha na configuração do CMake"
log_success "Configuração concluída"

# Compilar o projeto
log_info "Compilando projeto..."
make -j$(nproc)
check_error "Falha na compilação"
log_success "Compilação concluída com sucesso!"

# Informações finais
log_info "Arquivos gerados:"
if [ -f "MercadoLivre" ]; then
    log_success "✓ Executável principal: ${BUILD_DIR}/MercadoLivre"
else
    log_warning "✗ Executável principal não encontrado"
fi

if [ -f "parser_test" ]; then
    log_success "✓ Ferramenta de diagnóstico: ${BUILD_DIR}/parser_test"
else
    log_warning "✗ Ferramenta de diagnóstico não encontrada"
fi

if [ -f "libMercadoLivreCore.a" ]; then
    log_success "✓ Biblioteca principal: ${BUILD_DIR}/libMercadoLivreCore.a"
else
    log_warning "✗ Biblioteca principal não encontrada"
fi

# Verificar se a estrutura de diretórios necessária existe
log_info "Verificando estrutura de diretórios..."
if [ ! -d "${PROJECT_DIR}/data/input" ]; then
    log_warning "Diretório data/input não encontrado. Criando..."
    mkdir -p "${PROJECT_DIR}/data/input"
fi

if [ ! -d "${PROJECT_DIR}/data/output" ]; then
    log_warning "Diretório data/output não encontrado. Criando..."
    mkdir -p "${PROJECT_DIR}/data/output"
fi

# Verificar se o arquivo de teste existe
if [ ! -f "${PROJECT_DIR}/data/input/test_instance.txt" ]; then
    log_warning "Arquivo de teste não encontrado em data/input/test_instance.txt"
    log_info "Para testar o parser, crie um arquivo de teste neste local."
fi

log_info "Para executar o programa principal: ${BUILD_DIR}/MercadoLivre"
log_info "Para testar o parser: ${BUILD_DIR}/parser_test [arquivo_instancia]"

exit 0