Considerando as boas práticas de programação em C++, Clean Code e modularização, uma estrutura de diretórios sugerida para o seu projeto de otimização seria:

ProjectRoot/
├── CMakeLists.txt          # Arquivo de configuração do CMake
├── .gitignore              # Arquivo para ignorar arquivos no Git
├── README.md               # Arquivo de documentação do projeto
├── data/                  # Diretório para os arquivos de entrada
│   ├── instance1.txt       # Exemplo de arquivo de entrada
│   ├── instance2.txt
│   └── ...
├── include/               # Diretório para os arquivos de cabeçalho (.h)
│   ├── core/              # Componentes principais do problema
│   │   ├── item.h         # Definição da classe Item
│   │   ├── order.h        # Definição da classe Order
│   │   ├── corridor.h     # Definição da classe Corridor
│   │   ├── warehouse.h    # Definição da classe Warehouse (contém pedidos, itens e corredores)
│   │   └── solution.h     # Definição da classe Solution
│   ├── input/             # Lógica de leitura dos arquivos de entrada
│   │   └── input_parser.h # Classe para fazer o parsing dos arquivos de entrada
│   ├── output/            # Lógica de escrita dos arquivos de saída
│   │   └── output_writer.h # Classe para escrever os arquivos de saída
│   ├── algorithm/        # Algoritmos de otimização
│   │   ├── optimization_algorithm.h # Interface para os algoritmos de otimização
│   │   └── greedy_algorithm.h     # Implementação de um algoritmo guloso
│   ├── utils/             # Utilitários gerais
│   │   └── logger.h       # Sistema de logging
│   └── config.h           # Arquivo de configuração geral
├── src/                   # Diretório para os arquivos de código fonte (.cpp)
│   ├── core/
│   │   ├── item.cpp
│   │   ├── order.cpp
│   │   ├── corridor.cpp
│   │   ├── warehouse.cpp
│   │   └── solution.cpp
│   ├── input/
│   │   └── input_parser.cpp
│   ├── output/
│   │   └── output_writer.cpp
│   ├── algorithm/
│   │   └── optimization_algorithm.cpp
│   │   └── greedy_algorithm.cpp
│   ├── utils/
│   │   └── logger.cpp
│   └── main.cpp            # Ponto de entrada do programa
├── test/                  # Diretório para os testes unitários
│   ├── core/
│   │   ├── item_test.cpp
│   │   ├── order_test.cpp
│   │   └── ...
│   ├── input/
│   │   └── input_parser_test.cpp
│   ├── utils/
│   │   └── logger_test.cpp
│   └── CMakeLists.txt      # Arquivo CMake para os testes
└── docs/                  # Documentação do projeto (opcional)
Explicação da Estrutura:
ProjectRoot/:
Raiz do projeto.
CMakeLists.txt: Utilizado para configurar o build do projeto com CMake. Facilita a compilação e gerenciamento de dependências.
.gitignore: Especifica arquivos que o Git deve ignorar (arquivos de build, executáveis, etc.).
README.md: Documentação básica do projeto, incluindo instruções de uso, descrição e informações de contato.
data/:
Armazena os arquivos de entrada com as instâncias do problema.
Cada arquivo representa uma instância diferente (ex: instance1.txt, instance2.txt).
include/:
Contém todos os arquivos de cabeçalho (.h).
core/:
Define as classes principais do problema: Item, Order, Corridor, Warehouse, e Solution.
item.h: Definição da classe Item (produto solicitado).
order.h: Definição da classe Order (lista de itens solicitados).
corridor.h: Definição da classe Corridor (local de armazenamento dos itens).
warehouse.h: Definição da classe Warehouse (contém a estrutura completa do armazém: pedidos, itens e corredores).
solution.h: Definição da classe Solution (representa uma solução viável para o problema).
input/:
Lógica para ler os arquivos de entrada.
input_parser.h: Classe responsável por fazer o parsing dos arquivos de entrada e criar os objetos correspondentes.
output/:
Lógica para escrever os arquivos de saída.
output_writer.h: Classe responsável por formatar e escrever os resultados da solução em um arquivo de saída.
algorithm/:
Implementações dos algoritmos de otimização.
optimization_algorithm.h: Interface base para diferentes algoritmos de otimização.
greedy_algorithm.h: Implementação de um algoritmo guloso como exemplo.
utils/:
Utilitários gerais do projeto.
logger.h: Sistema de logging para facilitar o debugging e monitoramento do programa.
config.h:
Arquivo de configuração geral do projeto, com definições de constantes e flags.
src/:
Contém os arquivos de código fonte (.cpp) correspondentes aos arquivos de cabeçalho em include/.
A estrutura de subdiretórios espelha a estrutura em include/.
main.cpp: Ponto de entrada do programa, onde a execução começa.
test/:
Contém os testes unitários para garantir a qualidade do código.
Usa um framework de testes como Google Test ou Catch2.
CMakeLists.txt: Arquivo CMake para configurar e compilar os testes.
docs/:
Diretório opcional para armazenar a documentação detalhada do projeto (pode ser gerada com Doxygen).
Boas Práticas Aplicadas:
Modularização:
O código é dividido em módulos lógicos (core, input, output, algorithm, utils), facilitando a manutenção e reutilização.
Clean Code:
Nomes descritivos para classes, variáveis e funções.
Funções pequenas e focadas em uma única responsabilidade.
Comentários explicativos quando necessário.
Separação de Interface e Implementação:
Arquivos de cabeçalho (.h) definem as interfaces, enquanto os arquivos de código fonte (.cpp) contêm as implementações.
Gerenciamento de Dependências:
Uso do CMake para facilitar a compilação e o gerenciamento de dependências.
Testes Unitários:
Criação de testes unitários para garantir a qualidade e a correção do código.
Documentação:
Uso de um arquivo README.md para documentação básica e um diretório docs/ para documentação mais detalhada.