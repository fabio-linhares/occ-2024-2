#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>  // C++17
#include "input/input_parser.h" // Inclua o header do seu parser

namespace fs = std::filesystem;

// Estrutura para armazenar os resultados dos testes
struct TestResult {
    std::string filename;
    bool isValid;
    std::string error;
};

// Mutex para proteger o acesso aos resultados
std::mutex resultsMutex;
std::vector<TestResult> allResults;

// Função para validar um único arquivo
void validateFile(const std::string& filePath) {
    TestResult result;
    result.filename = filePath;
    result.isValid = true;
    result.error = "";

    try {
        // Use seu InputParser para tentar ler o arquivo
        InputParser parser;
        Warehouse warehouse = parser.parseFile(filePath);

        // Adicione aqui outras verificações de sanidade, se necessário
        // Ex: Verificar se o número de pedidos, itens e corredores é consistente
        // Ex: Verificar se os limites LB e UB são válidos

    } catch (const std::exception& e) {
        result.isValid = false;
        result.error = e.what();
    } catch (...) {
        result.isValid = false;
        result.error = "Unknown exception";
    }

    // Protege o acesso aos resultados e adiciona o resultado
    std::lock_guard<std::mutex> lock(resultsMutex);
    allResults.push_back(result);
}

TEST(InputFileValidator, ValidateAllFiles) {
    // Caminho para o diretório de entrada
    std::string inputDirectory = "/home/zerocopia/Projetos/occ-2024-2/Projeto_MercadoLivre/data/input/";

    //std::string inputDirectory = "../../data/input";

    // Vetor para armazenar as threads
    std::vector<std::thread> threads;

    // Itera sobre todos os arquivos no diretório
    for (const auto& entry : fs::directory_iterator(inputDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            // Cria uma thread para validar o arquivo
            threads.emplace_back(validateFile, entry.path().string());
        }
    }

    // Espera todas as threads terminarem
    for (auto& thread : threads) {
        thread.join();
    }

    // Imprime os resultados
    int totalFiles = allResults.size();
    int validFiles = 0;
    int invalidFiles = 0;

    for (const auto& result : allResults) {
        if (result.isValid) {
            validFiles++;
        } else {
            invalidFiles++;
            std::cerr << "Error: File " << result.filename << " is invalid. Error: " << result.error << std::endl;
        }
    }

    std::cout << "Total files: " << totalFiles << std::endl;
    std::cout << "Valid files: " << validFiles << std::endl;
    std::cout << "Invalid files: " << invalidFiles << std::endl;

    // Se houver arquivos inválidos, o teste falha
    ASSERT_EQ(invalidFiles, 0);
}