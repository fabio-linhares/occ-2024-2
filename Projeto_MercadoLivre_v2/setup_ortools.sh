#!/usr/bin/env bash
# Encontrar o OR-Tools instalado pelo Nix

# Buscar diretório do OR-Tools
ORTOOLS_PATH=$(find /nix/store -name "or-tools-*" -type d | head -n 1)

if [ -z "$ORTOOLS_PATH" ]; then
    echo "OR-Tools não encontrado!"
    exit 1
fi

echo "Encontrado OR-Tools em: $ORTOOLS_PATH"

# Configurar variáveis de ambiente
export CMAKE_PREFIX_PATH=$ORTOOLS_PATH:$CMAKE_PREFIX_PATH
export LD_LIBRARY_PATH=$ORTOOLS_PATH/lib:$LD_LIBRARY_PATH
export CPLUS_INCLUDE_PATH=$ORTOOLS_PATH/include:$CPLUS_INCLUDE_PATH

echo "Variáveis de ambiente configuradas."
echo "Execute 'source setup_ortools.sh' antes de compilar."