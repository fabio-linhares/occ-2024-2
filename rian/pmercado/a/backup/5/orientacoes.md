# Compilação

g++ -std=c++17 -O3 -fopenmp -pthread main.cpp problema.cpp solucao.cpp algoritmos.cpp utils.cpp metricas.cpp controle.cpp -o batch_mz -lstdc++fs 2> erros

Versão 4 troquei o grasp pelo ils

g++ -std=c++17 -O3 -fopenmp -pthread main.cpp problema.cpp solucao.cpp algoritmos.cpp utils.cpp metricas.cpp controle.cpp -o batch_m4 -lstdc++fs 2> erros 