# Compilação

g++ -std=c++17 -O3 -fopenmp -pthread main.cpp problema.cpp solucao.cpp algoritmos.cpp utils.cpp metricas.cpp controle.cpp -o batch_mz -lstdc++fs 2> erros