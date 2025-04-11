#!/run/current-system/sw/bin/bash

#################################################################################################
#
# Explicação do Script:
# 
# Variáveis: O script define variáveis para os nomes dos diretórios e arquivos, 
#facilitando a modificação da estrutura.
#
# mkdir -p: Cria os diretórios, incluindo os diretórios pais, se necessário.
# for loops: Iteram sobre as listas de arquivos e criam os arquivos, se não existirem.
# if [ ! -f "${FILENAME}" ]: Verifica se o arquivo não existe antes de criá-lo.
# echo e >: Usados para criar os arquivos e adicionar os comentários.
# cat <<EOF: Usado para criar o arquivo CMakeLists.txt com múltiplas linhas.
# .gitignore: Cria um arquivo .gitignore básico para ignorar arquivos de build e objetos.
# Comentários: Adiciona comentários em cada arquivo para descrever seu propósito.
# Este script facilita a configuração inicial do projeto, garantindo que todos os diretórios
# e arquivos necessários estejam presentes e organizados.
#
#################################################################################################





# Diretório raiz do projeto
PROJECT_DIR="optimal_order_selection"

# Diretórios a serem criados
SRC_DIR="${PROJECT_DIR}/src"
INCLUDE_DIR="${PROJECT_DIR}/include"
DATA_DIR="${PROJECT_DIR}/data"
INSTANCES_DIR="${DATA_DIR}/instances"
SOLUTIONS_DIR="${DATA_DIR}/solutions"
CONFIG_DIR="${PROJECT_DIR}/config"
SCRIPTS_DIR="${PROJECT_DIR}/scripts"
BUILD_DIR="${PROJECT_DIR}/build"
DOC_DIR="${PROJECT_DIR}/doc"
TEST_DIR="${PROJECT_DIR}/test"

# Arquivos a serem criados (com comentários)
SRC_FILES=(
  "${SRC_DIR}/main.cpp # Ponto de entrada do programa"
  "${SRC_DIR}/config_reader.cpp # Lógica para ler os arquivos de configuração"
  "${SRC_DIR}/config_reader.h # Header para config_reader.cpp"
  "${SRC_DIR}/data_structures.h # Definições das estruturas de dados (Pedido, Corredor, Instancia, Solucao)"
  "${SRC_DIR}/objective_function.cpp # Implementação da função objetivo"
  "${SRC_DIR}/objective_function.h # Header para objective_function.cpp"
  "${SRC_DIR}/constraints.cpp # Implementação das restrições"
  "${SRC_DIR}/constraints.h # Header para constraints.cpp"
  "${SRC_DIR}/algorithm.cpp # Implementação do algoritmo de otimização (Dinkelbach)"
  "${SRC_DIR}/algorithm.h # Header para algorithm.cpp"
  "${SRC_DIR}/solution.h # Definição da classe Solution"
  "${SRC_DIR}/validator.cpp # Implementação do validador de soluções"
  "${SRC_DIR}/validator.h # Header para validator.cpp"
)

CONFIG_FILES=(
  "${CONFIG_DIR}/problem_definition.txt # Definição do Problema"
  "${CONFIG_DIR}/objective_function.txt # Função Objetivo"
  "${CONFIG_DIR}/constraints.txt # Restrições"
  "${CONFIG_DIR}/algorithm_configuration.txt # Configuração do Algoritmo"
  "${CONFIG_DIR}/data_structures.txt # Estruturas de Dados"
  "${CONFIG_DIR}/variable_definitions.txt # Definição das Variáveis"
  "${CONFIG_DIR}/input_instance_format.txt # Formato do Arquivo de Instância"
  "${CONFIG_DIR}/output_solution_format.txt # Formato do Arquivo de Saída"
)

SCRIPT_FILES=(
  "${SCRIPTS_DIR}/validator.py # Script de validação"
)

# Cria o diretório raiz do projeto, se não existir
mkdir -p "${PROJECT_DIR}"

# Cria os diretórios, se não existirem
mkdir -p "${SRC_DIR}"
mkdir -p "${INCLUDE_DIR}"
mkdir -p "${DATA_DIR}"
mkdir -p "${INSTANCES_DIR}"
mkdir -p "${SOLUTIONS_DIR}"
mkdir -p "${CONFIG_DIR}"
mkdir -p "${SCRIPTS_DIR}"
mkdir -p "${BUILD_DIR}"
mkdir -p "${DOC_DIR}"
mkdir -p "${TEST_DIR}"

# Cria os arquivos de código fonte, se não existirem, e adiciona os comentários
for FILE in "${SRC_FILES[@]}"; do
  FILENAME=$(echo "$FILE" | cut -d' ' -f1)
  COMMENT=$(echo "$FILE" | cut -d' ' -f2-)
  if [ ! -f "${FILENAME}" ]; then
    echo "// ${COMMENT}" > "${FILENAME}"
    echo "Arquivo criado: ${FILENAME}"
  else
    echo "Arquivo já existe: ${FILENAME}"
  fi
done

# Cria os arquivos de configuração, se não existirem, e adiciona os comentários
for FILE in "${CONFIG_FILES[@]}"; do
  FILENAME=$(echo "$FILE" | cut -d' ' -f1)
  COMMENT=$(echo "$FILE" | cut -d' ' -f2-)
  if [ ! -f "${FILENAME}" ]; then
    echo "# ${COMMENT}" > "${FILENAME}"
    echo "Arquivo criado: ${FILENAME}"
  else
    echo "Arquivo já existe: ${FILENAME}"
  fi
done

# Cria os arquivos de script, se não existirem, e adiciona os comentários
for FILE in "${SCRIPT_FILES[@]}"; do
  FILENAME=$(echo "$FILE" | cut -d' ' -f1)
  COMMENT=$(echo "$FILE" | cut -d' ' -f2-)
  if [ ! -f "${FILENAME}" ]; then
    echo "#!/usr/bin/env python3" > "${FILENAME}"
    echo "# ${COMMENT}" >> "${FILENAME}"
    echo "Arquivo criado: ${FILENAME}"
  else
    echo "Arquivo já existe: ${FILENAME}"
  fi
done

# Cria o arquivo .gitignore, se não existir
if [ ! -f "${PROJECT_DIR}/.gitignore" ]; then
  echo "build/" > "${PROJECT_DIR}/.gitignore"
  echo "*.o" >> "${PROJECT_DIR}/.gitignore"
  echo "Arquivos .gitignore criado"
else
  echo "Arquivo .gitignore já existe"
fi

# Cria o arquivo CMakeLists.txt, se não existir
if [ ! -f "${PROJECT_DIR}/CMakeLists.txt" ]; then
  cat <<EOF > "${PROJECT_DIR}/CMakeLists.txt"
cmake_minimum_required(VERSION 3.10)
project(OptimalOrderSelection)

set(CMAKE_CXX_STANDARD 14)

# Include Boost library
find_package(Boost REQUIRED COMPONENTS system filesystem)
if(Boost_FOUND)
    include_directories(\${Boost_INCLUDE_DIRS})
    link_directories(\${Boost_LIBRARY_DIRS})
    add_definitions(\${Boost_DEFINITIONS})
endif()

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_executable(optimal_order_selection \${SOURCES})

if(Boost_FOUND)
    target_link_libraries(optimal_order_selection \${Boost_LIBRARIES})
endif()

# Install target
install(TARGETS optimal_order_selection DESTINATION bin)
install(DIRECTORY config/ DESTINATION config)
EOF
  echo "Arquivo CMakeLists.txt criado"
else
  echo "Arquivo CMakeLists.txt já existe"
fi

echo "Estrutura de diretórios criada com sucesso!"