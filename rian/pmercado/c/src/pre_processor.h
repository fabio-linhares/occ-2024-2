#ifndef PRE_PROCESSOR_H
#define PRE_PROCESSOR_H

#include <string>
#include <future>

// Classe para pré-processamento de arquivos
class PreProcessor {
public:
    // Construtor
    PreProcessor();
    
    // Função para pré-processar um arquivo
    // Retorna true se o arquivo deve ser processado, false para pular
    bool preProcessarArquivo(const std::string& arquivo_caminho, int indice, int total_arquivos, int threads_disponiveis);
    
    // Verifica se deve continuar o processamento após este arquivo
    bool deveContinuar() const;
    
    // Processa um arquivo de instância e extrai informações
    static void processarArquivoInstancia(const std::string& arquivo_caminho);
    
    // Processa a última linha do arquivo para obter LB e UB
    static std::pair<int, int> processarUltimaLinhaAsync(const std::string& arquivo_caminho);

private:
    bool continuar_processamento;
    int max_arquivos_processados;
};

#endif // PRE_PROCESSOR_H