#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <string>
#include <map>
#include <vector>

class ConfigReader {
private:
    std::map<std::string, std::string> configs;
    std::string config_path;

public:
    ConfigReader(const std::string& path);
    
    // Ler as configurações do arquivo
    bool lerConfiguracoes();
    
    // Obter um valor específico da configuração
    std::string getValor(const std::string& chave) const;
    
    // Listar e contar arquivos no diretório de entrada
    std::vector<std::string> listarArquivosEntrada() const;
    
    // Mostrar lista de arquivos de entrada no terminal
    void mostrarArquivosEntrada() const;
};

#endif // CONFIG_READER_H
