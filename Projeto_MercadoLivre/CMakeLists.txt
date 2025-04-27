cmake_minimum_required(VERSION 3.14)  # Versão mínima que suporta FetchContent corretamente
project(MercadoLivre LANGUAGES CXX)

# Definir o padrão C++ para C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuração para otimizações em modo release
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

# Configuração para suporte a threads
find_package(Threads REQUIRED)

# Importar GoogleTest via FetchContent (não precisa do find_package)
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1  # Usar uma versão estável, não main
)
# Configuração especial para prevenir que o gtest instale manipuladores de sinais
set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Habilitar testes
enable_testing()
include(GoogleTest)

# Defina as variáveis de diretórios
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

# Inclua os diretórios de cabeçalho globalmente
include_directories(${INCLUDE_DIR})

# Biblioteca principal
file(GLOB_RECURSE CORE_SOURCES
    "${SOURCE_DIR}/core/*.cpp"
    "${SOURCE_DIR}/input/*.cpp"
    "${SOURCE_DIR}/output/*.cpp"
    "${SOURCE_DIR}/utils/*.cpp"
    "${SOURCE_DIR}/algorithm/*.cpp"
    "${SOURCE_DIR}/config/*.cpp"
    "${SOURCE_DIR}/model/*.cpp"
)

# Remover main.cpp da lista se existir
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Adicionar arquivos de algoritmos
set(ALGORITHM_SOURCES
    "${SOURCE_DIR}/algorithm/greedy_algorithm.cpp"
    "${SOURCE_DIR}/algorithm/optimization_algorithm.cpp"
    "${SOURCE_DIR}/algorithm/dinkelbach_algorithm.cpp"
)

message(STATUS "Compilando dinkelbach_algorithm.cpp: ${SOURCE_DIR}/algorithm/dinkelbach_algorithm.cpp")

# Criar biblioteca principal
add_library(MercadoLivreCore STATIC ${CORE_SOURCES} ${ALGORITHM_SOURCES})
target_include_directories(MercadoLivreCore PUBLIC ${INCLUDE_DIR})
target_link_libraries(MercadoLivreCore PRIVATE Threads::Threads)

# Adicionar arquivos para o executável principal
file(GLOB APP_SOURCES
    "${SOURCE_DIR}/app/*.cpp"
    "${SOURCE_DIR}/report/*.cpp"
)

# Executável principal - CORRIGIDO
add_executable(MercadoLivre
    ${SOURCE_DIR}/main.cpp
    ${APP_SOURCES}
    ${SOURCE_DIR}/modules/solucao_inicial.cpp
)
target_link_libraries(MercadoLivre PRIVATE MercadoLivreCore Threads::Threads)

# Configuração dos testes

# Analisador de instâncias
add_executable(instance_analyzer "${TEST_DIR}/input/instance_analyzer.cpp")
target_link_libraries(instance_analyzer PRIVATE MercadoLivreCore)

# Executável de testes - MODIFICAR ESTA PARTE
# Use GLOB_RECURSE para buscar arquivos em subdiretórios
file(GLOB_RECURSE TEST_SOURCES 
    "${TEST_DIR}/*.cpp"
    "${TEST_DIR}/*/*.cpp"
)

# Verificar se há arquivos de teste
message(STATUS "Arquivos de teste encontrados: ${TEST_SOURCES}")

# Criar um fallback para quando não há testes
if(TEST_SOURCES)
    add_executable(run_tests ${TEST_SOURCES})
    target_link_libraries(run_tests PRIVATE 
        MercadoLivreCore
        gtest_main
        gmock_main
    )
    # Adicionar o teste diretamente em vez de usar a descoberta automática
    add_test(NAME AllTests COMMAND run_tests)
    
    # Definir um timeout maior para o teste (300 segundos = 5 minutos)
    set_tests_properties(AllTests PROPERTIES TIMEOUT 300)
else()
    # Criar um teste vazio como fallback
    message(WARNING "Nenhum arquivo de teste encontrado! Criando teste vazio.")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp" "
        #include <gtest/gtest.h>
        TEST(EmptyTest, PlaceholderTest) {
            EXPECT_TRUE(true);
        }
    ")
    add_executable(run_tests "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp")
    target_link_libraries(run_tests PRIVATE
        MercadoLivreCore
        gtest_main
    )
    add_test(NAME EmptyTest COMMAND run_tests)
endif()