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

/*
Validador Completo com Geração de Relatórios e Análise Comparativa

Este programa:
1. Valida soluções existentes sem fazer correções
2. Gera relatórios detalhados de cada validação
3. Armazena dados incrementalmente para análise histórica
4. Permite comparar resultados entre diferentes execuções

Principais Funcionalidades Implementadas
Validação Completa das Soluções:

Verifica todas as restrições (LB, UB, disponibilidade, IDs válidos)
Imprime resultados detalhados para cada instância
Geração de Relatórios Detalhados:

Para cada instância validada, gera um relatório em texto detalhado
Inclui análise da solução e recomendações
Armazenamento Incremental de Dados:

Salva os resultados de cada validação em um arquivo CSV
Formato: timestamp, instância, razão, total de itens, corredores, status das restrições
Script de Análise e Visualização:

Gera um script Python que analisa os dados históricos de validações
Cria diversos gráficos comparativos e de evolução temporal
Visualizações Comparativas:

Evolução da razão por instância ao longo do tempo
Comparação da conformidade com restrições entre execuções
Dashboard completo com múltiplas métricas
Relatório textual de resumo
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

struct ResultadoValidacao {
    bool lb_ok = false;
    bool ub_ok = false;
    bool disponibilidade_ok = false;
    bool ids_validos = true;
    std::vector<int> ids_invalidos;
    int totalItensColetados = 0;
    int numCorredoresVisitados = 0;
    double razao = 0.0;
    std::string timestamp; // Quando a validação foi executada
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

// Função para validar uma solução
ResultadoValidacao validarSolucao(const Instancia& instancia, const Solucao& solucao) {
    ResultadoValidacao resultado;
    
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

// Função para gerar relatório detalhado de uma validação
void gerarRelatorio(const std::string& instancia_nome, const ResultadoValidacao& resultado, 
                   const Instancia& instancia, const std::string& diretorio_relatorios) {
    // Criar diretório de relatórios se não existir
    std::filesystem::create_directories(diretorio_relatorios);
    
    // Nome do arquivo de relatório baseado na instância e timestamp
    std::string timestamp_formato = resultado.timestamp;
    std::replace(timestamp_formato.begin(), timestamp_formato.end(), ' ', '_');
    std::replace(timestamp_formato.begin(), timestamp_formato.end(), ':', '-');
    
    std::string nome_arquivo = diretorio_relatorios + "/" + instancia_nome + "_" + timestamp_formato + ".txt";
    
    // Criar arquivo de relatório
    std::ofstream arquivo(nome_arquivo);
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao criar arquivo de relatório: " << nome_arquivo << std::endl;
        return;
    }
    
    // Cabeçalho do relatório
    arquivo << "======================================================" << std::endl;
    arquivo << "RELATÓRIO DE VALIDAÇÃO: " << instancia_nome << std::endl;
    arquivo << "Data/Hora: " << resultado.timestamp << std::endl;
    arquivo << "======================================================" << std::endl << std::endl;
    
    // Resumo da validação
    arquivo << "RESUMO DA VALIDAÇÃO:" << std::endl;
    arquivo << "-------------------" << std::endl;
    arquivo << "Razão Itens/Corredores: " << std::fixed << std::setprecision(5) << resultado.razao << std::endl;
    arquivo << "Total de Itens Coletados: " << resultado.totalItensColetados << std::endl;
    arquivo << "Número de Corredores Visitados: " << resultado.numCorredoresVisitados << std::endl;
    arquivo << "Limites (LB, UB): (" << instancia.LB << ", " << instancia.UB << ")" << std::endl << std::endl;
    
    // Status das restrições
    arquivo << "STATUS DAS RESTRIÇÕES:" << std::endl;
    arquivo << "---------------------" << std::endl;
    arquivo << "Limite Inferior (LB): " << (resultado.lb_ok ? "OK" : "FALHA") << std::endl;
    arquivo << "Limite Superior (UB): " << (resultado.ub_ok ? "OK" : "FALHA") << std::endl;
    arquivo << "Disponibilidade de Itens: " << (resultado.disponibilidade_ok ? "OK" : "FALHA") << std::endl;
    arquivo << "IDs Válidos: " << (resultado.ids_validos ? "OK" : "FALHA") << std::endl << std::endl;
    
    // Detalhes de IDs inválidos, se houver
    if (!resultado.ids_validos && !resultado.ids_invalidos.empty()) {
        arquivo << "IDs INVÁLIDOS DETECTADOS:" << std::endl;
        arquivo << "------------------------" << std::endl;
        
        for (int id : resultado.ids_invalidos) {
            arquivo << "ID: " << id << std::endl;
        }
        arquivo << std::endl;
    }
    
    // Análise de validação
    arquivo << "ANÁLISE DA VALIDAÇÃO:" << std::endl;
    arquivo << "--------------------" << std::endl;
    
    // Analisar razão
    if (resultado.razao > 5.0) {
        arquivo << "Razão muito boa (>5.0): Excelente eficiência na coleta." << std::endl;
    } else if (resultado.razao > 3.0) {
        arquivo << "Razão boa (3.0-5.0): Boa eficiência na coleta." << std::endl;
    } else if (resultado.razao > 1.0) {
        arquivo << "Razão razoável (1.0-3.0): Eficiência média na coleta." << std::endl;
    } else {
        arquivo << "Razão baixa (<1.0): Baixa eficiência na coleta." << std::endl;
    }
    
    // Analisar limites
    if (!resultado.lb_ok) {
        arquivo << "Limite Inferior não satisfeito: " << resultado.totalItensColetados << " < " << instancia.LB << std::endl;
    }
    if (!resultado.ub_ok) {
        arquivo << "Limite Superior não satisfeito: " << resultado.totalItensColetados << " > " << instancia.UB << std::endl;
    }
    
    // Analisar disponibilidade
    if (!resultado.disponibilidade_ok) {
        arquivo << "Problema de disponibilidade de itens: Alguns itens demandados não estão disponíveis em quantidade suficiente nos corredores selecionados." << std::endl;
    }
    
    // Conclusão
    arquivo << std::endl << "CONCLUSÃO:" << std::endl;
    arquivo << "---------" << std::endl;
    
    if (resultado.lb_ok && resultado.ub_ok && resultado.disponibilidade_ok && resultado.ids_validos) {
        arquivo << "A solução é VÁLIDA e atende a todas as restrições do problema." << std::endl;
    } else {
        arquivo << "A solução é INVÁLIDA e viola pelo menos uma das restrições do problema." << std::endl;
    }
    
    arquivo.close();
    
    std::cout << "Relatório detalhado gerado: " << nome_arquivo << std::endl;
}

// Função para salvar dados de validação em arquivo CSV incremental
void salvarDadosIncrementais(const std::string& instancia_nome, const ResultadoValidacao& resultado, 
                           const Instancia& instancia, const std::string& arquivo_csv) {
    bool arquivo_existe = std::filesystem::exists(arquivo_csv);
    
    std::ofstream csv;
    if (arquivo_existe) {
        csv.open(arquivo_csv, std::ios_base::app); // Modo append
    } else {
        csv.open(arquivo_csv);
        // Escrever cabeçalho
        csv << "Timestamp,Instancia,Razao,TotalItens,NumCorredores,LB,UB,LB_OK,UB_OK,Disponibilidade_OK,IDs_Validos" << std::endl;
    }
    
    if (!csv.is_open()) {
        std::cerr << "Erro ao abrir arquivo CSV: " << arquivo_csv << std::endl;
        return;
    }
    
    // Escrever dados da validação atual
    csv << resultado.timestamp << "," 
        << instancia_nome << "," 
        << std::fixed << std::setprecision(5) << resultado.razao << "," 
        << resultado.totalItensColetados << "," 
        << resultado.numCorredoresVisitados << "," 
        << instancia.LB << "," 
        << instancia.UB << "," 
        << (resultado.lb_ok ? "1" : "0") << "," 
        << (resultado.ub_ok ? "1" : "0") << "," 
        << (resultado.disponibilidade_ok ? "1" : "0") << "," 
        << (resultado.ids_validos ? "1" : "0") 
        << std::endl;
    
    csv.close();
    
    if (!arquivo_existe) {
        std::cout << "Arquivo CSV criado: " << arquivo_csv << std::endl;
    } else {
        std::cout << "Dados adicionados ao arquivo CSV: " << arquivo_csv << std::endl;
    }
}

// Função para gerar o script Python que fará a análise e visualização dos dados
void gerarScriptPython(const std::string& arquivo_csv, const std::string& diretorio_graficos) {
    std::filesystem::create_directories(diretorio_graficos);
    
    std::string arquivo_script = "analise_validacoes.py";
    std::ofstream script(arquivo_script);
    
    if (!script.is_open()) {
        std::cerr << "Erro ao criar script Python: " << arquivo_script << std::endl;
        return;
    }
    
    script << R"(
#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os
from datetime import datetime

# Configurações
plt.style.use('ggplot')
DIRETORIO_GRAFICOS = ')" << diretorio_graficos << R"('
ARQUIVO_CSV = ')" << arquivo_csv << R"('

# Criar diretório para gráficos se não existir
os.makedirs(DIRETORIO_GRAFICOS, exist_ok=True)

# Carregar dados
print(f"Carregando dados de: {ARQUIVO_CSV}")
df = pd.read_csv(ARQUIVO_CSV)

# Converter timestamp para datetime
df['Timestamp'] = pd.to_datetime(df['Timestamp'])

# Ordenar por timestamp
df = df.sort_values('Timestamp')

# Adicionar coluna de execução (número sequencial por data)
df['Execucao'] = df.groupby(df['Timestamp'].dt.date)['Timestamp'].rank()

# Adicionar coluna de data formatada para facilitar plotagem
df['Data'] = df['Timestamp'].dt.strftime('%Y-%m-%d')

# Adicionar coluna de solução válida
df['Solucao_Valida'] = (df['LB_OK'] & df['UB_OK'] & df['Disponibilidade_OK'] & df['IDs_Validos']).astype(int)

# 1. Gráfico de evolução da razão por instância
def plot_evolucao_razao():
    plt.figure(figsize=(14, 8))
    
    # Selecionar top 10 instâncias com mais execuções
    top_instancias = df['Instancia'].value_counts().head(10).index.tolist()
    df_plot = df[df['Instancia'].isin(top_instancias)]
    
    sns.lineplot(data=df_plot, x='Timestamp', y='Razao', hue='Instancia', marker='o')
    
    plt.title('Evolução da Razão por Instância ao Longo do Tempo', fontsize=16)
    plt.xlabel('Data da Validação', fontsize=12)
    plt.ylabel('Razão (Itens/Corredores)', fontsize=12)
    plt.xticks(rotation=45)
    plt.grid(True, alpha=0.3)
    plt.legend(title='Instância', bbox_to_anchor=(1.05, 1), loc='upper left')
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'evolucao_razao.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: evolucao_razao.png")

# 2. Gráfico de barras comparando razão média por data
def plot_razao_media_por_data():
    plt.figure(figsize=(12, 6))
    
    # Calcular média da razão por data
    df_por_data = df.groupby('Data')['Razao'].mean().reset_index()
    
    # Plot
    ax = sns.barplot(data=df_por_data, x='Data', y='Razao')
    
    # Adicionar rótulos nas barras
    for i, bar in enumerate(ax.patches):
        ax.text(
            bar.get_x() + bar.get_width()/2., 
            bar.get_height() + 0.1, 
            f'{bar.get_height():.2f}', 
            ha='center', va='bottom'
        )
    
    plt.title('Razão Média por Data de Validação', fontsize=16)
    plt.xlabel('Data', fontsize=12)
    plt.ylabel('Razão Média', fontsize=12)
    plt.xticks(rotation=45)
    plt.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'razao_media_por_data.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: razao_media_por_data.png")

# 3. Gráfico de validação das restrições ao longo do tempo
def plot_restricoes_tempo():
    plt.figure(figsize=(14, 8))
    
    # Preparar dados para plotagem
    df_plot = df.groupby('Data')[['LB_OK', 'UB_OK', 'Disponibilidade_OK', 'IDs_Validos', 'Solucao_Valida']].mean()
    df_plot = df_plot * 100  # Converter para porcentagem
    
    # Plot
    ax = df_plot.plot(kind='bar', figsize=(14, 8))
    
    plt.title('Percentual de Conformidade por Restrição ao Longo do Tempo', fontsize=16)
    plt.xlabel('Data', fontsize=12)
    plt.ylabel('Porcentagem de Conformidade (%)', fontsize=12)
    plt.ylim(0, 105)
    plt.grid(axis='y', alpha=0.3)
    plt.xticks(rotation=45)
    plt.legend(title='Restrição', labels=[
        'Limite Inferior', 
        'Limite Superior', 
        'Disponibilidade de Itens', 
        'IDs Válidos',
        'Solução Completamente Válida'
    ])
    
    # Adicionar rótulos de porcentagem
    for i, p in enumerate(ax.patches):
        ax.annotate(
            f'{p.get_height():.1f}%', 
            (p.get_x() + p.get_width()/2., p.get_height() + 1), 
            ha='center', va='bottom'
        )
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'restricoes_tempo.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: restricoes_tempo.png")

# 4. Gráfico de dispersão: Razão vs. Número de Corredores
def plot_razao_vs_corredores():
    plt.figure(figsize=(12, 8))
    
    sns.scatterplot(
        data=df, 
        x='NumCorredores', 
        y='Razao', 
        hue='Instancia',
        size='TotalItens',
        sizes=(50, 300),
        alpha=0.7
    )
    
    plt.title('Relação entre Razão e Número de Corredores Visitados', fontsize=16)
    plt.xlabel('Número de Corredores', fontsize=12)
    plt.ylabel('Razão (Itens/Corredores)', fontsize=12)
    plt.grid(True, alpha=0.3)
    
    # Adicionar linha de tendência
    x = df['NumCorredores']
    y = df['Razao']
    
    if len(x) > 1:
        z = np.polyfit(x, y, 1)
        p = np.poly1d(z)
        plt.plot(x, p(x), "r--", alpha=0.7)
        
        # Adicionar equação da linha de tendência
        plt.text(
            0.05, 0.95, 
            f'y = {z[0]:.4f}x + {z[1]:.4f}', 
            transform=plt.gca().transAxes,
            bbox=dict(facecolor='white', alpha=0.7)
        )
    
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'razao_vs_corredores.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: razao_vs_corredores.png")

# 5. Gráfico de heatmap mostrando a correlação entre as métricas
def plot_correlacao_metricas():
    # Selecionar apenas colunas numéricas
    colunas_numericas = ['Razao', 'TotalItens', 'NumCorredores', 'LB', 'UB', 
                         'LB_OK', 'UB_OK', 'Disponibilidade_OK', 'IDs_Validos', 'Solucao_Valida']
    df_corr = df[colunas_numericas].corr()
    
    plt.figure(figsize=(10, 8))
    sns.heatmap(df_corr, annot=True, cmap='coolwarm', fmt='.2f', linewidths=0.5)
    
    plt.title('Correlação entre Métricas', fontsize=16)
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'correlacao_metricas.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: correlacao_metricas.png")

# 6. Dashboard com múltiplos gráficos
def plot_dashboard():
    fig = plt.figure(figsize=(15, 12))
    fig.suptitle('Dashboard de Análise de Validações', fontsize=18, y=0.98)
    
    # Grid layout
    gs = fig.add_gridspec(3, 3)
    
    # 1. Evolução da razão média ao longo do tempo
    ax1 = fig.add_subplot(gs[0, :])
    df_evolucao = df.groupby('Data')['Razao'].mean().reset_index()
    sns.lineplot(data=df_evolucao, x='Data', y='Razao', marker='o', color='blue', ax=ax1)
    ax1.set_title('Evolução da Razão Média ao Longo do Tempo')
    ax1.set_ylabel('Razão Média')
    ax1.set_xlabel('Data')
    ax1.grid(True, alpha=0.3)
    ax1.tick_params(axis='x', rotation=45)
    
    # 2. Distribuição de razões
    ax2 = fig.add_subplot(gs[1, 0])
    sns.histplot(data=df, x='Razao', kde=True, ax=ax2)
    ax2.set_title('Distribuição das Razões')
    ax2.set_xlabel('Razão')
    ax2.set_ylabel('Frequência')
    
    # 3. Proporção de restrições atendidas
    ax3 = fig.add_subplot(gs[1, 1])
    df_restricoes = pd.DataFrame({
        'Restrição': ['LB', 'UB', 'Disponibilidade', 'IDs', 'Solução Completa'],
        'Conformidade (%)': [
            df['LB_OK'].mean() * 100,
            df['UB_OK'].mean() * 100,
            df['Disponibilidade_OK'].mean() * 100,
            df['IDs_Validos'].mean() * 100,
            df['Solucao_Valida'].mean() * 100
        ]
    })
    sns.barplot(data=df_restricoes, x='Restrição', y='Conformidade (%)', ax=ax3)
    ax3.set_title('Conformidade por Restrição')
    ax3.set_xlabel('')
    ax3.set_ylim(0, 105)
    ax3.tick_params(axis='x', rotation=45)
    
    # Adicionar rótulos nas barras
    for i, p in enumerate(ax3.patches):
        ax3.annotate(
            f'{p.get_height():.1f}%', 
            (p.get_x() + p.get_width()/2., p.get_height() + 1), 
            ha='center', va='bottom'
        )
    
    # 4. Top instâncias por razão média
    ax4 = fig.add_subplot(gs[1, 2])
    df_top = df.groupby('Instancia')['Razao'].mean().reset_index()
    df_top = df_top.sort_values('Razao', ascending=False).head(10)
    sns.barplot(data=df_top, x='Razao', y='Instancia', ax=ax4)
    ax4.set_title('Top 10 Instâncias por Razão Média')
    ax4.set_xlabel('Razão Média')
    ax4.set_ylabel('')
    
    # 5. Dispersão: Itens vs. Corredores
    ax5 = fig.add_subplot(gs[2, :])
    scatter = ax5.scatter(
        df['NumCorredores'], 
        df['TotalItens'], 
        c=df['Razao'], 
        s=df['Razao']*20, 
        alpha=0.7, 
        cmap='viridis'
    )
    ax5.set_title('Relação entre Total de Itens e Número de Corredores')
    ax5.set_xlabel('Número de Corredores')
    ax5.set_ylabel('Total de Itens')
    ax5.grid(True, alpha=0.3)
    
    # Adicionar barra de cores
    cbar = plt.colorbar(scatter, ax=ax5)
    cbar.set_label('Razão')
    
    plt.tight_layout()
    plt.savefig(os.path.join(DIRETORIO_GRAFICOS, 'dashboard_completo.png'), dpi=300)
    plt.close()
    print("Gráfico salvo: dashboard_completo.png")

# 7. Relatório resumo
def gerar_relatorio_resumo():
    # Criar arquivo de relatório
    relatorio_path = os.path.join(DIRETORIO_GRAFICOS, 'relatorio_resumo.txt')
    
    with open(relatorio_path, 'w') as f:
        f.write("RELATÓRIO RESUMO DE VALIDAÇÕES\n")
        f.write("==============================\n\n")
        
        # Estatísticas gerais
        f.write(f"Total de validações: {len(df)}\n")
        f.write(f"Período: {df['Timestamp'].min().strftime('%Y-%m-%d')} a {df['Timestamp'].max().strftime('%Y-%m-%d')}\n")
        f.write(f"Instâncias distintas: {df['Instancia'].nunique()}\n\n")
        
        # Conformidade
        f.write("CONFORMIDADE COM RESTRIÇÕES:\n")
        f.write("--------------------------\n")
        f.write(f"Limite Inferior (LB): {df['LB_OK'].mean()*100:.2f}%\n")
        f.write(f"Limite Superior (UB): {df['UB_OK'].mean()*100:.2f}%\n")
        f.write(f"Disponibilidade de Itens: {df['Disponibilidade_OK'].mean()*100:.2f}%\n")
        f.write(f"IDs Válidos: {df['IDs_Validos'].mean()*100:.2f}%\n")
        f.write(f"Soluções Completamente Válidas: {df['Solucao_Valida'].mean()*100:.2f}%\n\n")
        
        # Estatísticas de razão
        f.write("ESTATÍSTICAS DE RAZÃO:\n")
        f.write("---------------------\n")
        f.write(f"Razão média global: {df['Razao'].mean():.4f}\n")
        f.write(f"Razão máxima: {df['Razao'].max():.4f}\n")
        f.write(f"Razão mínima: {df['Razao'].min():.4f}\n")
        f.write(f"Desvio padrão da razão: {df['Razao'].std():.4f}\n\n")
        
        # Melhores instâncias por razão
        f.write("TOP 5 INSTÂNCIAS POR RAZÃO MÉDIA:\n")
        f.write("--------------------------------\n")
        top_instancias = df.groupby('Instancia')['Razao'].mean().sort_values(ascending=False).head(5)
        for i, (instancia, razao) in enumerate(top_instancias.items(), 1):
            f.write(f"{i}. {instancia}: {razao:.4f}\n")
        
        f.write("\n")
        
        # Evolução da solução
        f.write("EVOLUÇÃO DA SOLUÇÃO:\n")
        f.write("-------------------\n")
        
        # Verificar tendência nas últimas validações
        ultimas_data = df.groupby('Data')['Razao'].mean().reset_index()
        if len(ultimas_data) >= 2:
            primeira_razao = ultimas_data.iloc[0]['Razao']
            ultima_razao = ultimas_data.iloc[-1]['Razao']
            variacao = ((ultima_razao - primeira_razao) / primeira_razao) * 100
            
            f.write(f"Razão média na primeira data: {primeira_razao:.4f}\n")
            f.write(f"Razão média na última data: {ultima_razao:.4f}\n")
            f.write(f"Variação percentual: {variacao:.2f}%\n")
            
            if variacao > 0:
                f.write(f"Tendência: MELHORIA de {variacao:.2f}% na razão média\n")
            elif variacao < 0:
                f.write(f"Tendência: PIORA de {abs(variacao):.2f}% na razão média\n")
            else:
                f.write("Tendência: ESTÁVEL (sem variação significativa)\n")
    
    print(f"Relatório resumo gerado: {relatorio_path}")

# Executar todas as funções de visualização
if __name__ == "__main__":
    # Verificar se há dados suficientes
    if len(df) < 2:
        print("Não há dados suficientes para gerar visualizações. Execute mais validações.")
    else:
        # Criar gráficos
        plot_evolucao_razao()
        plot_razao_media_por_data()
        plot_restricoes_tempo()
        plot_razao_vs_corredores()
        plot_correlacao_metricas()
        plot_dashboard()
        gerar_relatorio_resumo()
        
        print(f"\nTodos os gráficos foram salvos no diretório: {DIRETORIO_GRAFICOS}")
        print(f"Total de registros analisados: {len(df)}")
    )" << std::endl;
    
    script.close();
    
    // Tornar o script executável (apenas em sistemas Unix)
    #ifdef __unix__
    system(("chmod +x " + arquivo_script).c_str());
    #endif
    
    std::cout << "Script Python gerado: " << arquivo_script << std::endl;
    std::cout << "Execute-o com 'python3 " << arquivo_script << "' para gerar visualizações." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <pasta_instancias> <pasta_saida>" << std::endl;
        return 1;
    }

    std::string pasta_instancias = argv[1];
    std::string pasta_saida = argv[2];
    
    // Diretórios para relatórios e gráficos
    std::string diretorio_relatorios = "relatorios";
    std::string diretorio_graficos = "graficos";
    std::string arquivo_csv = "historico_validacoes.csv";
    
    std::filesystem::create_directories(diretorio_relatorios);
    
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
            
            // Mostrar resultados na tela
            std::cout << "  - Razao: " << std::fixed << std::setprecision(5) << resultado.razao << std::endl;
            std::cout << "  - Limite Inferior: " << (resultado.lb_ok ? "OK" : "FALHA") << std::endl;
            std::cout << "  - Limite Superior: " << (resultado.ub_ok ? "OK" : "FALHA") << std::endl;
            std::cout << "  - Disponibilidade de Itens: " << (resultado.disponibilidade_ok ? "OK" : "FALHA") << std::endl;
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
            
            // Gerar relatório detalhado
            gerarRelatorio(nome_instancia, resultado, instancia, diretorio_relatorios);
            
            // Salvar dados para análise comparativa
            salvarDadosIncrementais(nome_instancia, resultado, instancia, arquivo_csv);
        }
    }
    
    // Gerar script Python para análise dos dados
    gerarScriptPython(arquivo_csv, diretorio_graficos);
    
    return 0;
}