#!/usr/bin/env bash

# Script para verificar e criar a estrutura de diretórios e arquivos do projeto
# Uso: chmod +x setup_project_structure.sh && ./setup_project_structure.sh

# Diretório base do projeto
BASE_DIR="Projeto_MercadoLivre"

# Lista de diretórios que devem existir
dirs=(
  "$BASE_DIR"
  "$BASE_DIR/data"
  "$BASE_DIR/include"
  "$BASE_DIR/include/core"
  "$BASE_DIR/include/input"
  "$BASE_DIR/include/output"
  "$BASE_DIR/include/algorithm"
  "$BASE_DIR/include/utils"
  "$BASE_DIR/src"
  "$BASE_DIR/src/core"
  "$BASE_DIR/src/input"
  "$BASE_DIR/src/output"
  "$BASE_DIR/src/algorithm"
  "$BASE_DIR/src/utils"
  "$BASE_DIR/test"
  "$BASE_DIR/test/core"
  "$BASE_DIR/test/input"
  "$BASE_DIR/test/utils"
  "$BASE_DIR/docs"
)

# Lista de arquivos que devem existir
files=(
  "$BASE_DIR/CMakeLists.txt"
  "$BASE_DIR/.gitignore"
  "$BASE_DIR/README.md"
  "$BASE_DIR/data/instance1.txt"
  "$BASE_DIR/data/instance2.txt"
  "$BASE_DIR/include/core/item.h"
  "$BASE_DIR/include/core/order.h"
  "$BASE_DIR/include/core/corridor.h"
  "$BASE_DIR/include/core/warehouse.h"
  "$BASE_DIR/include/core/solution.h"
  "$BASE_DIR/include/input/input_parser.h"
  "$BASE_DIR/include/output/output_writer.h"
  "$BASE_DIR/include/algorithm/optimization_algorithm.h"
  "$BASE_DIR/include/algorithm/greedy_algorithm.h"
  "$BASE_DIR/include/utils/logger.h"
  "$BASE_DIR/include/config.h"
  "$BASE_DIR/src/core/item.cpp"
  "$BASE_DIR/src/core/order.cpp"
  "$BASE_DIR/src/core/corridor.cpp"
  "$BASE_DIR/src/core/warehouse.cpp"
  "$BASE_DIR/src/core/solution.cpp"
  "$BASE_DIR/src/input/input_parser.cpp"
  "$BASE_DIR/src/output/output_writer.cpp"
  "$BASE_DIR/src/algorithm/optimization_algorithm.cpp"
  "$BASE_DIR/src/algorithm/greedy_algorithm.cpp"
  "$BASE_DIR/src/utils/logger.cpp"
  "$BASE_DIR/src/main.cpp"
  "$BASE_DIR/test/core/item_test.cpp"
  "$BASE_DIR/test/core/order_test.cpp"
  "$BASE_DIR/test/input/input_parser_test.cpp"
  "$BASE_DIR/test/utils/logger_test.cpp"
  "$BASE_DIR/test/CMakeLists.txt"
)

# Cria diretórios faltantes
for dir in "${dirs[@]}"; do
  if [ ! -d "$dir" ]; then
    mkdir -p "$dir"
    echo "Criado diretório: $dir"
  fi
done

# Cria arquivos faltantes
for file in "${files[@]}"; do
  if [ ! -f "$file" ]; then
    touch "$file"
    echo "Criado arquivo: $file"
  fi
done

echo "Estrutura de projeto verificada e atualizada."

