#include <gtest/gtest.h>
#include "config/objective_function.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class ObjectiveFunctionTest : public ::testing::Test {
protected:
    ObjectiveFunction function;
    std::string testFilePath;
    
    void SetUp() override {
        // Criar um arquivo de teste temporário
        testFilePath = fs::temp_directory_path().string() + "/objective_test.txt";
        std::ofstream file(testFilePath);
        file << "# Função Objetivo para o Desafio SBPO 2025\n";
        file << "NOME: Produtividade de Coleta\n";
        file << "DESCRICAO: Maximizar a relação entre o número de itens coletados e o número de corredores visitados\n";
        file << "EXPRESSAO: max ∑(o∈O') ∑(i∈I(o)) u(oi) / |A'|\n";
        file << "TIPO: MAX\n";
        file.close();
    }
    
    void TearDown() override {
        // Remover arquivo temporário
        if (fs::exists(testFilePath)) {
            fs::remove(testFilePath);
        }
    }
};

TEST_F(ObjectiveFunctionTest, LoadsFromFile) {
    ASSERT_TRUE(function.loadFromFile(testFilePath));
    EXPECT_EQ(function.getName(), "Produtividade de Coleta");
    EXPECT_EQ(function.getDescription(), "Maximizar a relação entre o número de itens coletados e o número de corredores visitados");
    EXPECT_EQ(function.getExpression(), "max ∑(o∈O') ∑(i∈I(o)) u(oi) / |A'|");
    EXPECT_TRUE(function.isMaximize());
}

TEST_F(ObjectiveFunctionTest, CalculatesCorrectly) {
    // Aqui você pode criar uma solução mock e warehouse mock para testar o cálculo
    // Este teste pode ser expandido quando Solution estiver completamente implementado
    ASSERT_TRUE(function.loadFromFile(testFilePath));
    
    // Criar uma warehouse mock
    Warehouse warehouse;
    warehouse.numOrders = 1;
    warehouse.numItems = 1;
    warehouse.numCorridors = 1;
    
    // Adicionar um pedido com um item
    warehouse.orders.push_back({{0, 10}}); // Pedido 0 tem 10 unidades do item 0
    
    // Adicionar um corredor com um item
    warehouse.corridors.push_back({{0, 10}}); // Corredor 0 tem 10 unidades do item 0
    
    // Criar uma solução mock
    Solution solution;
    solution.addOrder(0, warehouse);
    
    // Calcular valor da função objetivo
    double value = function.evaluate(solution, warehouse);
    
    // A eficiência deve ser 10/1 = 10.0
    EXPECT_DOUBLE_EQ(value, 10.0);
}