#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <atomic>
#include "pre_processor.h" // Incluir o cabeçalho do pré-processador

// Declaração antecipada para evitar inclusão circular
class ConfigManager;

// Variáveis globais para armazenar informações da instância
extern int g_num_pedidos;
extern int g_num_itens;
extern int g_num_corredores;
extern int g_limite_inferior;
extern int g_limite_superior;

class FileManager {
public:
    FileManager(const ConfigManager& configManager);
    
    // Lista os arquivos no diretório de entrada
    void listarArquivosEntrada() const;
    
    // Retorna a lista de arquivos no diretório de entrada
    std::vector<std::string> obterArquivosEntrada() const;
    
    // Conta o número de arquivos no diretório de entrada
    int contarArquivosEntrada() const;
    
    // Consulta o número de threads disponíveis no sistema
    int consultarThreadsDisponiveis() const;
    
    // Processa um arquivo com base no número de threads disponíveis
    bool processarArquivo(const std::string& arquivo_caminho, int threads_disponiveis) const;
    
    // Ordena arquivos usando uma thread dedicada
    std::vector<std::string> ordenarArquivos(const std::string& input_dir) const;
    
    // Lista arquivos com pré-processamento
    void listarArquivosComPreProcessamento() const;
    
    // Limpa as variáveis globais de instância
    static void limparDadosInstancia();

private:
    const ConfigManager& configManager;
};

#endif // FILE_MANAGER_H