# Visão Geral da Arquitetura

O **NaxStudio Flow** foi projetado com uma arquitetura modular em camadas sobre o framework **Qt 6 (C++17)**, separando estritamente a lógica de negócio, modelos de dados e controle de áudio (`src/core`) da camada de apresentação visual (`src/ui`).

---

## 🏛️ Diagrama de Camadas

```mermaid
flowchart TD
    subgraph App["Camada de Inicialização (src/app)"]
        Main["main.cpp\n(Fontes, QTranslator, QSettings)"]
    end

    subgraph UI["Camada de Apresentação (src/ui)"]
        FlowWin["Flow (QMainWindow)"]
        CatalogUI["CatalogWidget\nFolderItemDelegate / CatalogItemDelegate"]
        PlaylistUI["PlaylistView\nPlaylistItemDelegate"]
        CartWallUI["CartWallView\nCartWallItemDelegate"]
        EditorUI["MixEditor\n(Waveform & Envelope UI)"]
    end

    subgraph Core["Camada de Domínio e Áudio (src/core)"]
        CatalogCore["Catálogo\nCatalogRepository / CatalogScanner\nFolderListModel / CatalogListModel"]
        PlaylistCore["Playlist\nPlaylistManager / PlaylistListModel"]
        CartWallCore["Cartucheira\nCartWallTableModel"]
        AudioEngine["Motor de Áudio\nPlayerManager (QMediaPlayer + QAudioOutput)"]
        Utils["Utilitários\nTimeUtils / Models (models.h)"]
    end

    subgraph Storage["Persistência Local & Rede"]
        LocalData["Diretório data/\n(catalog.json, blocks/, profiles/, logs/)"]
        Sidecar["Metadados Sidecar\n(musica.mp3.json)"]
        NaxServer["NaxServer (API REST em Go)"]
    end

    Main --> FlowWin
    FlowWin --> CatalogUI
    FlowWin --> PlaylistUI
    FlowWin --> CartWallUI
    FlowWin --> EditorUI

    CatalogUI --> CatalogCore
    PlaylistUI --> PlaylistCore
    CartWallUI --> CartWallCore
    EditorUI --> Sidecar

    PlaylistCore --> AudioEngine
    CartWallCore --> AudioEngine

    CatalogCore --> LocalData
    PlaylistCore --> LocalData
    CartWallCore --> LocalData
    PlaylistCore --> Sidecar
    CatalogCore -.-> NaxServer
    PlaylistCore -.-> NaxServer
```

---

## 📦 Responsabilidades de Cada Diretório em `src/`

### 1. `src/app/` - Ponto de Entrada
- Contém [`main.cpp`](../../src/app/main.cpp) e o `CMakeLists.txt` que declara o executável `flow`.
- Responsável por:
  - Inicializar o `QApplication` e o ícone global da janela.
  - Carregar a família tipográfica **Roboto** embutida nos recursos (`.qrc`) e ativar a *feature* OpenType `tnum` (números tabulares de largura fixa, essenciais para cronômetros que mudam a cada milissegundo sem causar trepidação no texto).
  - Ler o idioma configurado no `QSettings("NaxStudio", "Flow")` (ou detectar o locale do sistema operacional) e instalar o `QTranslator` correspondente (`:/lang/<locale>.qm`).
  - Instanciar e exibir a janela principal `Flow` maximizada.

### 2. `src/core/` - Núcleo de Dados, Regras de Negócio e Áudio
- **Regra Arquitetural Fundamental**: Nenhum arquivo em `src/core/` pode depender de `src/ui/` ou de componentes visuais (`QWidget`, `QPainter`, `QStyle`).
- Contém:
  - [`models.h`](../../src/core/models.h): Estruturas centrais (`PlaylistItem`), enums de domínio (`MediaType`, `ItemType`, `BlockType`, `PlaylistStatus`, `PlaylistItemStatus`) e *Roles* (`PlaylistItemRoles`).
  - `catalog/`: Repositório de leitura de JSON (`CatalogRepository`), scanner multithread assíncrono (`CatalogScanner` e `CatalogScannerWorker`) e modelos de lista (`FolderListModel` e `CatalogListModel`).
  - `playlist/`: Gerenciador de execução contínua (`PlaylistManager`), modelo de dados e cálculo de blocos (`PlaylistListModel`) e controle individual de reprodução/fade (`player/PlayerManager`).
  - `cartwall/`: Estrutura de slot (`CartItem`) e modelo tabular linear mapeado em grade (`CartWallTableModel`).
  - `utils/`: Funções puras de formatação de tempo (`TimeUtils`).

### 3. `src/ui/` - Interface Gráfica (Views, Delegates e Docks)
- Contém a janela principal [`Flow`](../../src/ui/Flow.h) e a subpasta `widgets/`.
- Adota o padrão **Model/View/Delegate** do Qt para todas as listas e grades:
  - **Views (`PlaylistView`, `CartWallView`, `QListView`)**: Tratam eventos de mouse, seleção, cursores, redimensionamento de células e eventos de *Drag-and-Drop* (incluindo o desenho de indicadores visuais de inserção).
  - **Delegates (`PlaylistItemDelegate`, `CartWallItemDelegate`, `CatalogItemDelegate`, `FolderItemDelegate`)**: Herdam de `QStyledItemDelegate` e realizam toda a pintura customizada (`paint()`) diretamente com `QPainter`, sem instanciar `QWidgets` filhos para cada linha ou célula.

### 4. `src/resources/` e `src/lang/`
- `src/resources/`: Armazena todos os ícones vetoriais (`.svg`), ícones de categoria de mídia (`music.svg`, `jingle.svg`, `commercial.svg`, `other.svg`), fontes `Roboto` e o arquivo de recursos `app.qrc`.
- `src/lang/`: Armazena os arquivos de tradução `.ts` (`pt_BR.ts`, `en_US.ts`) que são compilados pelo `qt_add_translations` em arquivos binários `.qm` embutidos no executável sob o prefixo `:/lang`.

---

## 🎨 Arquitetura Model/View/Delegate na Interface

Em uma emissora de rádio real, uma pasta de músicas no catálogo pode conter de **2.000 a 20.000 arquivos**, uma grade de playlist de 24 horas reúne **centenas de inserções e blocos**, e a cartucheira é redimensionada ou reordenada com frequência. Instanciar milhares de `QWidgets` individuais causaria alto consumo de memória, lentidão na troca de pastas, engasgos na rolagem (*scrolling*) e alto uso de CPU ao atualizar barras de progresso em tempo real.

Para garantir fluidez constante, todas as listas e grades do **NaxStudio Flow** utilizam a arquitetura **Model/View/Delegate** do Qt:

| Camada | Componentes Principais | Responsabilidade |
| :--- | :--- | :--- |
| **Models (`src/core/`)** | `CatalogListModel`, `FolderListModel`, `PlaylistListModel` (`QAbstractListModel`) e `CartWallTableModel` (`QAbstractTableModel`) | Armazenam dados e estado sem conhecer detalhes visuais, permitindo manipulação e testes independentes. |
| **Views (`src/ui/widgets/`)** | `QListView` (Catálogo), `PlaylistView` (`QListView`), `CartWallView` (`QTableView`) | Gerenciam rolagem, seleção, eventos de mouse, redimensionamento e *Drag-and-Drop*. |
| **Delegates (`src/ui/widgets/`)** | `CatalogItemDelegate`, `FolderItemDelegate`, `PlaylistItemDelegate`, `CartWallItemDelegate` (`QStyledItemDelegate`) | Desenham gradientes, cantos arredondados, barras de progresso com *clip path*, badges (`NO AR` / `PRÓXIMO`), ícones SVG e textos elidados sob demanda com `QPainter`. |

Essa separação traz três benefícios práticos:
- **Desempenho $O(1)$ na Renderização**: O Qt invoca `paint()` apenas para os itens visíveis no *viewport*, independentemente de a lista possuir 10 ou 50.000 itens.
- **Atualização Leve de Progresso**: Atualizar a posição de reprodução de uma faixa emite apenas `dataChanged(idx, idx, {PositionRole})`, repintando somente o retângulo daquele card.
- **Cache de Recursos no Delegate**: Para manter `paint()` e `sizeHint()` extremamente rápidos, fontes, ícones e pincéis de gradiente (`QBrush`) são pré-cacheados como membros da classe no construtor do delegate (veja detalhes em [Convenções de Código C++ e Qt](../conventions/coding-style.md#-boas-práticas-em-qstyleditemdelegate)).

---

## 🗂️ Estrutura de Instalação e Diretório `data/`

Em ambiente de produção, a instalação do NaxStudio Flow é auto-contida e segue a separação entre binários e dados de operação:

```text
Flow/
├── bin/                      # Executável principal (flow / flow.exe), DLLs/SOs e plugins Qt
└── data/                     # Diretório raiz de persistência local
    ├── catalog.json          # Espelho local das pastas do catálogo e mapeamento de paths
    ├── blocks/               # Arquivos de blocos de programação salvos localmente
    ├── profiles/             # Arquivos JSON de cada perfil (permissões + cartucheira)
    ├── logs/                 # Histórico detalhado de reprodução (auditoria/ECAD)
    └── config/               # Configurações locais da estação (placas de áudio, rede, etc.)
```

Essa organização permite fazer backup, migração ou atualização dos binários em `Flow/bin/` sem risco de sobrescrever as configurações, perfis, logs ou blocos locais em `Flow/data/`.

