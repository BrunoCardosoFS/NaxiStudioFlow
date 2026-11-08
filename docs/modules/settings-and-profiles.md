# Perfis e Configurações

O **NaxStudio Flow** oferece gerenciamento de perfis operacionais e uma central de configurações locais da estação para adaptar o comportamento do software à rotina de cada emissora.

---

## 👤 Sistema de Perfis (`data/profiles/`)

Cada emissora organiza sua operação de maneira própria — algumas separam o ambiente por **locutor/operador**, outras por **programa da grade**. O seletor de perfis (`selectProfile`) na barra superior (`DockTop`) gerencia essa separação.

### Características dos Perfis
- **Armazenamento Local**: Cada perfil possui seu próprio arquivo JSON dentro de `Flow/data/profiles/<id-do-perfil>.json`.
- **Vínculo com a Cartucheira**: Os cartuchos da `CartWall` pertencem ao perfil. Quando um locutor seleciona seu perfil no topo da janela, a cartucheira carrega imediatamente os efeitos, trilhas e vinhetas daquele perfil.
- **Níveis de Permissão**:
  - **Perfil Administrador**: Possui acesso completo à janela de Configurações (`btnSettings`), mapeamento de caminhos do catálogo, configuração de placas de áudio, parâmetros do `NaxServer` e criação/edição de outros perfis.
  - **Perfil Operador / Programa**: Focado na operação de estúdio (Playlist, Catálogo, Cartucheira e MixEditor), com bloqueio de alterações nas configurações críticas do sistema.

---

## ⚙️ Janela de Configurações (`btnSettings`)

Acionada pelo botão de engrenagem no `DockTop`, concentra as configurações técnicas da máquina local (salvas em `Flow/data/config/`):

1. **Dispositivos de Áudio**:
   - Seleção da placa de som para a **Playlist (Principal)**.
   - Seleção da placa de som para a **Cartucheira**.
   - Seleção da placa de som para a **Pré-escuta (CUE)**.
2. **Caminhos do Catálogo e Espelho Local**:
   - Definição de caminhos locais alternativos (*mirror paths*) para as pastas sincronizadas no `catalog.json`.
3. **Conexão com o `NaxServer`**:
   - Endereço da API REST, credenciais da estação e status de sincronização.
4. **Playlist e Programação**:
   - **Blocos Carregados Simultaneamente**: Define a quantidade de blocos futuros mantidos na lista da Playlist (padrão: `4` blocos; ao finalizar um bloco, o próximo é carregado automaticamente no final).
   - **Histórico de Inserções Finalizadas**: Define quantas inserções já executadas (`Finished`) permanecem visíveis no topo da Playlist (padrão: `10` inserções, contabilizando apenas itens de mídia e comandos, sem contar marcadores de bloco).
5. **Atalhos de Teclado e Geral**:
   - Mapeamento de teclas rápidas para transporte e disparo de cartuchos, seleção de idioma (`pt_BR` / `en_US`) e configuração da Hora Certa.

