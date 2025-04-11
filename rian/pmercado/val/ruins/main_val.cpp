#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <filesystem> // C++17


/*
2. Validação das Restrições:

A função validarRestricoes faz exatamente isso:

Restrição de Limite Inferior (LB): Calcula o número total de itens coletados e verifica se é maior ou igual a LB.
Restrição de Limite Superior (UB): Calcula o número total de itens coletados e verifica se é menor ou igual a UB.
Restrição de Disponibilidade de Itens:
Cria um mapa (itensDemandados) com a demanda total de cada item na wave.
Cria um mapa (itensDisponiveis) com a disponibilidade total de cada item nos corredores selecionados.
Verifica se a quantidade demandada de cada item é menor ou igual à quantidade disponível.
3. Saída:

O programa agora imprime:

O nome da instância.
A razão calculada.
Um indicador se as restrições foram satisfeitas (Sim ou Nao).
4. Como Usar:

Salve o código como validador.cpp.
Compile: g++ -std=c++17 validador.cpp -o validador
Execute: ./validador <diretorio_instancias> <diretorio_saidas>
5. Próximos Passos:

Testar: Teste o código com várias instâncias e arquivos de saída para garantir que ele está funcionando corretamente.
Otimizar: Se precisar de mais desempenho, você pode otimizar a leitura dos arquivos e os cálculos.
Melhorar a Saída: Formate a saída para que seja mais fácil de ler e interpretar.
Adicionar Mais Validações: Se houver outras regras ou restrições no desafio, você pode adicioná-las à função validarRestricoes.
Com este código, você terá uma ferramenta poderosa para validar seus resultados e garantir que suas soluções estejam em conformidade com as regras do desafio. Se tiver mais alguma dúvida, é só perguntar!
*/

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

Solucao lerSolucao(const std::string& path) {
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
    }

    // Número de corredores
    std::getline(file, line);
    int numCorredores;
    std::stringstream ssNumCorredores(line);
    ssNumCorredores >> numCorredores;

    // Corredores
    solucao.corredores.resize(numCorredores);
    for (int i = 0; i < numCorredores; ++i) {
        std::getline(file, line);
        std::stringstream ssCorredor(line);
        ssCorredor >> solucao.corredores[i];
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

bool validarRestricoes(const Instancia& instancia, const Solucao& solucao) {
    // 1. Restrição de Limite Inferior (LB)
    int totalItensColetados = 0;
    for (int pedidoId : solucao.pedidos) {
        totalItensColetados += instancia.pedidos[pedidoId].totalItens;
    }
    if (totalItensColetados < instancia.LB) {
        std::cerr << "Erro: Restricao de Limite Inferior nao satisfeita. Total de itens: " << totalItensColetados << ", LB: " << instancia.LB << std::endl;
        return false;
    }

    // 2. Restrição de Limite Superior (UB)
    if (totalItensColetados > instancia.UB) {
        std::cerr << "Erro: Restricao de Limite Superior nao satisfeita. Total de itens: " << totalItensColetados << ", UB: " << instancia.UB << std::endl;
        return false;
    }

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

    for (const auto& itemDemandado : itensDemandados) {
        int itemId = itemDemandado.first;
        int quantidadeDemandada = itemDemandado.second;
        int quantidadeDisponivel = itensDisponiveis[itemId];
        if (quantidadeDemandada > quantidadeDisponivel) {
            std::cerr << "Erro: Restricao de Disponibilidade de Itens nao satisfeita. Item: " << itemId
                      << ", Demandado: " << quantidadeDemandada << ", Disponivel: " << quantidadeDisponivel << std::endl;
            return false;
        }
    }

    return true;
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
            Solucao solucao = lerSolucao(pathSaida);
            double razao = calcularRazao(instancia, solucao);
            bool valido = validarRestricoes(instancia, solucao);

            std::cout << "Instancia: " << nomeInstancia << ", Razao: " << razao << ", Valido: " << (valido ? "Sim" : "Nao") << std::endl;
        }
    }

    return 0;
}