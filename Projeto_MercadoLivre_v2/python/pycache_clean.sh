#!/usr/bin/env bash

# Encontrar todos os diretórios cujo nome contém 'pycache'
mapfile -t CACHE_DIRS < <(find . -type d \( -iname "__pycache__" -o -iname "*pycache*" \))

# Se não encontrar nenhum, encerra
if [ ${#CACHE_DIRS[@]} -eq 0 ]; then
    echo "Nenhum diretório de cache encontrado."
    exit 0
fi

# Lista os diretórios encontrados
echo "Diretórios de cache encontrados:"
for dir in "${CACHE_DIRS[@]}"; do
    echo "  $dir"
done

# Pergunta confirmação ao usuário
read -r -p "Deseja removê-los? [y/N] " CONFIRM
case "$CONFIRM" in
    [yY][eE][sS]|[yY])
        # Remove cada diretório
        for dir in "${CACHE_DIRS[@]}"; do
            rm -rf "$dir"
            echo "Removido: $dir"
        done
        echo "Todos os diretórios de cache foram removidos."
        ;;
    *)
        echo "Operação abortada. Nenhum diretório foi removido."
        ;;
esac
