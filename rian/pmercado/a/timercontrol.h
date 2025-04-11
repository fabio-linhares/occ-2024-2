#pragma once

#include <chrono>
#include <iostream>
#include <atomic>

class TimerControl {
private:
    // Apenas declare as variáveis estáticas, não as defina aqui
    static std::atomic<bool> inicializado;
    static std::chrono::time_point<std::chrono::high_resolution_clock> inicio_global;
    static int tempo_limite_ms;

public:
    static void inicializar(int limite_ms);
    static bool tempoExcedido(int margem_ms = 0);
    static int tempoRestante();
    static int tempoDisponivel(double fracao);
};