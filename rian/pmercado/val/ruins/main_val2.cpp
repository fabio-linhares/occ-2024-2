#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <filesystem> // C++17

struct Pedido {
    int id;
    std::map<int, int> itens; // item_id -> quantidade
    int totalItens = 0; // Total de itens no pedido
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

Instancia lerInstancia(const std::string& path) {
    Instancia instancia;
    std::ifstream file(path);
    std::string line;

    // Primeira linha: o, i, a
    std::getline(file, line);
    std::stringstream ss(line);
    ss >> instancia.numPedidos >> instancia.numItens >> instancia.numCorredores;

    // Pedidos
    instancia.pedidos.resize(instancia.numPedidos);
    for (int i = 0; i < instancia.numPedidos; ++i) {
        instancia.pedidos[i].id = i;
        std::getline(file, line);
        std::stringstream ssPedido(line);
        int k;
        ssPedido >> k;
        for (int j = 0; j < k; ++j) {
            int item, quantidade;
            ssPedido >> item >> quantidade;
            instancia.pedidos[i].itens[item] = quantidade;
            instancia.pedidos[i].totalItens += quantidade;
        }
    }

    // Corredores
    instancia.corredores.resize(instancia.numCorredores);
    for (int i = 0; i < instancia.numCorredores; ++i) {
        instancia.corredores[i].id = i;
        std::getline(file, line);
        std::stringstream ssCorredor(line);
        int l;
        ssCorredor >> l;
        for (int j = 0; j < l; ++j) {
            int item, quantidade;
            ssCorredor >> item >> quantidade;
            instancia.corredores[i].itens[item] = quantidade;
        }
    }

    // Última linha: LB, UB
    std::getline(file, line);
    std::stringstream ssLimites(line);
    ssLimites >> instancia.LB >> instancia.UB;

    return instancia;
}

Solucao lerSolucao(const std::string& path, const Instancia& instancia) {
    Solucao solucao;
    std::ifstream file(path);
    std::string line;

    // Número de pedidos
    std::getline(file, line);
    int numPedidos;
    std::stringstream ssNumPedidos(line);
    ssNumPedidos >> numPedidos;

    // Pedidos
    solucao.pedidos.resize(numPedidos);
    for (int i = 0; i < numPedidos; ++i) {
        std::getline(file, line);
        std::stringstream ssPedido(line);
        ssPedido >> solucao.pedidos[i];

        // Verificar se o ID do pedido é válido
        if (solucao.pedidos[i] < 0 || solucao.pedidos[i] >= instancia.numPedidos) {
            std::cerr << "Erro: ID de pedido invalido (" << solucao.pedidos[i] << ") no arquivo de solucao: " << path << std::endl;
            solucao.pedidos[i] = 0; // Definir um valor padrão para evitar problemas futuros
        }
    }

    // Número de corredores
    std::getline(file, line);
    int numCorredores;
    std::stringstream ssNumCorredores(line);
    ssNumCorredores >> numCorredores;

    // Corredores
    solucao.corredores.clear();
    solucao.corredores.reserve(numCorredores); // Reserve espaço para evitar realocações

    // Ler os IDs dos corredores
    std::getline(file, line);
    std::stringstream ssCorredores(line);
    int corredorId;
    while (ssCorredores >> corredorId) {
        // Verificar se o ID do corredor é válido
        if (corredorId < 0 || corredorId >= instancia.numCorredores) {
            std::cerr << "Erro: ID de corredor invalido (" << corredorId << ") no arquivo de solucao: " << path << std::endl;
            corredorId = 0; // Definir um valor padrão para evitar problemas futuros
        }
        solucao.corredores.push_back(corredorId);
    }

    return solucao;
}

double calcularRazao(const Instancia& instancia, const Solucao& solucao) {
    int totalItensColetados = 0;
    for (int pedidoId : solucao.pedidos) {
        totalItensColetados += instancia.pedidos[pedidoId].totalItens;
    }

    int numCorredoresVisitados = solucao.corredores.size();

    if (numCorredoresVisitados == 0) {
        return 0.0; // Evitar divisão por zero
    }

    return static_cast<double>(totalItensColetados) / numCorredoresVisitados;
}

struct ResultadoValidacao {
    bool lb_ok = false;
    bool ub_ok = false;
    bool disponibilidade_ok = false;
    int totalItensColetados = 0;
    int numCorredoresVisitados = 0;
    double razao = 0.0;
};

ResultadoValidacao validarRestricoes(const Instancia& instancia, const Solucao& solucao) {
    ResultadoValidacao resultado;

    // 1. Restrição de Limite Inferior (LB)
    resultado.totalItensColetados = 0;
    for (int pedidoId : solucao.pedidos) {
        resultado.totalItensColetados += instancia.pedidos[pedidoId].totalItens;
    }
    resultado.lb_ok = (resultado.totalItensColetados >= instancia.LB);

    // 2. Restrição de Limite Superior (UB)
    resultado.ub_ok = (resultado.totalItensColetados <= instancia.UB);

    // 3. Restrição de Disponibilidade de Itens
    std::map<int, int> itensDemandados;
    for (int pedidoId : solucao.pedidos) {
        const Pedido& pedido = instancia.pedidos[pedidoId];
        for (const auto& item : pedido.itens) {
            int itemId = item.first;
            int quantidade = item.second;
            itensDemandados[itemId] += quantidade;
        }
    }

    std::map<int, int> itensDisponiveis;
    for (int corredorId : solucao.corredores) {
        const Corredor& corredor = instancia.corredores[corredorId];
        for (const auto& item : corredor.itens) {
            int itemId = item.first;
            int quantidade = item.second;
            itensDisponiveis[itemId] += quantidade;
        }
    }

    resultado.disponibilidade_ok = true;
    for (const auto& itemDemandado : itensDemandados) {
        int itemId = itemDemandado.first;
        int quantidadeDemandada = itemDemandado.second;
        int quantidadeDisponivel = itensDisponiveis[itemId];
        if (quantidadeDemandada > quantidadeDisponivel) {
            resultado.disponibilidade_ok = false;
            break;
        }
    }

    resultado.numCorredoresVisitados = solucao.corredores.size();
    if (resultado.numCorredoresVisitados > 0) {
        resultado.razao = static_cast<double>(resultado.totalItensColetados) / resultado.numCorredoresVisitados;
    }

    return resultado;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <diretorio_instancias> <diretorio_saidas>" << std::endl;
        return 1;
    }

    std::string dirInstancias = argv[1];
    std::string dirSaidas = argv[2];

    // Arquivo de relatório
    std::ofstream relatorio("relatorio.txt");
    if (!relatorio.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de relatorio." << std::endl;
        return 1;
    }

    relatorio << "Relatorio de Validacao das Instancias e Solucoes" << std::endl;
    relatorio << "--------------------------------------------------" << std::endl;

    for (const auto& entry : std::filesystem::directory_iterator(dirInstancias)) {
        if (entry.is_regular_file() && entry.path().string().find("instance_") != std::string::npos) {
            std::string nomeInstancia = entry.path().filename().string();
            std::string pathInstancia = entry.path().string();
            std::string nomeSaida = nomeInstancia.substr(0, nomeInstancia.size() - 4) + "_out.txt"; // Remove ".txt" e adiciona "_out.txt"
            std::string pathSaida = dirSaidas + "/" + nomeSaida;

            // Verificar se o arquivo de saída existe
            std::ifstream saidaFile(pathSaida);
            if (!saidaFile.good()) {
                std::cerr << "Arquivo de saida nao encontrado: " << pathSaida << std::endl;
                continue;
            }

            Instancia instancia = lerInstancia(pathInstancia);
            Solucao solucao = lerSolucao(pathSaida, instancia);
            ResultadoValidacao resultado = validarRestricoes(instancia, solucao);

            std::cout << "Instancia: " << nomeInstancia << std::endl;
            std::cout << "  - Razao: " << resultado.razao << std::endl;
            std::cout << "  - Limite Inferior: " << (resultado.lb_ok ? "OK" : "FALHA") << std::endl;
            std::cout << "  - Limite Superior: " << (resultado.ub_ok ? "OK" : "FALHA") << std::endl;
            std::cout << "  - Disponibilidade de Itens: " << (resultado.disponibilidade_ok ? "OK" : "FALHA") << std::endl;
            std::cout << "  - Total de Itens Coletados: " << resultado.totalItensColetados << std::endl;
            std::cout << "  - Numero de Corredores Visitados: " << resultado.numCorredoresVisitados << std::endl;
            std::cout << "  - Limites (LB, UB): (" << instancia.LB << ", " << instancia.UB << ")" << std::endl;
            std::cout << std::endl;

            relatorio << "Instancia: " << nomeInstancia << std::endl;
            relatorio << "  - Razao: " << resultado.razao << std::endl;
            relatorio << "  - Limite Inferior: " << (resultado.lb_ok ? "OK" : "FALHA") << std::endl;
            relatorio << "  - Limite Superior: " << (resultado.ub_ok ? "OK" : "FALHA") << std::endl;
            relatorio << "  - Disponibilidade de Itens: " << (resultado.disponibilidade_ok ? "OK" : "FALHA") << std::endl;
            relatorio << "  - Total de Itens Coletados: " << resultado.totalItensColetados << std::endl;
            relatorio << "  - Numero de Corredores Visitados: " << resultado.numCorredoresVisitados << std::endl;
            relatorio << "  - Limites (LB, UB): (" << instancia.LB << ", " << instancia.UB << ")" << std::endl;
            relatorio << std::endl;
        }
    }

    relatorio.close();
    std::cout << "Relatorio gerado em relatorio.txt" << std::endl;

    return 0;
}