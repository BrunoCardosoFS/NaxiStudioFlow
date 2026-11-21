# Motor de Áudio e Roteamento

O motor de áudio do **NaxStudio Flow** na **versão 1.x.x** é construído sobre o módulo `Qt6::Multimedia` (utilizando o backend **FFmpeg** definido em `CMakeLists.txt`). Ele é projetado para garantir transições musicais perfeitas (*crossfading*) com alinhamento preciso de pontos de mixagem, curvas de fade naturais ao ouvido humano, disparo simultâneo na cartucheira e separação física de placas de som.

> **Evolução Planejada para a Versão 2.0**: Na versão 2.0, o NaxStudio Flow substituirá o `Qt6::Multimedia` por uma **engine de áudio própria construída diretamente sobre as bibliotecas do FFmpeg**, eliminando a dependência do wrapper multimídia do Qt para obter controle total sobre o fluxo de áudio, buffers, mixagem em nível de amostras e processamento em tempo real.

---

## 🎯 Sincronismo dos Pontos de Mixagem (`mixEnd` $\leftrightarrow$ `mixStart`)

Na automação do NaxStudio Flow, a transição entre a faixa atual ($i$) e a próxima faixa ($i+1$) não espera simplesmente o `mixEnd` da faixa atual para dar play no início da próxima. Em vez disso, **o próximo áudio começa antes, de modo que o `mixEnd` do áudio atual fique exatamente sincronizado no tempo com o `mixStart` do próximo áudio**:

```text
Faixa Atual (i):     |---[startPoint]=================[mixEnd]-----(fadeOut)-----[endPoint]---|
                                                         │ (Pontos sincronizados no tempo)
Próxima Faixa (i+1):                      |---[startPoint]====[mixStart]======================|
                                          ▲
                         Instante exato de disparo da Faixa (i+1)
```

### Cálculo do Instante de Disparo

Seja $\Delta t_{\text{intro}}(i+1) = \text{mixStart}_{i+1} - \text{startPoint}_{i+1}$ o tempo que a próxima faixa leva desde o seu ponto de partida (`startPoint`) até atingir seu ponto de mixagem inicial (`mixStart`).

Para que $\text{mixStart}_{i+1}$ coincida exatamente com $\text{mixEnd}_i$, o `PlaylistManager` deve disparar a faixa $i+1$ (a partir de $\text{startPoint}_{i+1}$) quando a posição de reprodução da faixa atual $i$ atingir:

$$\text{Posição de Disparo em } i = \text{mixEnd}_i - (\text{mixStart}_{i+1} - \text{startPoint}_{i+1})$$

---

## 🔄 Rodízio de Players na Playlist (`PlaylistManager`)

Como a próxima faixa antecipa sua entrada para alinhar seu `mixStart` ao `mixEnd` da atual (enquanto a atual ainda cumprirá seu `fadeOut` até `endPoint`), e frequentemente existem vinhetas curtas entre duas músicas — ou casos em que o intervalo $(\text{mixStart}_{i+2} - \text{startPoint}_{i+2})$ da Música 2 exige que ela comece antes que a vinheta intermediária e a cauda da Música 1 terminem — ocorrem momentos em que **três faixas se sobrepõem simultaneamente**. Apenas 2 players não seriam suficientes sem cortar o final da faixa anterior.

Por esse motivo, o [`PlaylistManager`](../../src/core/playlist/PlaylistManager.h) mantém um pool de **3 instâncias de `PlayerManager`** (`player0`, `player1`, `player2`) operando em rodízio circular (*round-robin*):

```text
Fila da Playlist: [Arquivo 1, Arquivo 2, Arquivo 3, Arquivo 4, ...]

player0 ──► Arquivo 1
player1 ──► Arquivo 2
player2 ──► Arquivo 3
player0 ──► Arquivo 4 (reutilizado após o término completo do Arquivo 1 em endPoint)
```

### Estratégia Dinâmica de Pré-carregamento

Para economizar memória RAM sem atrasar nenhuma entrada antecipada, a decisão de pré-carregar apenas o próximo item ($i+1$) ou também o subsequente ($i+2$) é tomada **de forma dinâmica**, analisando a relação entre os pontos das faixas:

1. **Pré-carregamento Padrão ($i+1$)**:
   - Enquanto a faixa atual ($i$) está tocando, o próximo player do rodízio mantém a faixa $i+1$ (`Next`) pré-carregada e posicionada em $\text{startPoint}_{i+1}$.
2. **Pré-carregamento Estendido Dinâmico ($i+2$)**:
   - O sistema avalia o tempo útil da faixa intermediária $i+1$ ($\text{mixEnd}_{i+1} - \text{startPoint}_{i+1}$, ou a partir de $\text{mixStart}_{i+1}$) frente ao tempo de antecendência necessário para a entrada da faixa $i+2$, dado por $(\text{mixStart}_{i+2} - \text{startPoint}_{i+2})$.
   - A posição dentro da faixa $i+1$ em que a faixa $i+2$ deverá começar a tocar é:
     $$\text{triggerPos}_{i+1 \to i+2} = \text{mixEnd}_{i+1} - (\text{mixStart}_{i+2} - \text{startPoint}_{i+2})$$
   - Se esse instante de disparo ocorrer enquanto a faixa $i$ ainda não tiver atingido seu $\text{endPoint}_i$ (tripla sobreposição) ou se a janela disponível entre o início de $i+1$ e $\text{triggerPos}_{i+1 \to i+2}$ for curta demais para carregar $i+2$ apenas depois que $i+1$ entrar no ar, o 3º player do rodízio pré-carrega dinamicamente a faixa $i+2$ com antecedência.

---

## 📈 Automação de Volume e Curva *Equal-Power* (`PlayerManager`)

O [`PlayerManager`](../../src/core/playlist/player/PlayerManager.h) encapsula um `QMediaPlayer` e um `QAudioOutput`, monitorando o sinal `positionChanged` para calcular o volume instantâneo em função dos pontos configurados em `PlayerMetadata`:

```text
Volume
1.0 (Target) |          ___________________
             |         /                   \
             |        /                     \
             |       /                       \
0.0 (Mudo)   |______/                         \_______
             +------+--+-------------------+--+------+---> Tempo (ms)
             0   start fadeIn           fadeOut end  duration
                 Point EndPoint       StartPoint Point
```

### Fórmula da Curva *Equal-Power*

Fades lineares comuns ($v_{\text{out}}(t) = 1 - t$ e $v_{\text{in}}(t) = t$) fazem a soma da potência acústica no meio da transição ($t = 0.5$) cair para $0.5^2 + 0.5^2 = 0.5$ ($-3\text{ dB}$), gerando um "buraco" perceptível de energia sonora no centro de um *crossfade*. Para evitar isso, o `PlayerManager` aplica uma curva sinusoidal de potência constante (*equal-power*):

$$\text{fator}(t) = \sin\left(t \cdot \frac{\pi}{2}\right), \quad t \in [0, 1]$$

Onde $t$ é o progresso normalizado dentro da janela de *Fade In* ou *Fade Out*. Como $\sin^2(\theta) + \cos^2(\theta) = 1$, a potência acústica combinada durante um *crossfade* mantém-se constante em $100\%$ ($0\text{ dB}$).

### Otimizações Críticas de Performance em `volumeControl()`

Como `positionChanged` é disparado diversas vezes por segundo durante toda a reprodução:
1. **Pré-cálculo de Inversos (`updateFadeParameters`)**: As divisões `1.0f / fadeInDuration` e `1.0f / fadeOutDuration` são pré-calculadas apenas quando os metadados mudam, trocando divisões em ponto flutuante por multiplicações rápidas a cada tick.
2. **Fast-Path de Platô (`Q_LIKELY`)**: Mais de 95% do tempo de execução de uma música ocorre entre `fadeInEndPoint` e `fadeOutStartPoint`. Um branch otimizado com `Q_LIKELY` aplica imediatamente `m_targetVolume` sem executar cálculos trigonométricos.
3. **Filtro de Micro-Variações (`applyVolume`)**: O volume só é repassado ao `QAudioOutput` e emitido via `volumeChanged` se a diferença absoluta em relação ao volume anterior for $\ge 0.002$ (ou se atingir os extremos `0.0f` ou `m_targetVolume`), evitando sobrecarga no subsistema de áudio do sistema operacional.
4. **Proteção para Áudios Curtos**: Se um áudio for tão curto que `fadeInEndPoint` e `fadeOutStartPoint` se sobreponham, o algoritmo calcula ambos os fatores e aplica `std::min(inFactor, outFactor)` para garantir uma transição suave sem saltos de volume.

---

## 🎛️ Motor de Áudio da Cartucheira (`CartWall`)

Ao contrário da Playlist (que usa um rodízio de 3 players para uma fila única), na Cartucheira:
- **Players Independentes por Slot**: Cada slot disparado controla sua própria instância de reprodução, permitindo que o operador dispare múltiplos efeitos sonoros, trilhas de fundo e vinhetas simultaneamente.
- **Modo Loop**: Qualquer slot pode ser marcado em modo de repetição contínua (útil para trilhas/BG de locução).
- **Fila Encadeada (`Ctrl + Clique`)**: O operador pode segurar `Ctrl` e clicar em dois ou mais slots em sequência. Ao disparar o primeiro slot da cadeia, o motor monitora os pontos de mixagem e dispara automaticamente o próximo slot encadeado sincronizando o `mixEnd` do atual com o `mixStart` do próximo.

---

## 🔌 Roteamento para Múltiplas Placas de Som

Em estúdios de rádio profissionais, diferentes origens de áudio costumam entrar em canais separados da mesa de som analógica/digital ou em fones de ouvido dedicados. O NaxStudio Flow suporta o mapeamento independente de dispositivos (`QAudioDevice`) para **3 barramentos de saída**:

| Barramento | Destino Típico no Estúdio | Componentes que Utilizam |
| :--- | :--- | :--- |
| **1. Saída Principal (Playlist)** | Canal "Automação / Musical" da mesa de som (No Ar) | `PlaylistManager` (`player0`, `player1`, `player2`) |
| **2. Saída da Cartucheira** | Canal "Cartucheira / Vinhetas" da mesa de som (No Ar) | Slots da `CartWall` |
| **3. Saída de Pré-escuta (CUE)** | Fone de ouvido do locutor / Canal CUE (Fora do Ar) | `PreListen` do Catálogo e `MixEditor` |
