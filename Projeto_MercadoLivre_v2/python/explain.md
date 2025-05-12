
# Dockerfile para CPLEX com CUDA

```dockerfile

# Ubuntu 22.04, CUDA 12.1, Python 3.10.
FROM nvidia/cuda:12.1.0-devel-ubuntu22.04


ENV DEBIAN_FRONTEND=noninteractive

# dependências do sistema
RUN apt-get update && apt-get install -y \
    python3 python3-pip python3-dev build-essential \
    libssl-dev libffi-dev wget unzip git vim locales \
    && rm -rf /var/lib/apt/lists/*

# Localização PT-BR
RUN locale-gen pt_BR.UTF-8
ENV LANG=pt_BR.UTF-8
ENV LANGUAGE=pt_BR:pt
ENV LC_ALL=pt_BR.UTF-8

# Diretório de trabalho do projeto
WORKDIR /app

# Copiando os arquivos de requisitos primeiro (aproveita o cache do Docker)
COPY requirements.txt .

# Instalando dependências Python
RUN python3 -m pip install --upgrade pip && \
    pip3 install --no-cache-dir -r requirements.txt

# Copiando todo o projeto para dentro do container
COPY . .

# Variáveis para localizar o CPLEX dentro do container 
# Vai ter que instalar o CPLEX Hans...
# Lembre de mapear o volume na hora do docker run!
ENV CPLEX_STUDIO_DIR=/opt/ibm/ILOG/CPLEX_Studio2212

# (Opcional) No entrypoint/testes, adicione Pythonpath caso use API Python do CPLEX
ENV PYTHONPATH="${CPLEX_STUDIO_DIR}/cplex/python/3.10/x86-64_linux:$PYTHONPATH"

# Comando default
CMD ["python3", "main.py"]
       
```


## Criando o container:
### Comando para criar a imagem Docker
``` bash
# Criando a imagem
# O ponto final (.) indica o diretório atual
# O Dockerfile deve estar no mesmo diretório que o comando
# Se o Dockerfile estiver em outro diretório, use -f /caminho/para/o/Dockerfile

docker build -t my_cplex_cuda_app .
```

## Executando o container
### Sm GPU
```bash
sudo docker run -it --rm \
  --name dev-cplex \
  -v "/opt/ibm/ILOG/CPLEX_Studio2212:/opt/ibm/ILOG/CPLEX_Studio2212" \
  -v "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v3:/app" \
  my_cplex_cuda_app
```
### Com GPU
```bash
sudo docker run -it --rm \
  --device=nvidia.com/gpu=all \
  --name dev-cplex \
  -v "/opt/ibm/ILOG/CPLEX_Studio2212:/opt/ibm/ILOG/CPLEX_Studio2212" \
  -v "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v3:/app" \
  my_cplex_cuda_app
```

### Com GPU e Shell
```bash: 
  sudo docker run -it --rm \
  --device=nvidia.com/gpu=all \
  --name dev-cplex \
  -v "/opt/ibm/ILOG/CPLEX_Studio2212:/opt/ibm/ILOG/CPLEX_Studio2212" \
  -v "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre_v3:/app" \
  my_cplex_cuda_app \
  /bin/bash
```