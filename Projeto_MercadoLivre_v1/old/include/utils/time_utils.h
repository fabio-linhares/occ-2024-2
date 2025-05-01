#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <chrono>

// Função para verificar se o tempo limite foi excedido
inline bool isTimeExpired(const std::chrono::high_resolution_clock::time_point& startTime, double timeLimit) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
    return elapsedTime >= timeLimit;
}

#endif // TIME_UTILS_H