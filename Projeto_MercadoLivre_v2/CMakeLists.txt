cmake_minimum_required(VERSION 3.14)
project(MercadoLivre_v2 LANGUAGES CXX)

# Definir padrão C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Otimização para Release
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra -Wpedantic")

# Suporte a threads
find_package(Threads REQUIRED)

# Definir flag para usar solver personalizado
add_definitions(-DUSE_CUSTOM_PLI_SOLVER)

# GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Habilitar testes
enable_testing()
include(GoogleTest)

# Diretórios principais
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)

# Incluir headers globalmente
include_directories(${INCLUDE_DIR})

# Coletar todos os .cpp em src/
file(GLOB_RECURSE ALL_SOURCE_FILES "${SOURCE_DIR}/*.cpp")

# Separar main.cpp
set(MAIN_FILE "${SOURCE_DIR}/main.cpp")

# Verificar se o arquivo main.cpp existe
if(NOT EXISTS ${MAIN_FILE})
    message(STATUS "main.cpp não encontrado, será criado um arquivo placeholder")
    file(WRITE ${MAIN_FILE} "#include <iostream>\n#include \"menu.h\"\n\nint main() {\n    std::cout << \"Projeto MercadoLivre v2 - SBPO 2025\\n\";\n    int choice = 0;\n    do {\n        mostrarMenu();\n        std::cout << \"Digite sua escolha: \";\n        std::cin >> choice;\n        processarEscolhaMenu(choice);\n    } while (choice != 0);\n    return 0;\n}\n")
endif()

# Criar diretórios de dados se não existirem
if(NOT EXISTS ${DATA_DIR})
    file(MAKE_DIRECTORY ${DATA_DIR})
    file(MAKE_DIRECTORY "${DATA_DIR}/input")
    file(MAKE_DIRECTORY "${DATA_DIR}/output")
endif()

# Separar fontes da biblioteca (tudo menos main.cpp)
set(CORE_SOURCES ${ALL_SOURCE_FILES})
list(REMOVE_ITEM CORE_SOURCES ${MAIN_FILE})

# Verificar se existem arquivos fonte
if("${CORE_SOURCES}" STREQUAL "")
    message(WARNING "Nenhum arquivo fonte encontrado em ${SOURCE_DIR}!")
endif()

# Criar biblioteca principal
add_library(MercadoLivre_v2Core STATIC ${CORE_SOURCES})
target_include_directories(MercadoLivre_v2Core PUBLIC ${INCLUDE_DIR})
target_link_libraries(MercadoLivre_v2Core PRIVATE Threads::Threads)

# Criar executável principal
add_executable(MercadoLivre_v2 ${MAIN_FILE})
target_link_libraries(MercadoLivre_v2 PRIVATE MercadoLivre_v2Core Threads::Threads)

# --- TESTES ---

# Verificar se o diretório de testes existe
if(EXISTS ${TEST_DIR})
    # Buscar todos os testes
    file(GLOB_RECURSE TEST_SOURCES 
        "${TEST_DIR}/*.cpp"
        "${TEST_DIR}/*/*.cpp"
    )

    message(STATUS "Arquivos de teste encontrados: ${TEST_SOURCES}")

    if(TEST_SOURCES)
        add_executable(run_tests ${TEST_SOURCES})
        target_link_libraries(run_tests PRIVATE 
            MercadoLivre_v2Core
            gtest_main
            gmock_main
        )
        add_test(NAME AllTests COMMAND run_tests)
        set_tests_properties(AllTests PROPERTIES TIMEOUT 300)
    else()
        message(WARNING "Nenhum arquivo de teste encontrado! Criando teste vazio.")
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp" "
            #include <gtest/gtest.h>
            TEST(EmptyTest, PlaceholderTest) {
                EXPECT_TRUE(true);
            }
        ")
        add_executable(run_tests "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp")
        target_link_libraries(run_tests PRIVATE
            MercadoLivre_v2Core
            gtest_main
        )
        add_test(NAME EmptyTest COMMAND run_tests)
    endif()
else()
    message(STATUS "Diretório de testes não encontrado. Criando diretório...")
    file(MAKE_DIRECTORY ${TEST_DIR})
    message(WARNING "Nenhum arquivo de teste encontrado! Criando teste vazio.")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp" "
        #include <gtest/gtest.h>
        TEST(EmptyTest, PlaceholderTest) {
            EXPECT_TRUE(true);
        }
    ")
    add_executable(run_tests "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp")
    target_link_libraries(run_tests PRIVATE
        MercadoLivre_v2Core
        gtest_main
    )
    add_test(NAME EmptyTest COMMAND run_tests)
endif()


