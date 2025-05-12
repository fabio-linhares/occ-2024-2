#!/usr/bin/env bash
# Script para executar o projeto Wave Order Picking com CPLEX no ambiente FHS

# Define o caminho do CPLEX
export CPLEX_STUDIO_DIR="/opt/ibm/ILOG/CPLEX_Studio2212"

echo "======================================================"
echo "Executando Wave Order Picking com CPLEX em ambiente FHS"
echo "======================================================"

# Verifica se a variável de ambiente está configurada
if [ -z "$CPLEX_STUDIO_DIR" ]; then
    echo "ERRO: Variável CPLEX_STUDIO_DIR não configurada"
    echo "Defina o caminho para o diretório do CPLEX, ex:"
    echo "export CPLEX_STUDIO_DIR=/opt/ibm/ILOG/CPLEX_Studio2212"
    exit 1
fi

# Verifica se o diretório existe
if [ ! -d "$CPLEX_STUDIO_DIR" ]; then
    echo "ERRO: Diretório CPLEX não encontrado: $CPLEX_STUDIO_DIR"
    exit 1
fi

# Inicia o projeto no ambiente FHS
echo "Iniciando aplicação no ambiente FHS..."
nix-shell cuda2.nix --run "source venv/bin/activate && python main.py"
