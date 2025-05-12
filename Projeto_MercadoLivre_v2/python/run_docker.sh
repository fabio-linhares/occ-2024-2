#!/usr/bin/env bash

CPLEX_HOST_PATH="/opt/ibm/ILOG/CPLEX_Studio2212/"
CONTAINER_NAME="wop_solver"

if [ ! -d "$CPLEX_HOST_PATH" ]; then
    echo "ERRO: CPLEX não encontrado em $CPLEX_HOST_PATH"
    exit 1
fi

if [ ! -d "$(pwd)/datasets" ]; then
    mkdir -p "$(pwd)/datasets"
fi
if [ ! -d "$(pwd)/results" ]; then
    mkdir -p "$(pwd)/results"
fi

echo "Construindo imagem Docker..."
docker build -t "$CONTAINER_NAME" .

echo "Executando o container..."
sudo docker run --gpus all -it --rm \
    --name "$CONTAINER_NAME" \
    -v "$CPLEX_HOST_PATH:/cplex_volume" \
    -v "$(pwd)/datasets:/app/datasets" \
    -v "$(pwd)/results:/app/results" \
    "$CONTAINER_NAME"