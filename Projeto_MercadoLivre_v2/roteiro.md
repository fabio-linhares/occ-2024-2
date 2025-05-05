# Roteiro de Desenvolvimento: Otimização de Waves para o MercadoLivre

## Fase 1: Preparação e Análise ✓
- [x] Análise dos resultados atuais e comparação com BOV oficial
- [x] Identificação de pontos de melhoria no algoritmo atual
- [x] Proposta de estratégias para melhorar os resultados
- [x] Decisão de implementar múltiplas abordagens de otimização

## Fase 2: Refatoração do AnalisadorRelevancia ✓
- [x] Modificação da função de pontuação para focar na maximização do objetivo
- [x] Atualização da API com novos métodos `calcularRelevancia` e `ordenarPorRelevancia`
- [x] Correção de referências em arquivos dependentes
- [x] Teste de compilação

## Fase 3: Implementação do OtimizadorWave com Simulated Annealing ✓
- [x] Definição da estrutura de solução interna para armazenar pedidos e corredores
- [x] Implementação do algoritmo de Simulated Annealing
- [x] Adição de estratégias de intensificação e diversificação
- [x] Correção de erros de compilação e ajustes de API

## Fase 4: Implementação do Algoritmo de Dinkelbach ✓
- [x] Criação da classe `OtimizadorDinkelbach`
- [x] Implementação do algoritmo iterativo de Dinkelbach
- [x] Desenvolvimento da função de resolução do problema linearizado
- [x] Integração com estrutura de solução existente

## Fase 5: Integração e Seleção de Otimizadores ✓
- [x] Implementação da estrutura `SolucaoComum` para unificar resultados
- [x] Definição de critérios para escolha do otimizador baseado na instância
- [x] Seleção automática entre Dinkelbach e Simulated Annealing
- [x] Tratamento unificado da saída e gravação de resultados

## Fase 6: Melhorias na Interface e Formatação ✓
- [x] Correção de problemas com caracteres Unicode em bordas
- [x] Implementação de funções de formatação centralizadas
- [x] Padronização da saída para todos os módulos
- [x] Sincronização adequada de saídas paralelas

## Fase 7: Aprimoramento do Branch-and-Bound ✓
- [x] Implementação da estrutura básica de branch-and-bound
- [x] Controle de tempo para limitar explosão combinatória
- [x] Implementação de estratégias avançadas de seleção de variáveis:
  - [x] Seleção baseada em impacto potencial na função objetivo
  - [x] Estratégias de most-infeasible-first adaptada
  - [x] Seleção baseada em pseudo-custos com aprendizado
- [x] Implementação de técnicas de corte sofisticadas:
  - [x] Cortes de cobertura para pedidos incompatíveis
  - [x] Cortes de dominância para redução do espaço de busca
- [x] Melhoria do cálculo de limites superiores e inferiores
  - [x] Limites superiores com simulação inteligente
  - [x] Limites inferiores baseados em soluções reais

## Fase 8: Busca Local Avançada ✓
- [x] Implementação da classe base para busca local avançada:
  - [x] Estruturas de dados para soluções e movimentos
  - [x] Interface unificada para diferentes métodos de busca local
  - [x] Mecanismos de controle de tempo e estatísticas
- [x] Implementação de estruturas de vizinhança complexas:
  - [x] Movimentos de swap baseados em eficiência
  - [x] Movimentos de chain-exchange entre múltiplos pedidos
  - [x] Operadores de path-relinking
- [x] Integração de técnicas de escape de ótimos locais:
  - [x] Busca Tabu com memória de curto prazo
  - [x] Busca Tabu com memória de longo prazo
  - [x] Variable Neighborhood Search (VNS)
  - [x] Iterated Local Search (ILS)
- [x] Métodos auxiliares e utilitários:
  - [x] Aplicação de movimentos e cálculo de impacto
  - [x] Perturbação de soluções para ILS
  - [x] Busca local em múltiplas vizinhanças para VNS
- [x] Integração com algoritmo de Dinkelbach:
  - [x] Uso de busca local como refinamento de soluções
  - [x] Inicialização inteligente com soluções anteriores

## Fase 9: Integração com Solvers de Programação Linear/Inteira ✓
- [x] Formulação do problema como PLI (desenvolvendo solver próprio):
  - [x] Estrutura base para modelagem das variáveis de decisão
  - [x] Implementação completa das restrições de wave
  - [x] Formulação completa da função objetivo
- [x] Integração com o algoritmo de Dinkelbach:
  - [x] Uso de PLI para resolver subproblemas linearizados
  - [x] Warm-start a partir de soluções heurísticas
- [x] Implementação de relaxações para fornecer limites:
  - [x] Relaxação linear para obter limites duais
  - [x] Técnicas de arredondamento para recuperar soluções inteiras

## Fase 10: Técnicas de Decomposição para Instâncias Grandes 🔄
- [x] Implementação de decomposição por pedidos (parcial)
- [x] Implementação básica da classe OtimizadorDistribuido (parcial)
- [ ] Implementação de decomposição de Benders (em andamento)
- [ ] Implementação de técnicas de decomposição de Dantzig-Wolfe

## Fase 11: Paralelização Avançada 🔄
- [x] Implementação básica da classe OtimizadorParalelo (parcial)
- [x] Mecanismos de coordenação e comunicação entre threads (parcial)
- [ ] Divisão do espaço de busca (em andamento)
- [ ] Balanceamento de carga dinâmico entre threads

## Fase 12: Aprendizado de Máquina para Otimização
- [ ] Prediçao de parâmetros de algoritmos:
  - [ ] Ajuste automático de parâmetros de SA e Dinkelbach
  - [ ] Seleção de algoritmo baseada em características da instância
- [ ] Heurísticas aprimoradas por ML:
  - [ ] Aprendizado de boas combinações de pedidos
  - [ ] Modelos de priorização usando características dos pedidos
- [ ] Implementação de técnicas de ML para inicialização:
  - [ ] Geração de soluções iniciais de alta qualidade
  - [ ] Transferência de conhecimento entre instâncias similares

## Fase 13: Validação e Análise Estatística
- [ ] Testes intensivos em diferentes instâncias
- [ ] Análise comparativa de resultados entre algoritmos
- [ ] Identificação de características que influenciam desempenho
- [ ] Documentação detalhada dos resultados e conclusões

## Fase 14: Documentação e Visualização
- [ ] Documentação detalhada do código e algoritmos
- [ ] Visualização interativa de soluções e estatísticas
- [ ] Interface de configuração para parâmetros avançados
- [ ] Preparação de material para apresentação e publicação


## Status Atual: Reformulação e Ajuste Crítico

### Problemas Críticos Identificados
- BOV significativamente abaixo do esperado (até 96% abaixo do BOV oficial)
- Falhas na validação de soluções (LB, UB, estoque)
- Implementação incompleta de componentes essenciais

### Próximos Passos Prioritários
1. **Correção do algoritmo de Dinkelbach**
   - Rever o cálculo da função objetivo
   - Implementar proteções contra divisão por zero
   - Melhorar estimativa de lambda inicial

2. **Otimização da Busca Local**
   - Refinar estratégias de perturbação para focar em eficiência
   - Implementar intensificação mais agressiva em regiões promissoras
   - Desenvolver novas vizinhanças focadas na maximização do BOV

3. **Verificação e Correção de Viabilidade**
   - Garantir o respeito estrito aos limites LB e UB
   - Implementar reparo de soluções inviáveis
   - Priorizar pedidos com alta eficiência (unidades/corredores)

4. **Revisão do Branch-and-Bound**
   - Corrigir cálculo de limitantes
   - Refinar estratégias de ramificação
   - Melhorar heurística gulosa inicial

5. **Reteste completo**
   - Analisar BOV em todas as instâncias após correções
   - Comparar com BOVs oficiais
   - Documentar melhorias obtidas