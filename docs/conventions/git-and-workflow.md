# Convenções de Git, Commits e Versionamento

Este documento descreve o padrão de histórico de versionamento, organização de branches e controle de versão CMake do **NaxStudio Flow**.

---

## 📝 Padrão de Mensagens de Commit (Conventional Commits)

Todos os commits devem seguir a especificação **Conventional Commits** redigida em **inglês**:

```text
<type>(<scope>): <imperative summary>

- Detailed change 1
- Detailed change 2
```

### Regras do Título (Primeira Linha)
1. **Tipo (`<type>`)**:
   - `feat`: Adição de uma nova funcionalidade.
   - `fix`: Correção de um bug.
   - `refactor`: Reestruturação de código interno sem alterar o comportamento final.
   - `style`: Ajustes de estilos visuais (QSS, cores, espaçamentos) ou formatação de código.
   - `perf`: Otimizações de performance (CPU, memória, renderização, I/O).
   - `docs`: Criação ou atualização de arquivos de documentação (`docs/`, `README.md`, `CONTRIBUTING.md`).
   - `chore`: Atualizações de build (`CMakeLists.txt`), `.gitignore`, CI/CD ou dependências.
2. **Escopo (`<scope>`)**:
   - Indica a camada ou módulo afetado: `core`, `ui`, `playlist`, `catalog`, `cartwall`, `editor`, `player`, `build`, `lang`.
3. **Resumo no Imperativo**:
   - Escrito em letras minúsculas, sem ponto final, no modo imperativo em inglês (ex.: `add`, `implement`, `fix`, `remove`, `refactor`).

### Corpo do Commit
- Quando o commit envolver múltiplas alterações lógicas no arquivo ou módulo, liste cada mudança com marcadores (`- `) no corpo do commit (conforme praticado em todo o histórico recente do repositório).

---

## 🌿 Estratégia de Branches

- **`main`**: Branch principal do projeto. Deve sempre compilar limpa em todos os sistemas operacionais suportados.
- **Branches de Desenvolvimento**:
  - `feat/<nome-curto>` - Ex.: `feat/playlist-manager-playback`
  - `fix/<nome-curto>` - Ex.: `fix/catalog-search-cancel`
  - `refactor/<nome-curto>` - Ex.: `refactor/mix-editor-waveform`
  - `docs/<nome-curto>` - Ex.: `docs/architecture-guides`

---

## 🏷️ Sistema de Versionamento (`version.cmake`)

O **NaxStudio Flow** encontra-se atualmente na fase **Alpha** e adota o **versionamento semântico (`MAJOR.MINOR.PATCH`)** com sufixos padronizados de pré-lançamento.

### 1. Ciclo de Versões Antes da `1.0.0`

| Fase | Versão | Significado |
| :--- | :--- | :--- |
| **Alpha** *(Estado Atual)* | `0.1.0`, `0.2.0`, `0.3.1`... | Arquitetura ainda mudando; pode quebrar o formato de arquivos e configurações. |
| **Beta** | `1.0.0-beta.1`, `1.0.0-beta.2`... | Funcionalidades completas para a `1.0.0`, faltando estabilizar e testar. |
| **Release Candidate** | `1.0.0-rc.1`... | Apenas correção de bugs; se não aparecer nenhum problema crítico, torna-se a `1.0.0`. |
| **Estável** | `1.0.0` | Compromisso de estabilidade e de não quebrar compatibilidade sem aumentar o `MAJOR`. |

### 2. Ciclo de Versões Depois da `1.0.0`

Após o lançamento da versão `1.0.0`, os sufixos de pré-release continuam sendo utilizados, sempre se referindo à **versão alvo que será lançada em seguida**:

- **Novas Funcionalidades (`MINOR`)**:
  - `1.2.0-beta.1` indica o primeiro *beta* do que se tornará a versão `1.2.0`.
- **Correções (`PATCH`)**:
  - Para versões de *patch* normalmente não deverá ser realizado ciclo de beta, será lançado diretamente a versão final `1.2.9`.
- **Mudanças Estruturais Grandes (`MAJOR` / `-alpha`)**:
  - O sufixo `-alpha` após a `1.0.0` é reservado para mudanças arquiteturais profundas (como um futuro `2.0.0-alpha.1` durante a implementação da engine de áudio própria com FFmpeg).

### 3. Configuração no CMake (`version.cmake`)

A versão da aplicação é injetada na compilação pelo arquivo [`version.cmake`](../../version.cmake) na raiz do repositório:

- Variáveis CMake: `VERSION_MAJOR`, `VERSION_MINOR` e `VERSION_PATCH`.
- O script gera automaticamente:
  - `CLEAN_APP_VERSION`: `"X.Y.Z"` (usado na definição do projeto CMake e metadados do executável).
  - `APP_VERSION`: `"VX.Y.Z"` (injetado no C++ via macro de pré-processador `-DAPP_VERSION="VX.Y.Z"` e exibido na barra de título da janela `Flow`).
- Em pipelines de CI/CD (GitHub Actions), os valores podem ser passados diretamente na linha de comando do CMake:
  ```bash
  cmake -B build -S . -DVERSION_MAJOR=1 -DVERSION_MINOR=0 -DVERSION_PATCH=0
  ```

