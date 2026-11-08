# ADR 001: Migração para Arquitetura Model/View/Delegate do Qt

- **Status**: Aceito e Implementado
- **Contexto**: Camadas `src/core/` e `src/ui/` (Catálogo, Playlist e Cartucheira)

---

## Contexto e Problema

Nas primeiras versões de prototipagem do **NaxStudio Flow**, cada item do catálogo (`FileWidget`), cada bloco/item da playlist (`ProgramBlock`, `PlaylistItem`) e cada botão da cartucheira (`CartWallArea`, `CartWallItem`) era instanciado como um `QWidget` independente com seu próprio arquivo `.ui` e layout interno.

Em uma emissora de rádio real:
- Uma pasta de músicas no catálogo pode conter de **2.000 a 20.000 arquivos**.
- Uma grade de playlist de 24 horas pode conter **centenas de inserções e blocos**.
- Uma cartucheira redimensionada ou reordenada com frequência sofria lentidão ao destruir e recriar dezenas de widgets na árvore da janela.

Instanciar milhares de `QWidgets` causa alto consumo de memória, lentidão na troca de pastas, engasgos na rolagem (*scrolling*) e alto uso de CPU ao atualizar barras de progresso em tempo real.

---

## Decisão

Substituir integralmente os `QWidgets` repetitivos pela arquitetura **Model/View/Delegate** do Qt:

1. **Models em `src/core/`**:
   - `CatalogListModel` e `FolderListModel` (`QAbstractListModel`)
   - `PlaylistListModel` (`QAbstractListModel`)
   - `CartWallTableModel` (`QAbstractTableModel`)
2. **Views em `src/ui/widgets/`**:
   - `QListView` (Catálogo), `PlaylistView` (`QListView`) e `CartWallView` (`QTableView`).
3. **Delegates de Pintura Direta (`QStyledItemDelegate`)**:
   - `CatalogItemDelegate`, `FolderItemDelegate`, `PlaylistItemDelegate` e `CartWallItemDelegate`.
   - Toda a interface de cada item (gradientes, cantos arredondados, barras de progresso com *clip path*, badges `NO AR` / `PRÓXIMO`, ícones SVG e textos elidados) é desenhada sob demanda com `QPainter` apenas para os itens visíveis no *viewport*.

---

## Consequências

- **Positivas**:
  - **Desempenho Constante $O(1)$ na Renderização**: O Qt só chama `paint()` para as linhas atualmente visíveis na tela, independentemente de a lista ter 10 ou 50.000 itens.
  - **Atualização Leve de Progresso**: Atualizar a posição de reprodução de um item emite apenas `dataChanged(idx, idx, {PositionRole})`, repintando somente o retângulo daquele card.
  - **Desacoplamento Total**: Os modelos em `src/core/` não conhecem detalhes visuais e podem ser manipulados ou testados de forma independente.
- **Cuidados Necessários**:
  - Os métodos `paint()` e `sizeHint()` nos delegates devem ser extremamente rápidos, mantendo fontes, ícones e pincéis de gradiente (`QBrush`) previamente cacheados como membros da classe.

