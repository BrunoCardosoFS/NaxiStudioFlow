# Design System de UI e Internacionalização (i18n)

A interface do **NaxStudio Flow** foi desenhada especificamente para ambientes de estúdio de rádio: tema escuro de alto contraste (reduzindo a fadiga visual em jornadas longas), identificação imediata de categorias por cor e tipografia tabular para precisão de tempo.

---

## 🎨 Paleta de Cores Oficial

### Superfícies e Fundos (Dark Theme)

| Elemento | Hexadecimal | Uso Principal |
| :--- | :--- | :--- |
| **Fundo Principal da Janela** | `#191D24` | Background de `Flow`, `QDockWidget` e sulcos de sliders |
| **Fundo de Painéis e Blocos** | `#1F232C` | `DockTopWidget`, `PlayerControl`, `CartWallView`, `FilesListView` e contêiner de blocos na Playlist |
| **Fundo de Listas / Inputs** | `#2D3340` | Background da `PlaylistView`, `QLineEdit`, `QSpinBox`, `QComboBox`, `QPushButton` e itens normais do Catálogo |
| **Item Selecionado (Catálogo)** | `#3E4A5E` | Destaque de seleção em pastas e arquivos do Catálogo |
| **Item Hover (Catálogo)** | `#333A48` | Estado *mouse over* em pastas e arquivos |
| **Texto Principal** | `#FFFFFF` | Títulos, contadores de tempo e rótulos primários |
| **Texto Secundário / Muted** | `#8E9EB5` | Cabeçalhos de seção (`BLOCO COMERCIAL`), resumos de bloco e rótulos auxiliares |
| **Destaque Primário (Brand)** | `#FFA60A` | Relógio principal, aba selecionada, botões do topo e barra de sliders |
| **Indicador de Drag-and-Drop** | `#EAB308` | Linha de inserção na `PlaylistView` e na `CartWallView` |

### Gradientes por Tipo de Mídia (`MediaType`)

Tanto na **Playlist (`PlaylistItemDelegate`)** quanto na **Cartucheira (`CartWallItemDelegate`)**, os cards de áudio usam gradientes verticais padronizados para que o locutor reconheça a categoria da faixa instantaneamente:

| Categoria (`MediaType`) | Cor Superior (Top `0.0`) | Cor Inferior (Bottom `1.0`) | Ícone Associado |
| :--- | :--- | :--- | :--- |
| **Música (`Music = 1`)** | `#418DB4` (Azul claro) | `#0066A5` (Azul escuro) | `:/images/catalog/music.svg` |
| **Vinheta (`Jingle = 0`)** | `#68B127` (Verde claro) | `#3C8808` (Verde escuro) | `:/images/catalog/jingle.svg` |
| **Comercial (`Commercial = 2`)** | `#D97706` (Âmbar claro) | `#B45309` (Âmbar escuro) | `:/images/catalog/commercial.svg` |
| **Outros (`Other = 3`)** | `#475569` (Cinza ardósia) | `#334155` (Cinza escuro) | `:/images/catalog/other.svg` |
| **Barra de Progresso (`Playing`)** | `#CF3B6B` (Magenta/Rosa) | `#AC073C` (Vermelho rubi) | Preenchimento da esquerda p/ direita |

### Badges de Status na Playlist
- **`NO AR` (`Playing`)**: Fundo `#E50914` (vermelho vivo) com texto `#FFFFFF`.
- **`PRÓXIMO` (`Next`)**: Fundo `#F59E0B` (amarelo/âmbar) com texto `#000000`.

---

## 🔤 Tipografia e Formatação de Tempo

- **Fonte Padrão**: `Roboto` (tamanho base `10pt`), carregada de `:/fonts/fonts/Roboto-VariableFont_wdth,wght.ttf`.
- **Números Tabulares (`tnum`)**: Em [`main.cpp`](../../src/app/main.cpp), `globalFont.setFeature("tnum", 1)` é habilitado globalmente. Isso força todos os dígitos numéricos (`0` a `9`) a ocuparem exatamente a mesma largura em pixels, impedindo que contadores regressivos e cronômetros (`mm:ss:zzz`) " dancem" horizontalmente durante a reprodução.
- **Funções de Formatação ([`TimeUtils.h`](../../src/core/utils/TimeUtils.h))**:
  - `msec2string(qint64 msec)`: Retorna `mm:ss:zzz` (ou `hh:mm:ss:zzz` se $\ge 1\text{ hora}$). Usado na Playlist e no Editor de Mixagem.
  - `formatDuration(qint64 msec, bool forceHours)`: Retorna `mm:ss` (ou `hh:mm:ss` sem milissegundos). Usado nos cartuchos da Cartucheira e nos resumos longos de blocos.

---

## 🌍 Internacionalização (i18n)

O projeto suporta múltiplos idiomas utilizando o sistema de tradução nativo do Qt (`Qt6::LinguistTools`):

1. **Arquivos `.ts`**: Localizados em `src/lang/ts/` (`pt_BR.ts` e `en_US.ts`).
2. **Compilação Automática**: O arquivo [`src/lang/CMakeLists.txt`](../../src/lang/CMakeLists.txt) compila os arquivos `.ts` em `.qm` e os empacota nos recursos da aplicação sob o prefixo `:/lang/<locale>.qm`.
3. **Boas Práticas no Código**:
   - Em arquivos `.ui`, certifique-se de que propriedades `<string>` traduzíveis **não** estejam marcadas com `notr="true"` (use `notr="true"` apenas para valores fixos como `"00:00:00"` ou nomes próprios).
   - Em arquivos `.cpp`, utilize `tr("...")` para textos exibidos na interface.

