# Guia de Contribuição - NaxStudio Flow

Boas-vindas ao **NaxStudio Flow**! Ficamos muito felizes com o seu interesse em contribuir para o desenvolvimento do nosso projeto.

Este documento reúne todas as instruções essenciais para configurar seu ambiente de desenvolvimento, entender a organização do projeto e enviar suas contribuições. Para mais detalhes sobre a arquitetura e os módulos internos, consulte a documentação completa em [`docs/README.md`](docs/README.md).

---

## 📋 Índice

1. [Filosofia](#-filosofia)
2. [Pré-requisitos de Desenvolvimento](#-pré-requisitos-de-desenvolvimento)
3. [Configurando o Ambiente e Compilando](#-configurando-o-ambiente-e-compilando)
4. [Visão Geral da Estrutura do Projeto](#-visão-geral-da-estrutura-do-projeto)
5. [Fluxo de Trabalho (Git e Pull Requests)](#-fluxo-de-trabalho-git-e-pull-requests)
6. [Padrões de Código e Arquitetura](#-padrões-de-código-e-arquitetura)
7. [Internacionalização (Traduções)](#-internacionalização-traduções)
8. [Onde Encontrar o Que Fazer](#-onde-encontrar-o-que-fazer)

---

## 🤝 Filosofia

O **NaxStudio Flow** é desenvolvido para operar em ambientes de radiodifusão (24/7). Por isso, todas as contribuições devem priorizar:

- **Estabilidade**: O software nunca deve travar ou interromper a transmissão por falhas de rede, arquivos ausentes ou erros de interface.
- **Baixo consumo de recursos e fluidez visual**: Interfaces responsivas mesmo em computadores modestos, priorizando renderização direta via `QPainter` (`Model/View/Delegate`).
- **Operação Offline-First**: Mesmo integrado ao servidor de gerenciamento ([NaxServer](https://github.com/BrunoCardosoFS/NaxServer)), o Flow deve ser capaz de operar de forma totalmente autônoma utilizando seus arquivos locais.

---

## 🛠️ Pré-requisitos de Desenvolvimento

Para compilar e desenvolver o NaxStudio Flow, você precisará de:

- **Compilador C++17**:
  - **Windows**: MinGW 13.1+ (distribuído junto ao instalador do Qt) ou MSVC 2022
  - **Linux**: GCC 11+ ou Clang 13+
  - **macOS**: Xcode Command Line Tools (Clang)
- **CMake**: versão `3.16` ou superior
- **Qt 6**: versão `6.5` ou superior (recomendado **Qt 6.8.x**) com os seguintes módulos instalados:
  - `Qt6::Core`
  - `Qt6::Widgets`
  - `Qt6::Multimedia` (com plugin de backend **FFmpeg**)
  - `Qt6::LinguistTools`
- **Recomendado para IDE**:
  - **VS Code** com a extensão **clangd** (o projeto já exporta `compile_commands.json` automaticamente para `build/`), ou **Qt Creator**.

---

## 🚀 Configurando o Ambiente e Compilando

### 1. Clonar o repositório

```bash
git clone https://github.com/BrunoCardosoFS/NaxiStudioFlow.git
cd NaxiStudioFlow
```

### 2. Configurar o CMake

Certifique-se de que o diretório do seu kit do Qt 6 esteja no `PATH` ou informe-o via `-DCMAKE_PREFIX_PATH`:

```bash
# Modo Debug (recomendado durante o desenvolvimento)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Ou informando explicitamente o caminho do Qt:
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
```

> **Nota sobre o `clangd`**: A flag `CMAKE_EXPORT_COMPILE_COMMANDS` já está ativa no `CMakeLists.txt` raiz e o arquivo `.clangd` aponta para `build/`. Assim que você rodar o comando de configuração acima, o autocompletar e a análise estática funcionarão automaticamente.

### 3. Compilar o projeto

```bash
cmake --build build --config Debug --parallel
```

O executável `flow` (ou `flow.exe` no Windows) será gerado dentro do diretório `build/`.

### 4. Estrutura de Execução Local (`data/`)

Em produção e durante testes locais, o Flow utiliza uma pasta de dados persistentes para armazenar o catálogo, blocos de programação, perfis e configurações:

```text
Flow/
├── bin/          # Executável e bibliotecas dinâmicas
└── data/         # Dados locais
    ├── catalog.json
    ├── blocks/
    ├── profiles/
    ├── logs/
    └── config/
```

---

## 🌿 Fluxo de Trabalho (Git e Pull Requests)

### 1. Branches

Crie uma branch descritiva a partir da `main` de acordo com o tipo de trabalho:

- `feat/<escopo>-<descricao>` - Novas funcionalidades (ex.: `feat/cartwall-audio-playback`)
- `fix/<escopo>-<descricao>` - Correções de bugs (ex.: `fix/playlist-drop-indicator`)
- `refactor/<escopo>-<descricao>` - Refatorações de código sem mudança de comportamento externo
- `docs/<descricao>` - Criações ou atualizações de documentação

### 2. Mensagens de Commit (Conventional Commits)

Seguimos o padrão **Conventional Commits** com mensagens escritas em **inglês** (para manter consistência com o histórico do repositório):

```text
<type>(<scope>): <short summary in imperative mood>

- Optional bullet points explaining the changes in detail
```

- **Tipos permitidos (`<type>`)**:
  - `feat`: Nova funcionalidade
  - `fix`: Correção de bug
  - `refactor`: Refatoração de código
  - `style`: Ajustes visuais, QSS ou formatação de código
  - `perf`: Melhoria de desempenho
  - `docs`: Alterações na documentação
  - `chore`: Alterações em scripts de build, CMake, `.gitignore`, etc.
- **Escopos comuns (`<scope>`)**: `core`, `ui`, `playlist`, `catalog`, `cartwall`, `editor`, `player`, `build`, `lang`.

**Exemplos reais do projeto**:
```text
feat(ui): implement CartWallView and CartWallItemDelegate
refactor(core): modernize catalog architecture with repository, scanner worker, and folder model
fix(ui): round selected finished item highlight in PlaylistItemDelegate
```

### 3. Enviando um Pull Request

1. Verifique se o projeto compila sem erros ou *warnings* novos.
2. Certifique-se de que não incluiu arquivos temporários de teste, binários ou diretórios `build/` e `data/`.
3. Abra o Pull Request descrevendo claramente:
   - **O que foi feito** e **por quê**.
   - Caso haja mudanças visuais na interface, anexe capturas de tela ou GIFs demonstrando o resultado.

---

## 📐 Padrões de Código e Arquitetura

Antes de escrever código, leia os guias em [`docs/conventions/`](docs/conventions/coding-style.md). Em resumo:

1. **Separação estrita `core` vs. `ui`**:
   - Arquivos dentro de `src/core/` **jamais** devem incluir cabeçalhos de `src/ui/` ou depender de widgets visuais (`QWidget`, `QPainter`, etc.). A comunicação entre `core` e `ui` ocorre exclusivamente via **Model/View (`QAbstractItemModel`)** e **Signals & Slots**.
2. **Nomenclatura**:
   - **Arquivos e Classes**: `PascalCase` (ex.: `PlaylistListModel.h`, `CartWallView.cpp`).
   - **Métodos, Funções e Variáveis**: `camelCase` (ex.: `recalculateBlockMetadata()`, `currentPlayingRow()`).
   - **Atributos Privados de Classe**: prefixo `m_` em `camelCase` (ex.: `m_items`, `m_workerThread`, `m_dropIndicatorRow`).
   - **Constantes locais/estáticas**: prefixo `k` em `PascalCase` dentro de namespaces anônimos (ex.: `kBlockRadius`, `kCellMargin`).
   - **Enums**: prefira `enum class` para estados e tipos (`MediaType`, `ItemType`, `PlaylistItemStatus`). Para *Roles* do Qt (`Qt::UserRole`), utilize `enum` tradicional com sufixo `Role` (`UuidRole`, `CartStatusRole`).
3. **Formatação**:
   - Estilo baseado em **LLVM/Clang**, com indentação de **2 espaços**, sem tabs.
4. **Performance de UI**:
   - Listas e grades dinâmicas (Catálogo, Playlist, Cartucheira) devem usar `QListView` / `QTableView` + `QStyledItemDelegate`.
   - Faça cache de `QIcon`, `QBrush` (com `QGradient::ObjectBoundingMode`) e `QFontMetrics` nos construtores dos delegates para evitar alocações durante o método `paint()`.

---

## 🌍 Internacionalização (Traduções)

- Todo texto visível ao usuário no código C++ deve ser envolvido por `tr("Texto")` (dentro de subclasses de `QObject`) ou `QCoreApplication::translate()`.
- Os arquivos de tradução ficam em `src/lang/ts/` (`pt_BR.ts` e `en_US.ts`).
- Mais detalhes em [`docs/conventions/ui-and-i18n.md`](docs/conventions/ui-and-i18n.md).

---

## 🗺️ Onde Encontrar o Que Fazer

- Consulte o [**Roadmap Oficial (`docs/ROADMAP.md`)**](docs/ROADMAP.md) para ver todas as funcionalidades planejadas e o que está pendente de implementação em cada módulo.
- Veja as [Issues abertas no GitHub](https://github.com/BrunoCardosoFS/NaxiStudioFlow/issues) para bugs reportados ou tarefas específicas.
- Leia a [Documentação Técnica (`docs/README.md`)](docs/README.md) para compreender como cada parte do sistema funciona antes de iniciar sua implementação.

