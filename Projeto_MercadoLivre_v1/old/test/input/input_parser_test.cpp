#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <filesystem>
#include "input/input_parser.h"

namespace fs = std::filesystem;

// Fixture para testes do InputParser
class InputParserTest : public ::testing::Test {
protected:
    InputParser parser;
    std::string testDirPath;

    void SetUp() override {
        // Criar diretório temporário para os testes
        testDirPath = fs::temp_directory_path().string() + "/parser_test";
        if (fs::exists(testDirPath)) {
            fs::remove_all(testDirPath);
        }
        fs::create_directory(testDirPath);
    }

    void TearDown() override {
        // Limpar diretório temporário após os testes
        if (fs::exists(testDirPath)) {
            fs::remove_all(testDirPath);
        }
    }

    // Helper para criar arquivo de teste
    std::string createTestFile(const std::string& content) {
        std::string filePath = testDirPath + "/test_file.txt";
        std::ofstream file(filePath);
        file << content;
        file.close();
        return filePath;
    }
};

// Teste para arquivo válido
TEST_F(InputParserTest, ParsesValidFileCorrectly) {
    // Criar um arquivo temporário para teste
    std::ofstream file("temp_test_input.txt");
    file << "3 5 2\n";                  // numOrders, numItems, numCorridors
    file << "2 0 1 3 2\n";              // Pedido 0: 2 itens (item 0: qtd 1, item 3: qtd 2)
    file << "1 2 3\n";                  // Pedido 1: 1 item (item 2: qtd 3)
    file << "3 1 1 2 1 4 1\n";          // Pedido 2: 3 itens (vários)
    file << "2 0 5 3 10\n";             // Corredor 0: 2 itens
    file << "3 1 8 2 12 4 7\n";         // Corredor 1: 3 itens
    file << "10 20\n";                  // LB, UB
    file.close();

    // Parse do arquivo
    Warehouse warehouse = parser.parseFile("temp_test_input.txt");

    // Verificações básicas
    EXPECT_EQ(warehouse.numOrders, 3);
    EXPECT_EQ(warehouse.numItems, 5);
    EXPECT_EQ(warehouse.numCorridors, 2);

    // Verificar pedido 0
    EXPECT_EQ(warehouse.orders[0].size(), 2);  // 2 itens no pedido 0
    EXPECT_EQ(warehouse.orders[0][0], 1);      // Item 0 tem quantidade 1
    EXPECT_EQ(warehouse.orders[0][3], 2);      // Item 3 tem quantidade 2

    // Verificar pedido 1
    EXPECT_EQ(warehouse.orders[1].size(), 1);  // 1 item no pedido 1
    EXPECT_EQ(warehouse.orders[1][2], 3);      // Item 2 tem quantidade 3

    // Verificar pedido 2
    EXPECT_EQ(warehouse.orders[2].size(), 3);  // 3 itens no pedido 2
    EXPECT_EQ(warehouse.orders[2][1], 1);      // Item 1 tem quantidade 1
    EXPECT_EQ(warehouse.orders[2][2], 1);      // Item 2 tem quantidade 1
    EXPECT_EQ(warehouse.orders[2][4], 1);      // Item 4 tem quantidade 1

    // Verificar corredor 0
    EXPECT_EQ(warehouse.corridors[0].size(), 2);  // 2 itens no corredor 0
    EXPECT_EQ(warehouse.corridors[0][0], 5);      // Item 0 tem quantidade 5
    EXPECT_EQ(warehouse.corridors[0][3], 10);     // Item 3 tem quantidade 10

    // Verificar corredor 1
    EXPECT_EQ(warehouse.corridors[1].size(), 3);  // 3 itens no corredor 1
    EXPECT_EQ(warehouse.corridors[1][1], 8);      // Item 1 tem quantidade 8
    EXPECT_EQ(warehouse.corridors[1][2], 12);     // Item 2 tem quantidade 12
    EXPECT_EQ(warehouse.corridors[1][4], 7);      // Item 4 tem quantidade 7

    // Verificar LB e UB
    EXPECT_EQ(warehouse.LB, 10);
    EXPECT_EQ(warehouse.UB, 20);

    // Limpar
    std::remove("temp_test_input.txt");
}

// Teste para arquivo inválido
TEST_F(InputParserTest, ThrowsExceptionForInvalidFile) {
    EXPECT_THROW(parser.parseFile("non_existent_file.txt"), std::runtime_error);
}

// Teste para verificar se o parser lida corretamente com LB/UB ausentes
TEST_F(InputParserTest, HandlesDefaultLBUB) {
    std::string content = 
        "2 3 2\n"                 // 2 pedidos, 3 itens, 2 corredores
        "1 0 1\n"                 // Pedido 1: 1 item - (0,1) 
        "1 1 2\n"                 // Pedido 2: 1 item - (1,2)
        "1 0 5\n"                 // Corredor 1: 1 item - (0,5)
        "1 1 8\n";                // Corredor 2: 1 item - (1,8)
                                  // LB/UB ausentes

    std::string filePath = createTestFile(content);
    Warehouse warehouse = parser.parseFile(filePath);
    
    // Verificar valores padrão para LB/UB
    EXPECT_EQ(warehouse.LB, 1);
    EXPECT_EQ(warehouse.UB, 2); // numCorridors
}

// Localize o teste HandlesInvalidLBUB
TEST_F(InputParserTest, HandlesInvalidLBUB) {
    // Cria um arquivo com LB=3 e UB=1 (inválido, pois LB > UB)
    std::string content = "3 3 3\n1 0 1\n1 1 1\n1 2 1\n1 0 1\n1 1 1\n1 2 1\n3 1\n";
    std::string filePath = createTestFile(content);
    
    // Agora esperamos que o parser lance uma exceção ao encontrar LB > UB
    EXPECT_THROW(parser.parseFile(filePath), std::runtime_error);
    
    fs::remove(filePath);
}

// Teste para verificar se o parser detecta items inválidos
TEST_F(InputParserTest, DetectsInvalidItems) {
    std::string content = 
        "1 3 1\n"                 // 1 pedido, 3 itens, 1 corredor
        "1 5 1\n"                 // Pedido 1: item com índice 5 (inválido, pois numItems = 3)
        "1 0 5\n";                // Corredor 1: 1 item - (0,5)

    std::string filePath = createTestFile(content);
    EXPECT_THROW(parser.parseFile(filePath), std::runtime_error);
}

// Teste para verificar se o parser detecta quantidades inválidas
TEST_F(InputParserTest, DetectsInvalidQuantities) {
    std::string content = 
        "1 3 1\n"                 // 1 pedido, 3 itens, 1 corredor
        "1 0 0\n"                 // Pedido 1: item com quantidade 0 (inválido)
        "1 0 5\n";                // Corredor 1: 1 item - (0,5)

    std::string filePath = createTestFile(content);
    EXPECT_THROW(parser.parseFile(filePath), std::runtime_error);
}

// Teste para verificar se o parser lida corretamente com arquivo inexistente
TEST_F(InputParserTest, HandlesNonExistentFile) {
    EXPECT_THROW(parser.parseFile("/path/to/nonexistent/file.txt"), std::runtime_error);
}

// Teste para verificar se o parser lida corretamente com formato incorreto 
TEST_F(InputParserTest, HandlesIncorrectFormat) {
    // Usar uma string que tenha o início do formato correto, mas depois falhe
    std::string content = "3 5 2\nInvalid content here";
    std::string filePath = createTestFile(content);
    EXPECT_THROW(parser.parseFile(filePath), std::runtime_error);
}

// Teste com instância real (usando os arquivos existentes)
TEST_F(InputParserTest, ParsesRealInstance) {
    std::string filePath = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/input/instance_0001.txt";
    if (fs::exists(filePath)) {
        Warehouse warehouse = parser.parseFile(filePath);
        
        // Verificar valores conhecidos da instância 1
        EXPECT_EQ(warehouse.numOrders, 61);
        EXPECT_EQ(warehouse.numItems, 155);
        EXPECT_EQ(warehouse.numCorridors, 116);
        EXPECT_EQ(warehouse.LB, 30);
        EXPECT_EQ(warehouse.UB, 68);
    } else {
        GTEST_SKIP() << "Arquivo de instância real não encontrado";
    }
}