#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>

namespace fs = std::filesystem;

struct Pedido {
    int id;
    std::map<int, int> itens; // item_id -> quantidade
    int totalItens = 0;
};

struct Corredor {
    int id;
    std::map<int, int> itens; // item_id -> quantidade
};

struct Instancia {
    int numPedidos;
    int numItens;
    int numCorredores;
    int LB;
    int UB;
    std::vector<Pedido> pedidos;
    std::vector<Corredor> corredores;
};

struct Solucao {
    std::vector<int> pedidos;
    std::vector<int> corredores;
};

struct ResultadoValidacao {
    bool lb_ok = false;
    bool ub_ok = false;
    bool disponibilidade_ok = false;
    bool ids_validos = true;
    std::vector<int> ids_invalidos;
    int totalItensColetados = 0;
    int numCorredoresVisitados = 0;
    double razao = 0.0;
    std::string timestamp;
    std::string nomeInstancia;
};

// Função para ler a instância de um arquivo
Instancia lerInstancia(const std::string& path) {
    Instancia instancia;
    std::ifstream file(path);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << path << std::endl;
        return instancia;
    }

    // Primeira linha: numPedidos, numItens, numCorredores
    std::getline(file, line);
    std::stringstream ss(line);
    ss >> instancia.numPedidos >> instancia.numItens >> instancia.numCorredores;

    // Pedidos
    instancia.pedidos.resize(instancia.numPedidos);
    for (int i = 0; i < instancia.numPedidos; ++i) {
        instancia.pedidos[i].id = i;
        std::getline(file, line);
        std::stringstream ssPedido(line);
        
        int numTiposItens;
        ssPedido >> numTiposItens;
        
        for (int j = 0; j < numTiposItens; ++j) {
            int itemId, quantidade;
            ssPedido >> itemId >> quantidade;
            instancia.pedidos[i].itens[itemId] = quantidade;
            instancia.pedidos[i].totalItens += quantidade;
        }
    }

    // Corredores
    instancia.corredores.resize(instancia.numCorredores);
    for (int i = 0; i < instancia.numCorredores; ++i) {
        instancia.corredores[i].id = i;
        std::getline(file, line);
        std::stringstream ssCorredor(line);
        
        int numTiposItens;
        ssCorredor >> numTiposItens;
        
        for (int j = 0; j < numTiposItens; ++j) {
            int itemId, quantidade;
            ssCorredor >> itemId >> quantidade;
            instancia.corredores[i].itens[itemId] = quantidade;
        }
    }

    // Última linha: LB, UB
    std::getline(file, line);
    std::stringstream ssLimites(line);
    ssLimites >> instancia.LB >> instancia.UB;

    return instancia;
}

// Função para ler a solução de um arquivo
Solucao lerSolucao(const std::string& path) {
    Solucao solucao;
    std::ifstream file(path);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << path << std::endl;
        return solucao;
    }

    // Número de pedidos
    std::getline(file, line);
    int numPedidos;
    std::stringstream(line) >> numPedidos;

    // Lê cada pedido (um por linha)
    for (int i = 0; i < numPedidos; i++) {
        if (std::getline(file, line)) {
            int pedidoId;
            std::stringstream(line) >> pedidoId;
            solucao.pedidos.push_back(pedidoId);
        }
    }

    // Número de corredores
    if (std::getline(file, line)) {
        int numCorredores;
        std::stringstream(line) >> numCorredores;

        // Lê cada corredor (um por linha)
        for (int i = 0; i < numCorredores; i++) {
            if (std::getline(file, line)) {
                int corredorId;
                std::stringstream(line) >> corredorId;
                solucao.corredores.push_back(corredorId);
            }
        }
    }

    return solucao;
}

// Função para calcular a razão
double calcularRazao(const Instancia& instancia, const Solucao& solucao) {
    int totalItensColetados = 0;
    for (int pedidoId : solucao.pedidos) {
        if (pedidoId >= 0 && pedidoId < instancia.numPedidos) {
            totalItensColetados += instancia.pedidos[pedidoId].totalItens;
        }
    }

    int numCorredoresVisitados = solucao.corredores.size();

    if (numCorredoresVisitados == 0) {
        return 0.0; // Evitar divisão por zero
    }

    return static_cast<double>(totalItensColetados) / numCorredoresVisitados;
}

// Função para validar se os corredores contêm itens suficientes para os pedidos
bool verificarDisponibilidadeItens(const Instancia& instancia, const Solucao& solucao) {
    // 1. Calcular a demanda total por cada tipo de item
    std::map<int, int> demandaTotal;
    for (int pedidoId : solucao.pedidos) {
        if (pedidoId >= 0 && pedidoId < instancia.numPedidos) {
            const Pedido& pedido = instancia.pedidos[pedidoId];
            for (const auto& [itemId, quantidade] : pedido.itens) {
                demandaTotal[itemId] += quantidade;
            }
        }
    }
    
    // 2. Calcular a disponibilidade total nos corredores selecionados
    std::map<int, int> disponibilidadeTotal;
    for (int corredorId : solucao.corredores) {
        if (corredorId >= 0 && corredorId < instancia.numCorredores) {
            const Corredor& corredor = instancia.corredores[corredorId];
            for (const auto& [itemId, quantidade] : corredor.itens) {
                disponibilidadeTotal[itemId] += quantidade;
            }
        }
    }
    
    // 3. Verificar se a disponibilidade atende à demanda
    for (const auto& [itemId, quantidadeDemandada] : demandaTotal) {
        if (quantidadeDemandada > disponibilidadeTotal[itemId]) {
            return false;  // Disponibilidade insuficiente
        }
    }
    
    return true;  // Disponibilidade suficiente para todos os itens
}

// Função para validar uma solução
ResultadoValidacao validarSolucao(const Instancia& instancia, const Solucao& solucao, const std::string& nomeInstancia) {
    ResultadoValidacao resultado;
    resultado.nomeInstancia = nomeInstancia;
    
    // Adicionar timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    resultado.timestamp = ss.str();
    
    // Verificar IDs de pedidos
    for (int pedidoId : solucao.pedidos) {
        if (pedidoId < 0 || pedidoId >= instancia.numPedidos) {
            resultado.ids_validos = false;
            resultado.ids_invalidos.push_back(pedidoId);
        }
    }
    
    // Verificar IDs de corredores
    for (int corredorId : solucao.corredores) {
        if (corredorId < 0 || corredorId >= instancia.numCorredores) {
            resultado.ids_validos = false;
            resultado.ids_invalidos.push_back(corredorId);
        }
    }
    
    // Calcular total de itens coletados
    resultado.totalItensColetados = 0;
    for (int pedidoId : solucao.pedidos) {
        if (pedidoId >= 0 && pedidoId < instancia.numPedidos) {
            resultado.totalItensColetados += instancia.pedidos[pedidoId].totalItens;
        }
    }
    
    resultado.numCorredoresVisitados = solucao.corredores.size();
    resultado.razao = calcularRazao(instancia, solucao);
    
    // Verificar LB
    resultado.lb_ok = (resultado.totalItensColetados >= instancia.LB);
    
    // Verificar UB
    resultado.ub_ok = (resultado.totalItensColetados <= instancia.UB);
    
    // Verificar disponibilidade de itens usando a função especializada
    resultado.disponibilidade_ok = verificarDisponibilidadeItens(instancia, solucao);
    
    return resultado;
}

// Função para exibir os resultados de uma validação
void exibirResultadoValidacao(const ResultadoValidacao& resultado, const Instancia& instancia) {
    std::cout << "=== RESULTADO DA VALIDAÇÃO: " << resultado.nomeInstancia << " ===" << std::endl;
    std::cout << "Data/Hora: " << resultado.timestamp << std::endl << std::endl;

    std::cout << "MÉTRICAS:" << std::endl;
    std::cout << "- Razão (Itens/Corredores): " << std::fixed << std::setprecision(5) << resultado.razao << std::endl;
    std::cout << "- Total de Itens Coletados: " << resultado.totalItensColetados << std::endl;
    std::cout << "- Número de Corredores Visitados: " << resultado.numCorredoresVisitados << std::endl;
    std::cout << "- Limites (LB, UB): (" << instancia.LB << ", " << instancia.UB << ")" << std::endl << std::endl;

    std::cout << "VALIDAÇÃO DE RESTRIÇÕES:" << std::endl;
    std::cout << "- Limite Inferior (LB): " << (resultado.lb_ok ? "OK" : "FALHA") << std::endl;
    std::cout << "- Limite Superior (UB): " << (resultado.ub_ok ? "OK" : "FALHA") << std::endl;
    std::cout << "- Disponibilidade de Itens: " << (resultado.disponibilidade_ok ? "OK" : "FALHA") << std::endl;
    std::cout << "- IDs Válidos: " << (resultado.ids_validos ? "OK" : "FALHA") << std::endl << std::endl;

    // Exibir IDs inválidos, se houver
    if (!resultado.ids_validos && !resultado.ids_invalidos.empty()) {
        std::cout << "IDs INVÁLIDOS DETECTADOS:" << std::endl;
        for (int id : resultado.ids_invalidos) {
            std::cout << "- ID: " << id << std::endl;
        }
        std::cout << std::endl;
    }

    // Resultado final
    bool solucaoValida = resultado.lb_ok && resultado.ub_ok && 
                         resultado.disponibilidade_ok && resultado.ids_validos;
    
    std::cout << "RESULTADO FINAL: " << (solucaoValida ? "SOLUÇÃO VÁLIDA" : "SOLUÇÃO INVÁLIDA") << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
}

// Função para extrair o nome base do arquivo
std::string obterNomeBase(const std::string& caminho) {
    fs::path path(caminho);
    return path.filename().string();
}

// Função para verificar se um arquivo existe
bool arquivoExiste(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    return arquivo.good();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <diretorio_instancias> <diretorio_solucoes>" << std::endl;
        return 1;
    }

    std::string dirInstancias = argv[1];
    std::string dirSolucoes = argv[2];

    // Verificar se os diretórios existem
    if (!fs::exists(dirInstancias) || !fs::is_directory(dirInstancias)) {
        std::cerr << "Erro: Diretório de instâncias não encontrado: " << dirInstancias << std::endl;
        return 1;
    }

    if (!fs::exists(dirSolucoes) || !fs::is_directory(dirSolucoes)) {
        std::cerr << "Erro: Diretório de soluções não encontrado: " << dirSolucoes << std::endl;
        return 1;
    }

    std::vector<ResultadoValidacao> todosResultados;
    int totalInstancias = 0;
    int instanciasValidas = 0;

    // Processar todos os arquivos .txt no diretório de instâncias
    for (const auto& entry : fs::directory_iterator(dirInstancias)) {
        if (entry.path().extension() == ".txt" && !entry.is_directory()) {
            std::string caminhoInstancia = entry.path().string();
            std::string nomeInstancia = obterNomeBase(caminhoInstancia);
            std::string nomeEsperadoSolucao = nomeInstancia + "_solution.txt";
            std::string caminhoSolucao = dirSolucoes + "/" + nomeEsperadoSolucao;

            totalInstancias++;

            // Verificar se o arquivo de solução existe
            if (!arquivoExiste(caminhoSolucao)) {
                std::cout << "Aviso: Solução não encontrada para " << nomeInstancia << std::endl;
                continue;
            }

            // Ler instância e solução
            Instancia instancia = lerInstancia(caminhoInstancia);
            Solucao solucao = lerSolucao(caminhoSolucao);

            // Validar solução
            ResultadoValidacao resultado = validarSolucao(instancia, solucao, nomeInstancia);
            
            // Exibir resultados para esta instância
            exibirResultadoValidacao(resultado, instancia);

            // Verificar se a solução é válida
            bool solucaoValida = resultado.lb_ok && resultado.ub_ok && 
                               resultado.disponibilidade_ok && resultado.ids_validos;
            if (solucaoValida) {
                instanciasValidas++;
            }

            // Guardar resultado para o resumo final
            todosResultados.push_back(resultado);
        }
    }

    // Exibir resumo
    std::cout << "\n===== RESUMO DA VALIDAÇÃO =====" << std::endl;
    std::cout << "Total de instâncias: " << totalInstancias << std::endl;
    std::cout << "Instâncias com soluções válidas: " << instanciasValidas << std::endl;
    std::cout << "Taxa de sucesso: " << std::fixed << std::setprecision(2) 
              << (totalInstancias > 0 ? (100.0 * instanciasValidas / totalInstancias) : 0) 
              << "%" << std::endl << std::endl;

    // Exibir métricas de instâncias válidas
    if (!todosResultados.empty()) {
        double razaoMedia = 0.0;
        int totalItens = 0;
        int totalCorredores = 0;

        for (const auto& resultado : todosResultados) {
            bool solucaoValida = resultado.lb_ok && resultado.ub_ok && 
                               resultado.disponibilidade_ok && resultado.ids_validos;
            if (solucaoValida) {
                razaoMedia += resultado.razao;
                totalItens += resultado.totalItensColetados;
                totalCorredores += resultado.numCorredoresVisitados;
            }
        }

        if (instanciasValidas > 0) {
            razaoMedia /= instanciasValidas;
            std::cout << "MÉTRICAS PARA SOLUÇÕES VÁLIDAS:" << std::endl;
            std::cout << "- Razão média (Itens/Corredores): " << std::fixed << std::setprecision(5) << razaoMedia << std::endl;
            std::cout << "- Total de itens coletados: " << totalItens << std::endl;
            std::cout << "- Total de corredores visitados: " << totalCorredores << std::endl;
        }
    }

    return (instanciasValidas == totalInstancias) ? 0 : 1;
}