# Escolha a imagem CUDA adequada. Aqui, Ubuntu 22.04, CUDA 12.1, Python 3.10.
FROM nvidia/cuda:12.1.0-devel-ubuntu22.04


ENV DEBIAN_FRONTEND=noninteractive

# Instale dependências do sistema
RUN apt-get update && apt-get install -y \
    python3 python3-pip python3-dev build-essential \
    libssl-dev libffi-dev wget unzip git vim locales \
    && rm -rf /var/lib/apt/lists/*

# Localização PT-BR (opcional)
RUN locale-gen pt_BR.UTF-8
ENV LANG=pt_BR.UTF-8
ENV LANGUAGE=pt_BR:pt
ENV LC_ALL=pt_BR.UTF-8

# Diretório de trabalho do projeto
WORKDIR /app

# Copie os arquivos de requisitos primeiro (aproveita o cache do Docker)
COPY requirements.txt .

# Instale dependências Python
RUN python3 -m pip install --upgrade pip && \
    pip3 install --no-cache-dir -r requirements.txt

# Copie todo o projeto para dentro do container
COPY . .

# Variáveis para localizar o CPLEX dentro do container (mapeie o volume na hora do docker run!)
ENV CPLEX_STUDIO_DIR=/opt/ibm/ILOG/CPLEX_Studio2212

# (Opcional) No entrypoint/testes, adicione Pythonpath caso use API Python do CPLEX
ENV PYTHONPATH="${CPLEX_STUDIO_DIR}/cplex/python/3.10/x86-64_linux:$PYTHONPATH"

# Comando default
CMD ["python3", "main.py"]
