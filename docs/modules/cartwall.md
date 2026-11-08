# Módulo da Cartucheira (Cartwall)

A **Cartucheira (`CartWall`)** permite o disparo instantâneo de vinhetas, efeitos sonoros, prefixos, trilhas de fundo e comerciais com um único clique ou tecla de atalho. No **NaxStudio Flow**, as cartucheiras são vinculadas ao **Perfil ativo** (`data/profiles/<perfil>.json`).

---

## 🔢 Mapeamento Linear para Grade Dinâmica (`CartWallTableModel`)

Diferente de cartucheiras antigas com número fixo de botões vazios na tela, o `CartWallTableModel` armazena apenas os itens reais em uma lista contínua (`m_items`) e os distribui dinamicamente de acordo com o número de colunas escolhido pelo usuário (`cartWallColumns`, de 1 a 8 colunas):

- **Número de linhas**: $\lceil \text{itemCount} / \text{columns} \rceil$
- **Índice linear a partir da célula $(\text{row}, \text{col})$**:
  $$\text{linearIndex} = \text{row} \times \text{columns} + \text{col}$$
- **Células após o fim da lista ($\text{linearIndex} \ge \text{itemCount}$)**:
  - Retornam `-1` em `linearIndex()`, `Qt::NoItemFlags` em `flags()` e não são desenhadas pelo `CartWallItemDelegate`.
  - Assim, se houver 8 itens em uma grade de 3 colunas, a última linha exibirá 2 cartuchos e a 3ª célula ficará totalmente invisível e inativa (com cursor padrão de seta).

---

## 🖱️ Interação, Disparo e Drag-and-Drop (`CartWallView`)

### 1. Diferenciação entre Clique (Disparo) e Arrasto
No `CartWallView`, o usuário pode tanto **clicar** em um cartucho para disparar o áudio quanto **arrastá-lo** para mudar sua posição na grade:
- No `mousePressEvent`, o índice pressionado (`m_pressedIndex`) e a posição inicial (`m_pressPos`) são registrados.
- Se o mouse se mover além de `QApplication::startDragDistance()`, inicia-se a operação de *Drag-and-Drop*.
- Se o botão esquerdo for solto (`mouseReleaseEvent`) sobre a mesma célula sem ter excedido a distância de arrasto, o sinal `slotTriggered(int linearIndex)` é emitido imediatamente.

### 2. Reordenação e Adição via Catálogo
- **Arrasto Interno (`Qt::MoveAction`)**: Permite reordenar cartuchos dentro da própria grade.
- **Arrasto do Catálogo (`Qt::CopyAction`)**: Aceita itens com MIME `application/x-catalog-item`, inserindo o novo cartucho exatamente no slot calculado pela posição horizontal do cursor (esquerda ou direita do centro da célula alvo) ou anexando ao final se solto na área vazia.
- Durante o arrasto, `paintEvent()` desenha uma barra vertical amarela (`#eab308`) indicando a fronteira exata onde o cartucho será inserido.

---

## 🔁 Recursos de Reprodução: Independente, Loop e Encadeamento (`Ctrl + Clique`)

1. **Disparo Independente (Multi-Voz)**:
   - Cada slot da cartucheira toca de maneira independente, permitindo sobrepor vários efeitos ou uma trilha de fundo com carimbos de voz.
2. **Modo Loop**:
   - Slots podem ser configurados em modo de repetição contínua até que sejam interrompidos pelo operador.
3. **Encadeamento Sequencial (`Ctrl + Clique`)**:
   - Segurando a tecla `Ctrl` e clicando em múltiplos cartuchos em sequência (ex.: Slot 1 $\rightarrow$ Slot 5 $\rightarrow$ Slot 3), o operador cria uma fila de reprodução encadeada na cartucheira.
   - Ao iniciar a reprodução do primeiro slot da sequência, o próximo slot encadeado será disparado automaticamente ao atingir o ponto de mixagem (`mixEnd`) do anterior, respeitando as curvas de transição.

