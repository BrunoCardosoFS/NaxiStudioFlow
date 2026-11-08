# ADR 004: Scanner de Catálogo Lock-Free em Thread Dedicada

- **Status**: Aceito e Implementado
- **Contexto**: `src/core/catalog/CatalogScanner.h` e `CatalogScanner.cpp`

---

## Contexto e Problema

Quando o operador alterna rapidamente entre pastas do catálogo ou digita um termo de busca para pesquisar em todas as pastas do acervo (busca global), o sistema precisa varrer diretórios com milhares de arquivos de áudio (`.mp3`, `.wav`, `.flac`, etc.).

Se uma busca longa for iniciada na Pasta A e, 50ms depois, o usuário clicar na Pasta B:
1. A thread da interface gráfica (GUI) não pode bloquear em nenhum momento.
2. A varredura antiga da Pasta A deve ser abortada imediatamente para não desperdiçar I/O de disco/rede.
3. Resultados atrasados da Pasta A jamais podem sobrescrever a lista da Pasta B na tela (*race condition*).

---

## Decisão

Implementar o par `CatalogScanner` (na thread principal) e `CatalogScannerWorker` (em uma `QThread` dedicada `m_workerThread`) coordenados por identificadores monotônicos de requisição e variáveis atômicas (`QAtomicInt`):

1. **ID de Requisição Monotônico (`m_currentRequestId`)**:
   - A cada nova chamada de `scanLocal()` ou `scanGlobal()`, o `CatalogScanner` cancela a requisição anterior e incrementa atomicamente `m_currentRequestId`.
2. **Cancelamento Imediato Lock-Free (`m_cancelledRequestId`)**:
   - O sinal `requestCancel(current)` está conectado ao slot `CatalogScannerWorker::cancel()` via `Qt::DirectConnection`, gravando o ID cancelado em `m_cancelledRequestId` com semântica `storeRelease()` instantaneamente, mesmo enquanto o worker está dentro do loop do `QDirIterator`.
   - A cada arquivo iterado no `QDirIterator`, o worker verifica `m_cancelledRequestId.loadAcquire() >= requestId` e retorna imediatamente se a busca tiver sido substituída.
3. **Descarte Seguro na Recepção (`onScanFinished`)**:
   - Quando o sinal `scanFinished` chega de volta à thread principal via `Qt::QueuedConnection`, `onScanFinished` compara `requestId == m_currentRequestId.loadAcquire()`, descartando qualquer resultado obsoleto.

---

## Consequências

- **Positivas**:
  - Interface 100% fluida mesmo durante buscas globais em dezenas de pastas ou diretórios mapeados em rede.
  - Cancelamento instantâneo sem necessidade de `QMutex` ou bloqueios de thread.
  - Garantia matemática contra exibição de arquivos da pasta errada ao clicar rapidamente entre várias pastas.

