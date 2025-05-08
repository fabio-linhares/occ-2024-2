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

# Suporte ao OR-Tools - apenas a parte de detecção
option(USE_ORTOOLS "Usar OR-Tools para PLI" OFF)

# Detecção do OR-Tools
set(ORTOOLS_DETECTED FALSE)
if(USE_ORTOOLS)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ORTOOLS ortools)
    endif()
    
    if(NOT ORTOOLS_FOUND)
        # Procurar manualmente (necessário para alguns sistemas)
        find_path(ORTOOLS_INCLUDE_DIR ortools/linear_solver/linear_solver.h
            PATHS /nix/store/*/include /usr/include /usr/local/include)

        find_library(ORTOOLS_LIBRARY ortools
            PATHS /nix/store/*/lib /usr/lib /usr/local/lib)
    endif()
    
    if(ORTOOLS_FOUND OR (ORTOOLS_INCLUDE_DIR AND ORTOOLS_LIBRARY))
        set(ORTOOLS_DETECTED TRUE)
        add_definitions(-DUSE_ORTOOLS)
        message(STATUS "OR-Tools encontrado: ${ORTOOLS_INCLUDE_DIR}")
    else()
        message(STATUS "OR-Tools não encontrado. Usando método heurístico.")
    endif()
endif()

# Detecção manual do GoogleTest para NixOS
set(GTEST_FOUND FALSE)
# Procurar diretamente no NixOS (similar ao que fizemos com o OR-Tools)
find_path(GTEST_INCLUDE_DIR gtest/gtest.h
    PATHS /nix/store/*/include /usr/include /usr/local/include)

find_library(GTEST_LIBRARY gtest
    PATHS /nix/store/*/lib /usr/lib /usr/local/lib)

find_library(GTEST_MAIN_LIBRARY gtest_main
    PATHS /nix/store/*/lib /usr/lib /usr/local/lib)

if(GTEST_INCLUDE_DIR AND GTEST_LIBRARY AND GTEST_MAIN_LIBRARY)
    message(STATUS "GoogleTest encontrado manualmente no NixOS")
    set(GTEST_FOUND TRUE)
    set(GTEST_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
    set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
else()
    message(WARNING "GoogleTest não encontrado manualmente. Tentando find_package...")
    
    # Tentar com find_package como backup
    find_package(GTest QUIET)
    if(GTest_FOUND)
        set(GTEST_LIBRARIES GTest::GTest GTest::Main)
    else()
        set(GTEST_INCLUDE_DIRS "")
    endif()
endif()

# Habilitar testes
enable_testing()

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
    # Re-run glob to include the newly created main.cpp
    file(GLOB_RECURSE ALL_SOURCE_FILES "${SOURCE_DIR}/*.cpp")
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
    # Criar um arquivo fonte mínimo para evitar bibliotecas vazias
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_lib.cpp" "// Auto-generated empty file\n")
    set(CORE_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/empty_lib.cpp")
endif()

# Definir a biblioteca principal
add_library(MercadoLivre_v2Core STATIC ${CORE_SOURCES})

# Configurar a biblioteca
target_include_directories(MercadoLivre_v2Core PUBLIC ${INCLUDE_DIR})
target_link_libraries(MercadoLivre_v2Core PRIVATE Threads::Threads)

# Configurar OR-Tools para a biblioteca (somente se detectado)
if(ORTOOLS_DETECTED)
    if(ORTOOLS_FOUND)
        target_include_directories(MercadoLivre_v2Core PRIVATE ${ORTOOLS_INCLUDE_DIRS})
        target_link_libraries(MercadoLivre_v2Core PRIVATE ${ORTOOLS_LIBRARIES})
    else()
        target_include_directories(MercadoLivre_v2Core PRIVATE ${ORTOOLS_INCLUDE_DIR})
        target_link_libraries(MercadoLivre_v2Core PRIVATE ${ORTOOLS_LIBRARY})
    endif()
endif()

# Criar executável principal
add_executable(MercadoLivre_v2 ${MAIN_FILE})
target_link_libraries(MercadoLivre_v2 PRIVATE MercadoLivre_v2Core Threads::Threads)

# --- TESTES ---
if(EXISTS ${TEST_DIR})
    file(GLOB_RECURSE TEST_SOURCES "${TEST_DIR}/*.cpp" "${TEST_DIR}/*/*.cpp")
    message(STATUS "Arquivos de teste encontrados: ${TEST_SOURCES}")

    if(TEST_SOURCES)
        # Adicionar executável de testes
        add_executable(run_tests ${TEST_SOURCES})
        target_include_directories(run_tests PRIVATE ${INCLUDE_DIR})
        
        # Configurar GTest
        if(GTEST_INCLUDE_DIRS)
            target_include_directories(run_tests PRIVATE ${GTEST_INCLUDE_DIRS})
        endif()
        
        # Adicionar bibliotecas de teste
        target_link_libraries(run_tests PRIVATE 
            MercadoLivre_v2Core
            ${GTEST_LIBRARIES}
            Threads::Threads
        )
        
        # Adicionar o teste
        add_test(NAME AllTests COMMAND run_tests)
        set_tests_properties(AllTests PROPERTIES TIMEOUT 300)
    else()
        # Nenhum arquivo de teste encontrado
        message(WARNING "Nenhum teste encontrado em ${TEST_DIR}")
        # Criar um teste vazio para evitar erros
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp" 
            "#include <gtest/gtest.h>\nTEST(EmptyTest, Success) { EXPECT_TRUE(true); }")
        
        add_executable(run_tests "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp")
        
        target_include_directories(run_tests PRIVATE ${INCLUDE_DIR})
        
        if(GTEST_INCLUDE_DIRS)
            target_include_directories(run_tests PRIVATE ${GTEST_INCLUDE_DIRS})
        endif()
        
        target_link_libraries(run_tests PRIVATE 
            MercadoLivre_v2Core
            ${GTEST_LIBRARIES}
            Threads::Threads
        )
        
        add_test(NAME EmptyTest COMMAND run_tests)
    endif()
else()
    message(STATUS "Diretório de testes não encontrado. Criando diretório...")
    file(MAKE_DIRECTORY ${TEST_DIR})
    message(WARNING "Nenhum arquivo de teste encontrado! Criando teste vazio.")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp" 
        "#include <gtest/gtest.h>\nTEST(EmptyTest, Success) { EXPECT_TRUE(true); }")
    
    add_executable(run_tests "${CMAKE_CURRENT_BINARY_DIR}/empty_test.cpp")
    
    target_include_directories(run_tests PRIVATE ${INCLUDE_DIR})
    
    if(GTEST_INCLUDE_DIRS)
        target_include_directories(run_tests PRIVATE ${GTEST_INCLUDE_DIRS})
    endif()
    
    target_link_libraries(run_tests PRIVATE 
        MercadoLivre_v2Core
        ${GTEST_LIBRARIES}
        Threads::Threads
    )
    
    add_test(NAME EmptyTest COMMAND run_tests)
endif()


