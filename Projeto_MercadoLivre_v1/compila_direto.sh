#!/usr/bin/env bash

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Compilando interface básica...${NC}"

# Criar diretórios necessários
mkdir -p build
mkdir -p data/input
mkdir -p data/output

# Compilar apenas os arquivos necessários para a interface básica
g++ -std=c++17 \
    src/main.cpp \
    src/io/file_utils.cpp \
    src/parser/instance_parser.cpp \
    src/ui/menu.cpp \
    -I. -Isrc -Iinclude \
    -o build/interface_ml \
    -pthread

# Verificar resultado da compilação
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Compilação concluída com sucesso!${NC}"
    echo "Executável gerado: build/interface_ml"
    
    # Tornar executável
    chmod +x build/interface_ml
    
    echo -e "${YELLOW}ATENÇÃO:${NC} Esta é uma versão reduzida que contém apenas a interface básica."
    echo "Para executar: ./build/interface_ml"
else
    echo -e "${RED}Erro na compilação!${NC}"
    exit 1
fi