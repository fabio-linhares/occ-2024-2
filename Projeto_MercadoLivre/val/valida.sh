#!/usr/bin/env bash

# Compilar o validador
g++ -std=c++17 validador_completo.cpp -o validador_completo -lstdc++fs

echo "Validador compilado com sucesso!"
echo "Para executar: ./validador_completo"