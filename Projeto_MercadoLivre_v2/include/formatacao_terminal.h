#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * @brief Namespace com funções para formatação de texto no terminal
 */
namespace FormatacaoTerminal {
    // Códigos ANSI para cores
    const std::string RESET      = "\033[0m";
    const std::string BOLD       = "\033[1m";
    const std::string UNDERLINE  = "\033[4m";
    
    // Cores de texto
    const std::string VERMELHO   = "\033[31m";
    const std::string VERDE      = "\033[32m";
    const std::string AMARELO    = "\033[33m";
    const std::string AZUL       = "\033[34m";
    const std::string MAGENTA    = "\033[35m";
    const std::string CIANO      = "\033[36m";
    const std::string BRANCO     = "\033[37m";
    
    // Cores de fundo
    const std::string BG_VERMELHO = "\033[41m";
    const std::string BG_VERDE    = "\033[42m";
    const std::string BG_AMARELO  = "\033[43m";
    const std::string BG_AZUL     = "\033[44m";
    const std::string BG_MAGENTA  = "\033[45m";
    const std::string BG_CIANO    = "\033[46m";
    const std::string BG_BRANCO   = "\033[47m";
    
    // Caracteres de borda como strings - evita warnings de caracteres multi-byte
    const std::string BORDA_ES = "┌";  // Canto superior esquerdo
    const std::string BORDA_DS = "└";  // Canto inferior esquerdo
    const std::string BORDA_SD = "┐";  // Canto superior direito
    const std::string BORDA_ID = "┘";  // Canto inferior direito
    const std::string BORDA_EJ = "├";  // Junção esquerda
    const std::string BORDA_DJ = "┤";  // Junção direita
    const std::string BORDA_H = "─";   // Linha horizontal
    const std::string BORDA_V = "│";   // Linha vertical
    
    // Caracteres alternativos ASCII se necessário
    const std::string ALT_BORDA_ES = "+";  // Canto superior esquerdo
    const std::string ALT_BORDA_DS = "+";  // Canto inferior esquerdo
    const std::string ALT_BORDA_SD = "+";  // Canto superior direito
    const std::string ALT_BORDA_ID = "+";  // Canto inferior direito
    const std::string ALT_BORDA_EJ = "+";  // Junção esquerda
    const std::string ALT_BORDA_DJ = "+";  // Junção direita
    const std::string ALT_BORDA_H = "-";   // Linha horizontal
    const std::string ALT_BORDA_V = "|";   // Linha vertical
    
    /**
     * @brief Formata texto com cor
     * @param texto Texto a ser formatado
     * @param cor Código de cor ANSI
     * @return Texto formatado
     */
    inline std::string colorir(const std::string& texto, const std::string& cor) {
        return cor + texto + RESET;
    }
    
    /**
     * @brief Formata texto com cor e negrito
     * @param texto Texto a ser formatado
     * @param cor Código de cor ANSI
     * @return Texto formatado
     */
    inline std::string colorirBold(const std::string& texto, const std::string& cor) {
        return cor + BOLD + texto + RESET;
    }
    
    /**
     * @brief Cria um separador horizontal
     * @param caractere Caractere usado no separador
     * @param largura Largura do separador
     * @return String com o separador formatado
     */
    inline std::string separador(const std::string& caractere = "═", int largura = 60) {
        std::string resultado;
        for (int i = 0; i < largura; i++) {
            resultado += caractere;
        }
        return colorir(resultado, AZUL);
    }
    
    /**
     * @brief Formata um cabeçalho de seção
     * @param texto Texto do cabeçalho
     * @return Texto formatado como cabeçalho
     */
    inline std::string cabecalho(const std::string& texto) {
        std::string linha = separador();
        return "\n" + linha + "\n" + 
               colorirBold(" " + texto + " ", AZUL) + "\n" + 
               linha + "\n";
    }
    
    /**
     * @brief Formata um título de instância
     * @param nomeArquivo Nome do arquivo de instância
     * @return Texto formatado como título de instância
     */
    inline std::string tituloInstancia(const std::string& nomeArquivo) {
        std::string linha = separador();
        return "\n" + linha + "\n" + 
               colorirBold("▶ Processando instância: ", VERDE) + 
               colorirBold(nomeArquivo, AMARELO) + "\n" + 
               linha + "\n";
    }
    
    /**
     * @brief Formata informações da instância em formato tabular
     * @param numPedidos Número de pedidos
     * @param numItens Número de itens
     * @param numCorredores Número de corredores
     * @return Texto formatado com informações da instância
     */
    inline std::string infoInstancia(int numPedidos, int numItens, int numCorredores) {
        std::stringstream ss;
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┐\n";
        ss << "│ " << colorirBold("DETALHES DA INSTÂNCIA", CIANO) << std::string(36, ' ') << "│\n";
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┤\n";
        
        ss << "│ " << colorir("• Pedidos:    ", VERDE) 
           << std::setw(7) << std::right << numPedidos 
           << std::string(33, ' ') << "│\n";
        
        ss << "│ " << colorir("• Itens:      ", VERDE) 
           << std::setw(7) << std::right << numItens 
           << std::string(33, ' ') << "│\n";
        
        ss << "│ " << colorir("• Corredores: ", VERDE) 
           << std::setw(7) << std::right << numCorredores 
           << std::string(33, ' ') << "│\n";
        
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┘";
        
        return ss.str();
    }
    
    /**
     * @brief Formata informações de limites em formato tabular
     * @param LB Limite inferior
     * @param UB Limite superior
     * @return Texto formatado com informações de limites
     */
    inline std::string infoLimites(int LB, int UB) {
        std::stringstream ss;
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┐\n";
        ss << "│ " << colorirBold("LIMITES DA INSTÂNCIA", MAGENTA) << std::string(36, ' ') << "│\n";
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┤\n";
        
        ss << "│ " << colorir("• Limite Inferior (LB): ", BRANCO) 
           << colorirBold(std::to_string(LB), VERDE) 
           << std::string(29 - std::to_string(LB).length(), ' ') << "│\n";
        
        ss << "│ " << colorir("• Limite Superior (UB): ", BRANCO) 
           << colorirBold(std::to_string(UB), VERMELHO) 
           << std::string(29 - std::to_string(UB).length(), ' ') << "│\n";
        
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┘";
        
        return ss.str();
    }
    
    /**
     * @brief Formata informação de otimizador
     * @param nomeOtimizador Nome do otimizador
     * @return Texto formatado com informação do otimizador
     */
    inline std::string infoOtimizador(const std::string& nomeOtimizador) {
        std::stringstream ss;
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┐\n";
        ss << "│ " << colorirBold("MÉTODO DE OTIMIZAÇÃO", AMARELO) << std::string(38, ' ') << "│\n";
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┤\n";
        
        ss << "│ " << colorir("• Algoritmo: ", BRANCO) 
           << colorirBold(nomeOtimizador, CIANO)
           << std::string(45 - nomeOtimizador.length(), ' ') << "│\n";
        
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┘";
        
        return ss.str();
    }
    
    /**
     * @brief Formata informação de resultado em formato tabular
     * @param arquivoSaida Caminho do arquivo de saída
     * @param bov Valor objetivo
     * @param tempoExecucao Tempo de execução em segundos
     * @return Texto formatado com informação de resultado
     */
    inline std::string infoResultado(const std::string& arquivoSaida, double bov, double tempoExecucao) {
        std::stringstream ss;
        // Formatar BOV com precisão fixa
        std::stringstream bovStr;
        bovStr << std::fixed << std::setprecision(6) << bov;
        
        // Formatar tempo com precisão fixa
        std::stringstream tempoStr;
        tempoStr << std::fixed << std::setprecision(3) << tempoExecucao;
        
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┐\n";
        ss << "│ " << colorirBold("RESULTADOS", VERDE) << std::string(48, ' ') << "│\n";
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┤\n";
        
        ss << "│ " << colorir("✓ Arquivo de saída: ", VERDE) << std::endl;
        ss << "│   " << arquivoSaida << std::string(55 - arquivoSaida.length(), ' ') << "│\n";
        
        ss << "│ " << colorir("✓ BOV: ", VERDE) 
           << colorirBold(bovStr.str(), AZUL)
           << std::string(52 - bovStr.str().length(), ' ') << "│\n";
        
        ss << "│ " << colorir("✓ Tempo: ", VERDE) 
           << colorirBold(tempoStr.str() + " s", CIANO)
           << std::string(50 - (tempoStr.str().length() + 2), ' ') << "│\n";
        
        for (int i = 0; i < 58; i++) ss << "─";
        ss << "┘";
        
        return ss.str();
    }
    
    /**
     * @brief Formata uma mensagem de status
     * @param mensagem Texto da mensagem
     * @return Texto formatado como mensagem de status
     */
    inline std::string status(const std::string& mensagem) {
        return colorir("» " + mensagem, BRANCO);
    }
    
    /**
     * @brief Formata uma mensagem de sucesso
     * @param mensagem Texto da mensagem
     * @return Texto formatado como mensagem de sucesso
     */
    inline std::string sucesso(const std::string& mensagem) {
        return colorir("✓ " + mensagem, VERDE);
    }
    
    /**
     * @brief Formata uma mensagem de erro
     * @param mensagem Texto da mensagem
     * @return Texto formatado como mensagem de erro
     */
    inline std::string erro(const std::string& mensagem) {
        return colorir("❌ " + mensagem, VERMELHO);
    }
    
    /**
     * @brief Formata uma barra de progresso
     * @param percentual Percentual de conclusão (0-100)
     * @param largura Largura total da barra
     * @return Texto formatado como barra de progresso
     */
    inline std::string barraProgresso(int percentual, int largura = 40) {
        int preenchido = percentual * largura / 100;
        
        std::stringstream ss;
        ss << "[";
        for (int i = 0; i < largura; ++i) {
            if (i < preenchido) {
                ss << colorir("█", VERDE);
            } else {
                ss << colorir("░", BRANCO);
            }
        }
        ss << "] " << percentual << "%";
        
        return ss.str();
    }
    
    // Função para criar linhas horizontais
    inline std::string linhaHorizontal(int comprimento, bool unicode = true) {
        std::string borda = unicode ? BORDA_H : ALT_BORDA_H;
        std::string resultado;
        for (int i = 0; i < comprimento; ++i) {
            resultado += borda;
        }
        return resultado;
    }
    
    // Função para criar caixas completas
    inline std::string criarCaixaSimples(const std::string& titulo, int largura = 56, bool unicode = true) {
        std::string se = unicode ? BORDA_ES : ALT_BORDA_ES;
        std::string sd = unicode ? BORDA_SD : ALT_BORDA_SD;
        std::string ie = unicode ? BORDA_DS : ALT_BORDA_DS;
        std::string id = unicode ? BORDA_ID : ALT_BORDA_ID;
        std::string v = unicode ? BORDA_V : ALT_BORDA_V;
        
        std::stringstream ss;
        ss << se << linhaHorizontal(largura - 2, unicode) << sd << "\n";
        ss << v << " " << titulo << std::string(largura - 3 - titulo.length(), ' ') << v << "\n";
        ss << ie << linhaHorizontal(largura - 2, unicode) << id;
        return ss.str();
    }
    
    // Função para criar cabeçalho de caixa
    inline std::string criarCabecalhoCaixa(const std::string& titulo, int largura = 56, bool unicode = true) {
        std::string se = unicode ? BORDA_ES : ALT_BORDA_ES;
        std::string sd = unicode ? BORDA_SD : ALT_BORDA_SD;
        std::string ej = unicode ? BORDA_EJ : ALT_BORDA_EJ;
        std::string dj = unicode ? BORDA_DJ : ALT_BORDA_DJ;
        std::string v = unicode ? BORDA_V : ALT_BORDA_V;
        
        std::stringstream ss;
        ss << se << linhaHorizontal(largura - 2, unicode) << sd << "\n";
        ss << v << " " << colorirBold(titulo, CIANO) << std::string(largura - 3 - titulo.length(), ' ') << v << "\n";
        ss << ej << linhaHorizontal(largura - 2, unicode) << dj;
        return ss.str();
    }
    
    // Função para criar rodapé de caixa
    inline std::string criarRodapeCaixa(int largura = 56, bool unicode = true) {
        std::string ie = unicode ? BORDA_DS : ALT_BORDA_DS;
        std::string id = unicode ? BORDA_ID : ALT_BORDA_ID;
        
        return ie + linhaHorizontal(largura - 2, unicode) + id;
    }
    
    // Função para criar linha de caixa
    inline std::string criarLinhaCaixa(const std::string& conteudo, int largura = 56, bool unicode = true) {
        std::string v = unicode ? BORDA_V : ALT_BORDA_V;
        
        return v + " " + conteudo + std::string(std::max(0, largura - 3 - static_cast<int>(conteudo.length())), ' ') + v;
    }
}