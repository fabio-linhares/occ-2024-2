#!/usr/bin/env bash

# Compilar o validador com suporte para relatórios
g++ -std=c++17 validador_completo.cpp -o validador_completo -lstdc++fs

echo "Validador compilado com sucesso!"
echo "Para executar: ./validador_completo"
echo "Relatórios serão gerados em: relatorio_validacao.txt e relatorio_validacao.html"