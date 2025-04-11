#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>

/*
O que este Validador Faz
Este programa apenas valida as soluções existentes, sem fazer qualquer tipo de correção. Ele:

Lê cada instância e sua respectiva solução
Verifica se todas as restrições são atendidas:
IDs válidos: Verifica se todos os IDs de pedidos e corredores são válidos
Limite Inferior (LB): Verifica se o total de itens coletados é ≥ LB
Limite Superior (UB): Verifica se o total de itens coletados é ≤ UB
Disponibilidade de Itens: Verifica se há estoque suficiente para todos os itens pedidos
Imprime o resultado de cada validação, identificando claramente quais restrições foram violadas
Exibe informações estatísticas como total de itens coletados, corredores visitados e a razão
O programa mostrará "FALHA" para cada restrição não satisfeita, facilitando a identificação dos problemas nas soluções existentes.
*/

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
    std::stringstream ssNumPedidos(line);
    ssNumPedidos >> numPedidos;

    // Pedidos
    std::getline(file, line);
    std::stringstream ssPedidos(line);
    int pedidoId;
    while (ssPedidos >> pedidoId) {
        solucao.pedidos.push_back(pedidoId);
    }

    // Número de corredores
    std::getline(file, line);
    int numCorredores;
    std::stringstream ssNumCorredores(line);
    ssNumCorredores >> numCorredores;

    // Corredores
    std::getline(file, line);
    std::stringstream ssCorredores(line);
    int corredorId;
    while (ssCorredores >> corredorId) {
        solucao.corredores.push_back(corredorId);
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

// Estrutura para armazenar o resultado da validação
struct ResultadoValidacao {
    bool lb_ok = false;
    bool ub_ok = false;
    bool disponibilidade_ok = false;
    bool ids_validos = true;
    std::vector<int> ids_invalidos;
    int totalItensColetados = 0;
    int numCorredoresVisitados = 0;
    double razao = 0.0;
};

// Função para validar uma solução
ResultadoValidacao validarSolucao(const Instancia& instancia, const Solucao& solucao) {
    ResultadoValidacao resultado;
    
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
    
    // Verificar disponibilidade de itens
    std::map<int, int> itensDemandados;
    for (int pedidoId : solucao.pedidos) {
        if (pedidoId >= 0 && pedidoId < instancia.numPedidos) {
            const Pedido& pedido = instancia.pedidos[pedidoId];
            for (const auto& [itemId, quantidade] : pedido.itens) {
                itensDemandados[itemId] += quantidade;
            }
        }
    }
    
    std::map<int, int> itensDisponiveis;
    for (int corredorId : solucao.corredores) {
        if (corredorId >= 0 && corredorId < instancia.numCorredores) {
            const Corredor& corredor = instancia.corredores[corredorId];
            for (const auto& [itemId, quantidade] : corredor.itens) {
                itensDisponiveis[itemId] += quantidade;
            }
        }
    }
    
    resultado.disponibilidade_ok = true;
    for (const auto& [itemId, quantidadeDemandada] : itensDemandados) {
        int quantidadeDisponivel = itensDisponiveis.count(itemId) ? itensDisponiveis.at(itemId) : 0;
        if (quantidadeDemandada > quantidadeDisponivel) {
            resultado.disponibilidade_ok = false;
            break;
        }
    }
    
    return resultado;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <pasta_instancias> <pasta_saida>" << std::endl;
        return 1;
    }

    std::string pasta_instancias = argv[1];
    std::string pasta_saida = argv[2];

    for (const auto& entry : std::filesystem::directory_iterator(pasta_instancias)) {
        if (entry.path().extension() == ".txt" && entry.path().filename().string().find("instance_") == 0) {
            std::string nome_arquivo = entry.path().filename().string();
            std::string caminho_instancia = entry.path().string();
            std::string nome_instancia = nome_arquivo.substr(0, nome_arquivo.size() - 4);
            std::string caminho_solucao = pasta_saida + "/" + nome_instancia + "_out.txt";

            std::cout << "\nInstancia: " << nome_instancia << std::endl;
            
            Instancia instancia = lerInstancia(caminho_instancia);
            
            // Verificar se o arquivo de solução existe
            std::ifstream file_test(caminho_solucao);
            if (!file_test.good()) {
                std::cout << "  - Arquivo de solução não encontrado!" << std::endl;
                continue;
            }
            file_test.close();
            
            Solucao solucao = lerSolucao(caminho_solucao);
            ResultadoValidacao resultado = validarSolucao(instancia, solucao);
            
            // Formatar razão com 5 casas decimais
            std::cout << "  - Razao: " << std::fixed << std::setprecision(5) << resultado.razao << std::endl;
            
            // Mostrar resultado para cada restrição
            std::cout << "  - Limite Inferior: " << (resultado.lb_ok ? "OK" : "FALHA") << std::endl;
            std::cout << "  - Limite Superior: " << (resultado.ub_ok ? "OK" : "FALHA") << std::endl;
            std::cout << "  - Disponibilidade de Itens: " << (resultado.disponibilidade_ok ? "OK" : "FALHA") << std::endl;
            
            // Mostrar estatísticas
            std::cout << "  - Total de Itens Coletados: " << resultado.totalItensColetados << std::endl;
            std::cout << "  - Numero de Corredores Visitados: " << resultado.numCorredoresVisitados << std::endl;
            std::cout << "  - Limites (LB, UB): (" << instancia.LB << ", " << instancia.UB << ")" << std::endl;
            
            // Verificar IDs inválidos
            if (!resultado.ids_validos && !resultado.ids_invalidos.empty()) {
                std::cout << "Erro: ID de pedido invalido (";
                for (size_t i = 0; i < resultado.ids_invalidos.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << resultado.ids_invalidos[i];
                }
                std::cout << ") no arquivo de solucao: " << caminho_solucao << std::endl;
            }
        }
    }

    return 0;
}