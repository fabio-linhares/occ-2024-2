#include "menu.h"
#include "verificar_instancias.h"
#include "verificar_estruturas_auxiliares.h"
#include "solucionar_desafio.h"
#include "validar_resultados.h"
#include "desafio_info.h"
#include "formatacao_terminal.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <limits>
#include <sstream>
#include <thread>

using namespace FormatacaoTerminal;

// Renomeada para corresponder à declaração em menu.h
void mostrarMenu() {
    // Limpar a tela para um menu mais limpo
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    
    // Cabeçalho principal com borda mais sofisticada
    std::cout << std::endl;
    std::cout << colorir("╔══════════════════════════════════════════════════════════╗", CIANO) << std::endl;
    std::cout << colorir("║", CIANO) << colorirBold(" Projeto MercadoLivre v2 - SBPO 2025                     ", CIANO) << colorir("║", CIANO) << std::endl;
    std::cout << colorir("║", CIANO) << colorirBold(" Sistema de Otimização de Waves para Processamento      ", CIANO) << colorir("║", CIANO) << std::endl;
    std::cout << colorir("╚══════════════════════════════════════════════════════════╝", CIANO) << std::endl;
    std::cout << std::endl;

    // Menu de opções com cores mais vibrantes e formato retangular
    std::cout << colorir("┌────────────────── MENU PRINCIPAL ──────────────────┐", AZUL) << std::endl;
    std::cout << colorir("│", AZUL) << " " << colorirBold("1.", AMARELO) << " " << colorir("Verificar as instâncias", BRANCO) << std::string(29, ' ') << colorir("│", AZUL) << std::endl;
    std::cout << colorir("│", AZUL) << " " << colorirBold("2.", AMARELO) << " " << colorir("Verificar estruturas auxiliares", BRANCO) << std::string(19, ' ') << colorir("│", AZUL) << std::endl;
    std::cout << colorir("│", AZUL) << " " << colorirBold("3.", AMARELO) << " " << colorir("Solucionar o desafio", BRANCO) << std::string(30, ' ') << colorir("│", AZUL) << std::endl;
    std::cout << colorir("│", AZUL) << " " << colorirBold("4.", AMARELO) << " " << colorir("Validar resultados", BRANCO) << std::string(32, ' ') << colorir("│", AZUL) << std::endl;
    std::cout << colorir("│", AZUL) << " " << colorirBold("5.", AMARELO) << " " << colorir("Exibir informações do desafio", BRANCO) << std::string(22, ' ') << colorir("│", AZUL) << std::endl;
    std::cout << colorir("│", AZUL) << " " << colorirBold("0.", AMARELO) << " " << colorir("Sair", BRANCO) << std::string(46, ' ') << colorir("│", AZUL) << std::endl;
    std::cout << colorir("└────────────────────────────────────────────────────┘", AZUL) << std::endl;
}

// Adicionada a função que estava faltando
void processarEscolhaMenu(int escolha) {
    const std::string DIR_ENTRADA = "data/input";
    const std::string DIR_SAIDA = "data/output";
    
    std::cout << std::endl;
    
    // Executar a opção escolhida
    switch (escolha) {
        case 1: {
            std::cout << cabecalho("VERIFICAÇÃO DE INSTÂNCIAS") << std::endl;
            
            // Verificar o diretório de instâncias
            if (!std::filesystem::exists(DIR_ENTRADA)) {
                std::cout << colorir("Erro: Diretório de instâncias não encontrado!", VERMELHO) << std::endl;
                break;
            }
            
            // Listar arquivos disponíveis
            std::cout << colorir("Instâncias disponíveis:", VERDE) << std::endl;
            std::vector<std::string> arquivos;
            for (const auto& entry : std::filesystem::directory_iterator(DIR_ENTRADA)) {
                if (entry.is_regular_file()) {
                    arquivos.push_back(entry.path().filename().string());
                }
            }
            
            // Exibir arquivos em formato de tabela
            std::cout << separador() << std::endl;
            for (size_t i = 0; i < arquivos.size(); ++i) {
                std::cout << colorir(std::to_string(i+1) + ".", AMARELO) << " " << arquivos[i] << std::endl;
            }
            std::cout << separador() << std::endl;
            
            // Solicitar escolha do usuário
            std::cout << colorir("Digite o número da instância ou 0 para voltar: ", VERDE);
            int escolhaArquivo;
            std::cin >> escolhaArquivo;
            
            if (escolhaArquivo > 0 && escolhaArquivo <= static_cast<int>(arquivos.size())) {
                std::string arquivoSelecionado = DIR_ENTRADA + "/" + arquivos[escolhaArquivo-1];
                std::cout << std::endl;
                verificarInstancias(arquivoSelecionado);
            }
            
            break;
        }
        case 2: {
            std::cout << cabecalho("VERIFICAÇÃO DE ESTRUTURAS AUXILIARES") << std::endl;
            
            // Verificar o diretório de instâncias
            if (!std::filesystem::exists(DIR_ENTRADA)) {
                std::cout << colorir("Erro: Diretório de instâncias não encontrado!", VERMELHO) << std::endl;
                break;
            }
            
            // Listar arquivos disponíveis
            std::cout << colorir("Instâncias disponíveis:", VERDE) << std::endl;
            std::vector<std::string> arquivos;
            for (const auto& entry : std::filesystem::directory_iterator(DIR_ENTRADA)) {
                if (entry.is_regular_file()) {
                    arquivos.push_back(entry.path().filename().string());
                }
            }
            
            // Exibir arquivos em formato de tabela
            std::cout << separador() << std::endl;
            for (size_t i = 0; i < arquivos.size(); ++i) {
                std::cout << colorir(std::to_string(i+1) + ".", AMARELO) << " " << arquivos[i] << std::endl;
            }
            std::cout << separador() << std::endl;
            
            // Solicitar escolha do usuário
            std::cout << colorir("Digite o número da instância ou 0 para voltar: ", VERDE);
            int escolhaArquivo;
            std::cin >> escolhaArquivo;
            
            if (escolhaArquivo > 0 && escolhaArquivo <= static_cast<int>(arquivos.size())) {
                std::string arquivoSelecionado = DIR_ENTRADA + "/" + arquivos[escolhaArquivo-1];
                std::cout << std::endl;
                verificarEstruturasAuxiliares(arquivoSelecionado);
            }
            
            break;
        }
        case 3: {
            std::cout << cabecalho("SOLUÇÃO DO DESAFIO") << std::endl;
            
            // Diretórios padrão
            std::string dirEntrada = "data/input";
            std::string dirSaida = "data/output";
            
            // Perguntar ao usuário se deseja personalizar os diretórios
            std::string resposta;
            std::cout << "Usar diretórios padrão? (s/n): ";
            std::cin >> resposta;
            
            if (resposta[0] == 'n' || resposta[0] == 'N') {
                std::cout << "Digite o diretório de entrada: ";
                std::cin >> dirEntrada;
                std::cout << "Digite o diretório de saída: ";
                std::cin >> dirSaida;
            }
            
            // Chamar a função principal
            solucionarDesafio(dirEntrada, dirSaida);
            break;
        }
        case 4:
            validarResultados(DIR_ENTRADA, DIR_SAIDA);
            break;
        case 5:
            exibirInformacoesDesafio();
            break;
        case 0:
            std::cout << colorirBold("Obrigado por usar o Sistema de Otimização de Waves!", VERDE) << std::endl;
            break;
        default:
            std::cout << colorir("Opção inválida. Por favor, tente novamente.", VERMELHO) << std::endl;
    }
    
    if (escolha != 0) {
        std::cout << std::endl;
        std::cout << colorir("Pressione ENTER para continuar...", BRANCO);
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
        std::cout << std::endl;
    }
}