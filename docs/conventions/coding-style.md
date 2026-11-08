# Convenções de Código C++ e Qt

Este documento define as convenções de estilo, organização de cabeçalhos, gerência de memória e práticas de alto desempenho adotadas no código-fonte C++17 / Qt 6 do **NaxStudio Flow**.

---

## 🔤 Nomenclatura

| Elemento | Padrão | Exemplos no Projeto |
| :--- | :--- | :--- |
| **Arquivos (`.h`, `.cpp`, `.ui`)** | `PascalCase` | `PlaylistListModel.h`, `CartWallView.cpp`, `MixEditor.ui` |
| **Classes e Structs** | `PascalCase` | `CatalogScanner`, `PlaylistItem`, `CartItem` |
| **Métodos e Funções** | `camelCase` | `recalculateBlockMetadata()`, `formatDuration()` |
| **Membros Privados de Classe** | `m_` + `camelCase` | `m_items`, `m_workerThread`, `m_dropIndicatorRow` |
| **Constantes Locais/Estáticas** | `k` + `PascalCase` | `kBlockRadius`, `kCellMargin`, `kCatalogMime` |
| **Enums de Domínio** | `enum class` em `PascalCase` | `MediaType::Jingle`, `PlaylistItemStatus::Playing` |
| **Enums de Roles do Qt** | `enum` com sufixo `Role` | `UuidRole`, `CartStatusRole`, `CatalogPathRole` |
| **Macros de Compilação / Guards** | `UPPER_SNAKE_CASE` | `PLAYLISTLISTMODEL_H`, `APP_VERSION` |

---

## 🧱 Organização de Arquivos e Includes

1. **Header Guards**:
   - Todos os arquivos `.h` devem conter `#ifndef NOME_DO_ARQUIVO_H` / `#define NOME_DO_ARQUIVO_H` / `#endif // NOME_DO_ARQUIVO_H`.
2. **Ordem de `#include`**:
   - No arquivo `.cpp`, o primeiro include deve ser sempre o seu próprio cabeçalho (ex.: `#include "CartWallView.h"`).
   - Em seguida, includes internos do projeto usando caminhos relativos a `src/` (ex.: `#include "core/cartwall/CartItem.h"`).
   - Por último, cabeçalhos do Qt (`<QAbstractListModel>`, `<QPainter>`) e da biblioteca padrão C++ (`<algorithm>`, `<cmath>`, `<memory>`).
   - O arquivo `.clangd` do projeto já define `--header-insertion=never` para evitar que a IDE insira includes automáticos fora de ordem.

---

## 🧠 Gerência de Memória e Padrões Qt

1. **Árvore de Parentesco `QObject`**:
   - Todo `QObject` ou `QWidget` alocado dinamicamente com `new` deve receber seu `parent` no construtor (`new PlaylistManager(this)`), garantindo destruição automática quando o pai for destruído.
   - Para estruturas que não herdam de `QObject` (como caches de fontes em `PlaylistItemDelegate::Fonts`), utilize `std::unique_ptr` ou alocação por valor na stack/membro da classe.
2. **Containers e Passagem de Parâmetros**:
   - Prefira `QVector` / `QList` e `QHash` para coleções integradas às APIs do Qt.
   - Ao iterar sobre containers membros do Qt em loops `for` baseados em intervalo onde o container não deve sofrer *detach* (cópia *Copy-On-Write*), utilize métodos `const` ou `std::as_const(m_items)`.
   - Use `QStringLiteral("...")` para literais de string estáticos que não precisam de tradução (como formatos `"hh:mm:ss"` ou `"00:00:000"`), evitando alocações em tempo de execução.
3. **Signals & Slots (Sintaxe de Ponteiro de Função)**:
   - Utilize sempre a sintaxe moderna verificada em tempo de compilação:
     ```cpp
     connect(m_scanner, &CatalogScanner::finish,
             this, &CatalogWidget::onFilesLoaded);
     ```
   - Nunca utilize as macros antigas `SIGNAL()` e `SLOT()` baseadas em strings.

---

## 🎨 Boas Práticas em `QStyledItemDelegate`

Como os delegates (`PlaylistItemDelegate`, `CartWallItemDelegate`, `CatalogItemDelegate`, `FolderItemDelegate`) executam o método `paint()` dezenas de vezes por segundo durante animações de progresso e rolagem:

1. **Proibido Alocar Recursos Pesados no `paint()`**:
   - **Nunca** carregue arquivos SVG (`QIcon(":/...")`) ou construa gradientes complexos dentro do `paint()`.
   - Inicialize todos os `QIcon` e `QBrush` como membros da classe no construtor do Delegate.
2. **Gradientes Independentes de Retângulo (`ObjectBoundingMode`)**:
   - Ao criar `QLinearGradient` para cards de mídia, configure `g.setCoordinateMode(QGradient::ObjectBoundingMode)` com coordenadas de `0.0` a `1.0`. Isso permite criar o `QBrush` uma única vez no construtor e reutilizá-lo para pintar cards de qualquer largura ou altura.
3. **Balanceamento de `QPainter::save()` e `restore()`**:
   - Sempre envolva alterações de estado do `QPainter` (especialmente quando usar `setClipPath()`) com `p->save()` e `p->restore()`.
   - Ative `QPainter::Antialiasing` apenas ao desenhar formas arredondadas (`drawRoundedRect`, `drawPath`, `drawEllipse`) e desative-o ao desenhar linhas retas ortogonais de 1px para evitar borrões visuais.

