#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cmath>

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

// Função para salvar a solução em um arquivo
void salvarSolucao(const std::string& path, const Solucao& solucao) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo para escrita: " << path << std::endl;
        return;
    }

    // Número de pedidos
    file << solucao.pedidos.size() << std::endl;

    // Pedidos
    for (size_t i = 0; i < solucao.pedidos.size(); ++i) {
        if (i > 0) file << " ";
        file << solucao.pedidos[i];
    }
    file << std::endl;

    // Número de corredores
    file << solucao.corredores.size() << std::endl;

    // Corredores
    for (size_t i = 0; i < solucao.corredores.size(); ++i) {
        if (i > 0) file << " ";
        file << solucao.corredores[i];
    }
    file << std::endl;
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

// Função para verificar se o pedido requer algum item que já está no limite da disponibilidade
bool pedidoExcedeDisponibilidade(const Pedido& pedido, 
                                const std::map<int, int>& itensDemandados, 
                                const std::map<int, int>& itensDisponiveis) {
    for (const auto& [itemId, quantidade] : pedido.itens) {
        int novaQuantidadeDemandada = itensDemandados.count(itemId) ? 
                                       itensDemandados.at(itemId) + quantidade : 
                                       quantidade;
                                       
        int quantidadeDisponivel = itensDisponiveis.count(itemId) ? 
                                   itensDisponiveis.at(itemId) : 0;
                                   
        if (novaQuantidadeDemandada > quantidadeDisponivel) {
            return true;
        }
    }
    return false;
}

// Função para validar e corrigir a solução
bool corrigirSolucao(const Instancia& instancia, Solucao& solucao) {
    // 1. Remover pedidos inválidos
    solucao.pedidos.erase(
        std::remove_if(solucao.pedidos.begin(), solucao.pedidos.end(),
            [&instancia](int id) { return id < 0 || id >= instancia.numPedidos; }),
        solucao.pedidos.end()
    );

    // 2. Remover corredores inválidos
    solucao.corredores.erase(
        std::remove_if(solucao.corredores.begin(), solucao.corredores.end(),
            [&instancia](int id) { return id < 0 || id >= instancia.numCorredores; }),
        solucao.corredores.end()
    );

    // 3. Verificar disponibilidade de itens
    std::map<int, int> itensDisponiveis;
    for (int corredorId : solucao.corredores) {
        const Corredor& corredor = instancia.corredores[corredorId];
        for (const auto& [itemId, quantidade] : corredor.itens) {
            itensDisponiveis[itemId] += quantidade;
        }
    }

    // 4. Remover pedidos que excedem a disponibilidade
    std::map<int, int> itensDemandados;
    std::vector<int> pedidosValidos;
    int totalItensColetados = 0;

    for (int pedidoId : solucao.pedidos) {
        const Pedido& pedido = instancia.pedidos[pedidoId];
        bool podeAdicionar = true;

        // Verificar se adicionar este pedido excederia a disponibilidade
        for (const auto& [itemId, quantidade] : pedido.itens) {
            int novaQuantidadeDemandada = itensDemandados.count(itemId) ? 
                                          itensDemandados.at(itemId) + quantidade : 
                                          quantidade;
                                          
            int quantidadeDisponivel = itensDisponiveis.count(itemId) ? 
                                      itensDisponiveis.at(itemId) : 0;
                                      
            if (novaQuantidadeDemandada > quantidadeDisponivel) {
                podeAdicionar = false;
                break;
            }
        }

        // Se pode adicionar o pedido
        if (podeAdicionar) {
            pedidosValidos.push_back(pedidoId);
            for (const auto& [itemId, quantidade] : pedido.itens) {
                itensDemandados[itemId] += quantidade;
            }
            totalItensColetados += pedido.totalItens;
        }
    }

    solucao.pedidos = pedidosValidos;

    // 5. Ajustar pedidos para respeitar UB
    while (totalItensColetados > instancia.UB && !solucao.pedidos.empty()) {
        int ultimoPedidoId = solucao.pedidos.back();
        const Pedido& ultimoPedido = instancia.pedidos[ultimoPedidoId];
        
        solucao.pedidos.pop_back();
        
        for (const auto& [itemId, quantidade] : ultimoPedido.itens) {
            itensDemandados[itemId] -= quantidade;
        }
        
        totalItensColetados -= ultimoPedido.totalItens;
    }

    // 6. Adicionar pedidos para tentar atingir LB, se necessário
    if (totalItensColetados < instancia.LB) {
        // Criar conjunto de pedidos já incluídos para rápida verificação
        std::set<int> pedidosIncluidos(solucao.pedidos.begin(), solucao.pedidos.end());
        
        // Vector para armazenar candidatos a serem adicionados (pedidoId, totalItens)
        std::vector<std::pair<int, int>> candidatos;
        
        // Encontrar pedidos que podem ser adicionados
        for (int i = 0; i < instancia.numPedidos; ++i) {
            if (pedidosIncluidos.count(i) == 0) {  // Se ainda não incluído
                const Pedido& pedido = instancia.pedidos[i];
                
                // Verificar se adicionar este pedido excederia a disponibilidade
                bool podeAdicionar = !pedidoExcedeDisponibilidade(
                    pedido, itensDemandados, itensDisponiveis);
                
                if (podeAdicionar && totalItensColetados + pedido.totalItens <= instancia.UB) {
                    candidatos.push_back({i, pedido.totalItens});
                }
            }
        }
        
        // Ordenar candidatos pela quantidade de itens (descendente)
        std::sort(candidatos.begin(), candidatos.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Adicionar pedidos até atingir LB ou não ter mais candidatos
        for (const auto& [pedidoId, numItens] : candidatos) {
            if (totalItensColetados >= instancia.LB) break;
            
            const Pedido& pedido = instancia.pedidos[pedidoId];
            
            // Verificar novamente se adicionar este pedido excederia a disponibilidade
            bool podeAdicionar = !pedidoExcedeDisponibilidade(
                pedido, itensDemandados, itensDisponiveis);
            
            if (podeAdicionar && totalItensColetados + numItens <= instancia.UB) {
                solucao.pedidos.push_back(pedidoId);
                for (const auto& [itemId, quantidade] : pedido.itens) {
                    itensDemandados[itemId] += quantidade;
                }
                totalItensColetados += numItens;
            }
        }
    }

    // 7. Se ainda não atingiu LB, adicionar corredores para aumentar disponibilidade
    if (totalItensColetados < instancia.LB) {
        // Conjunto de corredores já incluídos
        std::set<int> corredoresIncluidos(solucao.corredores.begin(), solucao.corredores.end());
        
        // Iterativamente adicionar corredores e pedidos até atingir LB ou não ter mais opções
        bool melhorou;
        do {
            melhorou = false;
            
            // Para cada corredor não incluído
            for (int i = 0; i < instancia.numCorredores && totalItensColetados < instancia.LB; ++i) {
                if (corredoresIncluidos.count(i) == 0) {
                    // Adicionar corredor temporariamente
                    corredoresIncluidos.insert(i);
                    
                    // Atualizar disponibilidade
                    for (const auto& [itemId, quantidade] : instancia.corredores[i].itens) {
                        itensDisponiveis[itemId] += quantidade;
                    }
                    
                    // Tentar adicionar pedidos
                    int itensAntes = totalItensColetados;
                    
                    // Buscar pedidos que podem ser adicionados agora
                    std::set<int> pedidosIncluidos(solucao.pedidos.begin(), solucao.pedidos.end());
                    for (int j = 0; j < instancia.numPedidos; ++j) {
                        if (pedidosIncluidos.count(j) == 0) {
                            const Pedido& pedido = instancia.pedidos[j];
                            
                            bool podeAdicionar = !pedidoExcedeDisponibilidade(
                                pedido, itensDemandados, itensDisponiveis);
                                
                            if (podeAdicionar && totalItensColetados + pedido.totalItens <= instancia.UB) {
                                solucao.pedidos.push_back(j);
                                pedidosIncluidos.insert(j);
                                for (const auto& [itemId, quantidade] : pedido.itens) {
                                    itensDemandados[itemId] += quantidade;
                                }
                                totalItensColetados += pedido.totalItens;
                            }
                        }
                    }
                    
                    // Se melhorou, manter o corredor
                    if (totalItensColetados > itensAntes) {
                        solucao.corredores.push_back(i);
                        melhorou = true;
                    } else {
                        // Reverter disponibilidade
                        corredoresIncluidos.erase(i);
                        for (const auto& [itemId, quantidade] : instancia.corredores[i].itens) {
                            itensDisponiveis[itemId] -= quantidade;
                        }
                    }
                }
            }
        } while (melhorou && totalItensColetados < instancia.LB);
    }

    // 8. Ordenar pedidos e corredores para exibição consistente
    std::sort(solucao.pedidos.begin(), solucao.pedidos.end());
    std::sort(solucao.corredores.begin(), solucao.corredores.end());
    
    // Verificação final
    totalItensColetados = 0;
    for (int pedidoId : solucao.pedidos) {
        totalItensColetados += instancia.pedidos[pedidoId].totalItens;
    }
    
    // Imprimir resultados da correção
    std::cout << "Após correção: " << solucao.pedidos.size() << " pedidos, "
              << solucao.corredores.size() << " corredores, "
              << totalItensColetados << " itens coletados." << std::endl;
    std::cout << "Limites (LB, UB): (" << instancia.LB << ", " << instancia.UB << ")" << std::endl;
    std::cout << "Resultado LB: " << (totalItensColetados >= instancia.LB ? "OK" : "FALHA") << std::endl;
    std::cout << "Resultado UB: " << (totalItensColetados <= instancia.UB ? "OK" : "FALHA") << std::endl;
    
    double razao = calcularRazao(instancia, solucao);
    std::cout << "Razão: " << razao << std::endl;
    
    return totalItensColetados >= instancia.LB && 
           totalItensColetados <= instancia.UB;
}

// Função para verificar se a solução é válida
struct ResultadoValidacao {
    bool lb_ok = false;
    bool ub_ok = false;
    bool disponibilidade_ok = false;
    bool ids_validos = true;
    int totalItensColetados = 0;
    int numCorredoresVisitados = 0;
    double razao = 0.0;
};

ResultadoValidacao validarSolucao(const Instancia& instancia, const Solucao& solucao) {
    ResultadoValidacao resultado;
    
    // Verificar IDs de pedidos
    for (int pedidoId : solucao.pedidos) {
        if (pedidoId < 0 || pedidoId >= instancia.numPedidos) {
            resultado.ids_validos = false;
            std::cerr << "Erro: ID de pedido inválido (" << pedidoId << ")" << std::endl;
        }
    }
    
    // Verificar IDs de corredores
    for (int corredorId : solucao.corredores) {
        if (corredorId < 0 || corredorId >= instancia.numCorredores) {
            resultado.ids_validos = false;
            std::cerr << "Erro: ID de corredor inválido (" << corredorId << ")" << std::endl;
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

    // Criar pasta de saída se não existir
    std::filesystem::create_directories(pasta_saida);

    for (const auto& entry : std::filesystem::directory_iterator(pasta_instancias)) {
        if (entry.path().extension() == ".txt" && entry.path().filename().string().find("instance_") == 0) {
            std::string nome_arquivo = entry.path().filename().string();
            std::string caminho_instancia = entry.path().string();
            std::string nome_instancia = nome_arquivo.substr(0, nome_arquivo.size() - 4);
            std::string caminho_solucao = pasta_saida + "/" + nome_instancia + "_out.txt";

            std::cout << "\n\n===== Processando: " << nome_arquivo << " =====" << std::endl;
            
            Instancia instancia = lerInstancia(caminho_instancia);
            Solucao solucao;
            
            // Verificar se o arquivo de solução existe
            std::ifstream file_test(caminho_solucao);
            if (file_test.good()) {
                file_test.close();
                solucao = lerSolucao(caminho_solucao);
                
                // Validar solução existente
                ResultadoValidacao resultado = validarSolucao(instancia, solucao);
                
                std::cout << "Validação da solução original:" << std::endl;
                std::cout << "  - IDs válidos: " << (resultado.ids_validos ? "Sim" : "Não") << std::endl;
                std::cout << "  - LB satisfeito: " << (resultado.lb_ok ? "Sim" : "Não") << std::endl;
                std::cout << "  - UB satisfeito: " << (resultado.ub_ok ? "Sim" : "Não") << std::endl;
                std::cout << "  - Disponibilidade satisfeita: " << (resultado.disponibilidade_ok ? "Sim" : "Não") << std::endl;
                std::cout << "  - Total de itens: " << resultado.totalItensColetados << std::endl;
                std::cout << "  - Corredores visitados: " << resultado.numCorredoresVisitados << std::endl;
                std::cout << "  - Razão: " << resultado.razao << std::endl;
                
                // Se a solução não for válida, corrigir
                if (!resultado.ids_validos || !resultado.lb_ok || !resultado.ub_ok || !resultado.disponibilidade_ok) {
                    std::cout << "\nCorrigindo solução..." << std::endl;
                    corrigirSolucao(instancia, solucao);
                    salvarSolucao(caminho_solucao, solucao);
                    
                    // Validar solução corrigida
                    resultado = validarSolucao(instancia, solucao);
                    std::cout << "\nValidação da solução corrigida:" << std::endl;
                    std::cout << "  - IDs válidos: " << (resultado.ids_validos ? "Sim" : "Não") << std::endl;
                    std::cout << "  - LB satisfeito: " << (resultado.lb_ok ? "Sim" : "Não") << std::endl;
                    std::cout << "  - UB satisfeito: " << (resultado.ub_ok ? "Sim" : "Não") << std::endl;
                    std::cout << "  - Disponibilidade satisfeita: " << (resultado.disponibilidade_ok ? "Sim" : "Não") << std::endl;
                    std::cout << "  - Total de itens: " << resultado.totalItensColetados << std::endl;
                    std::cout << "  - Corredores visitados: " << resultado.numCorredoresVisitados << std::endl;
                    std::cout << "  - Razão: " << resultado.razao << std::endl;
                }
            } else {
                // Criar solução a partir do zero
                std::cout << "Arquivo de solução não encontrado. Criando nova solução..." << std::endl;
                
                // Estratégia simples: selecionar corredores com mais itens e depois pedidos que podem ser atendidos
                std::vector<std::pair<int, int>> corredores_ordenados;
                for (int i = 0; i < instancia.numCorredores; ++i) {
                    int total_itens = 0;
                    for (const auto& [itemId, quantidade] : instancia.corredores[i].itens) {
                        total_itens += quantidade;
                    }
                    corredores_ordenados.push_back({i, total_itens});
                }
                
                // Ordenar corredores pelo total de itens (descendente)
                std::sort(corredores_ordenados.begin(), corredores_ordenados.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });
                
                // Selecionar corredores mais "ricos" primeiro
                for (size_t i = 0; i < std::min(size_t(20), corredores_ordenados.size()); ++i) {
                    solucao.corredores.push_back(corredores_ordenados[i].first);
                }
                
                // Verificar disponibilidade após selecionar corredores
                std::map<int, int> itensDisponiveis;
                for (int corredorId : solucao.corredores) {
                    const Corredor& corredor = instancia.corredores[corredorId];
                    for (const auto& [itemId, quantidade] : corredor.itens) {
                        itensDisponiveis[itemId] += quantidade;
                    }
                }
                
                // Selecionar pedidos que podem ser atendidos
                std::map<int, int> itensDemandados;
                int totalItensColetados = 0;
                
                // Ordenar pedidos pelo número de itens (descendente)
                std::vector<std::pair<int, int>> pedidos_ordenados;
                for (int i = 0; i < instancia.numPedidos; ++i) {
                    pedidos_ordenados.push_back({i, instancia.pedidos[i].totalItens});
                }
                
                std::sort(pedidos_ordenados.begin(), pedidos_ordenados.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });
                
                // Selecionar pedidos
                for (const auto& [pedidoId, numItens] : pedidos_ordenados) {
                    if (totalItensColetados >= instancia.UB) break;
                    
                    const Pedido& pedido = instancia.pedidos[pedidoId];
                    bool podeAdicionar = true;
                    
                    // Verificar se todos os itens estão disponíveis
                    for (const auto& [itemId, quantidade] : pedido.itens) {
                        int novaQuantidadeDemandada = itensDemandados.count(itemId) ? 
                                                     itensDemandados.at(itemId) + quantidade : 
                                                     quantidade;
                                                     
                        int quantidadeDisponivel = itensDisponiveis.count(itemId) ? 
                                                 itensDisponiveis.at(itemId) : 0;
                                                 
                        if (novaQuantidadeDemandada > quantidadeDisponivel) {
                            podeAdicionar = false;
                            break;
                        }
                    }
                    
                    if (podeAdicionar && totalItensColetados + numItens <= instancia.UB) {
                        solucao.pedidos.push_back(pedidoId);
                        for (const auto& [itemId, quantidade] : pedido.itens) {
                            itensDemandados[itemId] += quantidade;
                        }
                        totalItensColetados += numItens;
                    }
                }
                
                // Verificar se atingimos LB
                if (totalItensColetados < instancia.LB) {
                    std::cout << "Aviso: Não foi possível atingir o Limite Inferior (LB=" << instancia.LB 
                              << ") com os corredores selecionados. Total de itens: " << totalItensColetados << std::endl;
                    // Adicionar mais corredores
                    corrigirSolucao(instancia, solucao);
                }
                
                salvarSolucao(caminho_solucao, solucao);
                
                // Validar solução criada
                ResultadoValidacao resultado = validarSolucao(instancia, solucao);
                std::cout << "\nValidação da solução criada:" << std::endl;
                std::cout << "  - IDs válidos: " << (resultado.ids_validos ? "Sim" : "Não") << std::endl;
                std::cout << "  - LB satisfeito: " << (resultado.lb_ok ? "Sim" : "Não") << std::endl;
                std::cout << "  - UB satisfeito: " << (resultado.ub_ok ? "Sim" : "Não") << std::endl;
                std::cout << "  - Disponibilidade satisfeita: " << (resultado.disponibilidade_ok ? "Sim" : "Não") << std::endl;
                std::cout << "  - Total de itens: " << resultado.totalItensColetados << std::endl;
                std::cout << "  - Corredores visitados: " << resultado.numCorredoresVisitados << std::endl;
                std::cout << "  - Razão: " << resultado.razao << std::endl;
            }
            
            std::cout << "===== Finalizado: " << nome_arquivo << " =====" << std::endl;
        }
    }

    return 0;
}