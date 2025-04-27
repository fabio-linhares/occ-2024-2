cmake_minimum_required(VERSION 3.14)
project(MercadoLivre LANGUAGES CXX)

# Definir padrão C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Otimização para Release
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

# Suporte a threads
find_package(Threads REQUIRED)

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

# Incluir headers globalmente
include_directories(${INCLUDE_DIR})

# Coletar todos os .cpp em src/
file(GLOB_RECURSE ALL_SOURCE_FILES "${SOURCE_DIR}/*.cpp")

# Separar main.cpp
set(MAIN_FILE "${SOURCE_DIR}/main.cpp")

# Separar fontes da biblioteca (tudo menos main.cpp)
set(CORE_SOURCES ${ALL_SOURCE_FILES})
list(REMOVE_ITEM CORE_SOURCES ${MAIN_FILE})

# Criar biblioteca principal
add_library(MercadoLivreCore STATIC ${CORE_SOURCES})
target_include_directories(MercadoLivreCore PUBLIC ${INCLUDE_DIR})
target_link_libraries(MercadoLivreCore PRIVATE Threads::Threads)

# Criar executável principal
add_executable(MercadoLivre ${MAIN_FILE})
target_link_libraries(MercadoLivre PRIVATE MercadoLivreCore Threads::Threads)

# --- TESTES ---

# Buscar todos os testes
file(GLOB_RECURSE TEST_SOURCES 
    "${TEST_DIR}/*.cpp"
    "${TEST_DIR}/*/*.cpp"
)

message(STATUS "Arquivos de teste encontrados: ${TEST_SOURCES}")

if(TEST_SOURCES)
    add_executable(run_tests ${TEST_SOURCES})
    target_link_libraries(run_tests PRIVATE 
        MercadoLivreCore
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
        MercadoLivreCore
        gtest_main
    )
    add_test(NAME EmptyTest COMMAND run_tests)
endif()
