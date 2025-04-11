#include "timercontrol.h"

// Definição das variáveis estáticas
std::atomic<bool> TimerControl::inicializado(false);
std::chrono::time_point<std::chrono::high_resolution_clock> TimerControl::inicio_global;
int TimerControl::tempo_limite_ms = 600000; // 10 minutos por padrão

void TimerControl::inicializar(int limite_ms) {
    tempo_limite_ms = limite_ms;
    inicio_global = std::chrono::high_resolution_clock::now();
    inicializado = true;
}

bool TimerControl::tempoExcedido(int margem_ms) {
    if (!inicializado) {
        return false; // Se não foi inicializado, assumir que não excedeu
    }
    
    auto agora = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(agora - inicio_global).count();
    
    return duracao >= (tempo_limite_ms - margem_ms);
}

int TimerControl::tempoRestante() {
    if (!inicializado) {
        return tempo_limite_ms; // Se não foi inicializado, retornar o tempo limite total
    }
    
    auto agora = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(agora - inicio_global).count();
    
    return (duracao >= tempo_limite_ms) ? 0 : (tempo_limite_ms - duracao);
}

int TimerControl::tempoDisponivel(double fracao) {
    int tempo_restante_ms = tempoRestante();
    return static_cast<int>(tempo_restante_ms * fracao);
}