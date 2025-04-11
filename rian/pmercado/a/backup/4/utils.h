#pragma once

#include <vector>
#include <string>
#include "solucao.h"
#include <fstream>
#include <iostream>

void gerarRelatorioDetalhado(const std::vector<ResultadoInstancia>& resultados, long long tempo_total_ms);