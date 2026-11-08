# Módulo de Playlist e Automação de Blocos

O módulo de Playlist é o coração operacional do **NaxStudio Flow**. Ele exibe e gerencia a sequência contínua de blocos de programação (horários, blocos comerciais e blocos musicais) e controla o estado de cada áudio desde a fila até a finalização.

---

## 🧱 Anatomia de um Bloco na Playlist

Diferente de players convencionais onde a lista contém apenas arquivos de áudio, o `PlaylistListModel` modela toda a estrutura da programação como uma sequência linear de quatro tipos de itens (`ItemType`):

```text
┌──────────────────────────────────────────────────────────┐
│ [BlockMarker]  14:00   Nome do Programa                  │ ◄── Cabeçalho do Bloco (Horário + Nome)
│ ──┬── [StartMarker] BLOCO COMERCIAL                      │ ◄── Início da Subseção Comercial
│   │   [Media] Comercial 01.mp3                (Finished) │
│ ──┴── [EndMarker]   BLOCO COMERCIAL   1 item - 00:30:000 │ ◄── Fim Comercial (Resumo calculado)
│ ──────────────────────────────────────────────────────── │
│       [StartMarker] (Musical)                            │ ◄── Início da Subseção Musical
│       [Media] Artista - Música 01.mp3         (Finished) │
│       [Media] Artista - Música 02.mp3         (NO AR)    │ ◄── Item atual (m_playingRow)
│       [Media] Vinheta 01.mp3                  (PRÓXIMO)  │ ◄── Próximo item a entrar
│       [Media] Artista - Música 03.mp3         (Default)  │
│       [EndMarker]                    4 itens - 14:47:375 │ ◄── Fim Musical (Resumo calculado)
└──────────────────────────────────────────────────────────┘
```

1. **`ItemType::BlockMarker`**: Define o início visual de um bloco de horário (ex.: `"14:00"` - `"Nome do Programa"`). Reinicia o acumulador de horário estimado (`accumulatedTime`) para o horário do bloco.
2. **`ItemType::StartMarker`**: Abre uma seção interna dentro do bloco (`BlockType::Commercial` ou `BlockType::Music`).
3. **`ItemType::Media`**: Representa uma inserção de áudio real (`Music`, `Jingle`, `Commercial` ou `Other`). Herda automaticamente o `BlockType` do `StartMarker` ativo.
4. **`ItemType::EndMarker`**: Fecha a seção comercial ou musical, exibindo a quantidade total de itens (`itemsCount`) e a soma das durações (`totalDurationMs`) daquela subseção.
5. **Itens de Comando (`Command`)**: Inserções especiais arrastadas do Catálogo que não são arquivos de áudio comuns, mas executam ações automatizadas ao serem atingidas pela reprodução:
   - **Comando de Pausa**: Ao chegar nesta inserção, o `PlaylistManager` pausa automaticamente a sequência da programação e aguarda até que o operador acione o botão `Next`.
   - **Comandos de Integração (ex.: OBS Studio)**: Disparam ações externas quando a fila passa pelo comando, como enviar um sinal via WebSocket para o OBS Studio trocar de cena ou acionar uma fonte.
   - **Comando de Hora Certa**: Dispara dinamicamente o anúncio do horário atual naquele ponto do bloco.

---

## ⏱️ Cálculo Automático de Metadados (`recalculateBlockMetadata`)

Sempre que um item é adicionado, inserido, removido ou movido na playlist, `PlaylistListModel::recalculateBlockMetadata()` percorre a lista atualizando todos os campos derivados:

1. **Horário Estimado de Execução (`estimatedTime`)**:
   - Parte do horário definido no último `BlockMarker` (formato `"hh:mm"`).
   - Para cada `ItemType::Media`, atribui o horário acumulado atual (`"hh:mm:ss"`) e avança o relógio somando a **duração efetiva** da faixa (descontando a sobreposição de mixagem `mixEnd` quando configurada).
2. **Contadores e Somas de Subseções**:
   - Soma separadamente a quantidade de itens e a duração total do segmento comercial e do segmento musical, gravando os totais nos respectivos `EndMarker` e no `BlockMarker`.
3. **Derivação de Status (`PlaylistItemStatus`)**:
   - Tudo acima de `m_playingRow` recebe `PlaylistItemStatus::Finished` (renderizado de forma compacta com 32px de altura).
   - A linha `m_playingRow` permanece como `Playing` (renderizada com 50px de altura, badge vermelho `NO AR` e barra de progresso).
   - O primeiro item de mídia imediatamente após `m_playingRow` recebe `PlaylistItemStatus::Next` (badge amarelo `PRÓXIMO`).
   - Os demais itens futuros recebem `PlaylistItemStatus::Default`.
4. **Coalescência de `dataChanged`**:
   - Para evitar repintar a lista inteira desnecessariamente, o método tira um *snapshot* dos campos derivados antes do cálculo e emite `dataChanged` apenas para os intervalos contíguos de linhas que realmente sofreram alteração.

---

## 🔒 Regras de Segurança e Drag-and-Drop

Para impedir acidentes durante a transmissão ao vivo:

- **Trava de Linha em Reprodução (`row <= playingRow`)**:
  - Nenhum item que já foi tocado (`Finished`) ou que está atualmente `NO AR` (`Playing`) pode ser arrastado, removido ou receber outro item acima dele.
- **Restrição de Soltura entre Marcadores (`isDropRowBetweenMarkers`)**:
  - Um item de mídia só pode ser solto em uma posição `row` se o item imediatamente anterior (`row - 1`) for um `StartMarker` ou `Media` **e** o item na posição `row` for um `EndMarker` ou `Media`.
  - Isso garante que músicas ou comerciais nunca sejam soltos fora de um bloco ou entre um `EndMarker` e um `StartMarker`.
- **Recepção de Mídias do Catálogo**:
  - Além de reordenar itens internos (`Qt::MoveAction`), a playlist aceita o drop de arquivos vindos do `CatalogWidget` (`application/x-catalog-item`), inserindo-os na posição exata indicada pela linha amarela (`#eab308`) do `PlaylistView`.

---

## 🔄 Carregamento Dinâmico de Blocos, "Abrir Programação" e Retenção de Histórico

Para manter a visualização limpa e o consumo de memória enxuto durante transmissões 24/7, a Playlist opera como uma **janela deslizante dinâmica**:

1. **Botão "Abrir Programação"**:
   - Permite ao operador escolher uma data e horário específicos (por exemplo, `07/11 às 21h55`).
   - O sistema localiza e carrega a programação a partir do **próximo bloco mais próximo desse horário**, no caso do exemplo acima, o bloco das 22h.
2. **Janela Deslizante de Blocos Futuros (Padrão: 4 Blocos)**:
   - A Playlist mantém carregados simultaneamente **4 blocos** por padrão (quantidade ajustável na Janela de Configurações).
   - Assim que um bloco termina de ser executado, o próximo bloco subsequente da programação é carregado automaticamente no final da lista.
3. **Retenção de Histórico de Inserções Finalizadas (Padrão: 10 Inserções)**:
   - No topo da lista, são mantidas as últimas **10 inserções já finalizadas (`Finished`)** por padrão (valor também configurável nas configurações).
   - Essa contagem de *"10 inserções"* **não deve contabilizar os marcadores**. Quando todas as inserções de um bloco antigo deixam o histórico, seus respectivos marcadores também são removidos automaticamente.

