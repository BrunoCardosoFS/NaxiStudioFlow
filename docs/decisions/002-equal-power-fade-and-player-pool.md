# ADR 002: Curva de Fade Equal-Power, Sincronismo de Mixagem e Pool de 3 Players

- **Status**: Aceito (PlayerManager implementado para v1.x; engine própria FFmpeg planejada para v2.0)
- **Contexto**: `src/core/playlist/player/PlayerManager` e `src/core/playlist/PlaylistManager`

---

## Contexto e Problema

Em automação de rádio profissional, três requisitos técnicos determinam a perfeição das transições sonoras:

1. **Queda de Energia em Fades Lineares**: Quando uma música faz *Fade Out* linear enquanto outra faz *Fade In* linear ($v_{\text{out}}(t) = 1 - t$ e $v_{\text{in}}(t) = t$), a soma da potência acústica no meio da transição ($t = 0.5$) cai para $0.5^2 + 0.5^2 = 0.5$ ($-3\text{ dB}$), gerando um "buraco" perceptível no volume da transmissão.
2. **Sincronismo entre `mixEnd` e `mixStart`**: Se a próxima faixa só começar a tocar quando a faixa atual atingir seu `mixEnd`, qualquer introdução ou rampa de entrada entre `startPoint` e `mixStart` da próxima faixa atrasará o ponto real de encaixe da mixagem.
3. **Sobreposição Tripla em Transições Rápidas**: Quando há uma faixa curta (ex.: uma vinheta entre a Música 1 e a Música 2), ou quando a distância $(\text{mixStart}_{i+2} - \text{startPoint}_{i+2})$ da Música 2 exige que ela comece antes da vinheta e da cauda da Música 1 terminarem, apenas 2 players não são suficientes sem cortar o final da Música 1.

---

## Decisão

1. **Curva de Fade *Equal-Power* com Fast-Path**:
   - No `PlayerManager`, calcular o fator de ganho nos trechos de *Fade In* e *Fade Out* usando a função trigonométrica:
     $$\text{fator}(t) = \sin\left(t \cdot \frac{\pi}{2}\right)$$
   - Como $\sin^2(\theta) + \cos^2(\theta) = 1$, a potência acústica combinada durante um *crossfade* mantém-se constante em $100\%$ ($0\text{ dB}$).
   - Implementar um *fast-path* (`Q_LIKELY`) para a região de platô e filtrar atualizações de volume menores que `0.002f`.

2. **Sincronismo Antecipado (`mixEnd` da atual alinhado ao `mixStart` da próxima)**:
   - O `PlaylistManager` dispara a próxima faixa ($i+1$, iniciando em $\text{startPoint}_{i+1}$) antes do `mixEnd` da faixa atual ($i$), exatamente no instante:
     $$\text{pos}_i = \text{mixEnd}_i - (\text{mixStart}_{i+1} - \text{startPoint}_{i+1})$$
   - Dessa forma, quando a faixa $i$ alcança $\text{mixEnd}_i$, a faixa $i+1$ está exatamente passando por $\text{mixStart}_{i+1}$.

3. **Pool em Rodízio de 3 Players e Pré-carregamento Dinâmico**:
   - O `PlaylistManager` mantém 3 instâncias de `PlayerManager` (`player0`, `player1`, `player2`) em rodízio circular.
   - O pré-carregamento estendido da faixa $i+2$ é calculado **dinamicamente** comparando o intervalo $(\text{mixStart}_{i+2} - \text{startPoint}_{i+2})$ com a duração da faixa intermediária $i+1$ (e a cauda restante da faixa $i$), pré-carregando o 3º player sempre que o ponto de disparo de $i+2$ exigir preparação antecipada.

4. **Roadmap de Evolução (v1.x com Qt Multimedia $\rightarrow$ v2.0 com Engine FFmpeg Própria)**:
   - Na **versão 1.x**, manter a implementação sobre `Qt6::Multimedia` (backend FFmpeg) para acelerar a entrega estável de todos os módulos.
   - Na **versão 2.0**, substituir o `Qt6::Multimedia` por uma engine de áudio própria comunicando-se diretamente com o FFmpeg para controle de baixo nível do pipeline de áudio.

---

## Consequências

- **Positivas**:
  - Sincronismo exato de batida/locução entre o ponto de saída (`mixEnd`) de um áudio e o ponto de entrada (`mixStart`) do próximo.
  - Zero cortes de cauda mesmo quando o intervalo $(\text{mixStart} - \text{startPoint})$ da música seguinte se sobrepõe a uma vinheta curta e ao fade-out da música anterior.
  - Uso inteligente de memória através do cálculo dinâmico de pré-carregamento.
