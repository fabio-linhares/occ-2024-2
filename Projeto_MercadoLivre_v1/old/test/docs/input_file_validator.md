Explicação:
input_file_validator_test.cpp:
Inclui o Google Test para criar os testes.
Usa std::filesystem (C++17) para iterar sobre os arquivos no diretório de entrada.
Para cada arquivo .txt encontrado, cria uma nova thread para executar a função validateFile.
A função validateFile usa o InputParser (que você precisa implementar) para tentar ler o arquivo.
Se o parsing falhar (lançar uma exceção), o arquivo é considerado inválido.
Usa um std::mutex para proteger o acesso ao vetor allResults, garantindo que as threads não interfiram umas nas outras ao adicionar resultados.
No final, espera todas as threads terminarem e imprime um resumo dos resultados.
O teste falha se algum arquivo for considerado inválido.
test/CMakeLists.txt:
Configura o CMake para compilar e executar os testes.
Encontra o Google Test e o linka ao executável de teste.
Adiciona o diretório include do projeto para que os headers sejam encontrados.
Habilita os testes com enable_testing() e adiciona o teste com add_test().
CMakeLists.txt Principal:
A linha add_subdirectory(test) inclui o diretório de testes no processo de build.
Passos para Executar:
Instale o Google Test:

No NixOS, você pode instalar o Google Test com:
bash
Copiar
nix-env -iA nixpkgs.gtest
Implemente o InputParser:

Você precisa implementar a classe InputParser e sua função parseFile para ler os arquivos de entrada e verificar se o conteúdo está conforme as definições adequadas.
Certifique-se de que a função lance uma exceção se o arquivo for mal formatado.
Configure o CMake:

Na raiz do projeto, crie um diretório build:
bash
Copiar
mkdir build
cd build
Execute o CMake:
bash
Copiar
cmake ..
Compile e Execute os Testes:

bash
Copiar
make
make test
Pontos Importantes:
C++17: Este código usa std::filesystem, que requer C++17 ou superior. Certifique-se de que seu compilador esteja configurado para usar C++17.
Google Test: Certifique-se de que o Google Test esteja instalado e configurado corretamente.
InputParser: A implementação do InputParser é crucial. Ele deve ser capaz de ler os arquivos de entrada, verificar se o conteúdo está correto e lançar uma exceção se algo estiver errado.
Paralelismo: O uso de threads permite que vários arquivos sejam validados simultaneamente, acelerando o processo.
Mutex: O mutex garante que o acesso aos resultados seja thread-safe.
Com esta estrutura e código, você terá um teste que verifica todos os arquivos de entrada, usando paralelismo para acelerar o processo e informando quais arquivos estão corretos e quais estão mal formatados.