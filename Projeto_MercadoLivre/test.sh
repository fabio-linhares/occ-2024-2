#!/usr/bin/env bash

######################################################################
#
# Script para compilar o projeto Mercado Livre
# Autor: Fábio Linhares
# Data: 2024-10-01
# Descrição: Este script compila o projeto Mercado Livre utilizando
# o CMake e o Nix. Ele remove o diretório build, cria um novo diretório
# build, e executa o CMake dentro do diretório build. O script também
# instala o Google Test e o pkg-config, que são necessários para a
# compilação do projeto.
# O script deve ser executado a partir do diretório raiz do projeto.
# O script é compatível com NixOS e outras distribuições Linux.
# Uso: ./test.sh
# Dependências: cmake, gtest, pkg-config, gcc
# Licença: MIT
# Versão: 1.0
#
# Uso:
#   ./test.sh
#   ctest . --rerun-failed --output-on-failure
#   ./test/instance_analyzer
#################################################################
# Vai para o diretório raiz do projeto
cd ~/Projetos/occ-2024-2/Projeto_MercadoLivre

# Remove o diretório build (se existir)
rm -rf build

# Cria o diretório build
mkdir build

# Entra no diretório build
cd build

# Executa o CMake com o nix-shell corrigido
# No NixOS, o pacote se chama 'gtest' e não 'googletest'
nix-shell -p cmake gtest pkg-config gcc --run "
  cmake .. 
  make
"