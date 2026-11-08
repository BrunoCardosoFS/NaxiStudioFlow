# Perfis, Configurações, Logs e Integrações Externas

Além dos módulos centrais de reprodução, o **NaxStudio Flow** conta com subsistemas essenciais para a rotina de uma emissora de rádio: controle de acesso por perfil, roteamento de hardware, auditoria legal de execução (ECAD/comerciais) e integração com transmissores e softwares de vídeo.

---

## 👤 Sistema de Perfis (`data/profiles/`)

Cada emissora organiza sua operação de maneira própria - algumas separam o ambiente por **locutor/operador**, outras por **programa da grade**. O seletor de perfis (`selectProfile`) na barra superior (`DockTop`) gerencia essa separação.

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
4. **Atalhos de Teclado e Geral**:
   - Mapeamento de teclas rápidas para transporte e disparo de cartuchos, seleção de idioma (`pt_BR` / `en_US`) e configuração da Hora Certa.

---

## 🕒 Hora Certa

O recurso de **Hora Certa** permite anunciar o horário atual automaticamente (em transições programadas) ou manualmente (via botão/atalho):
- Seleciona automaticamente o arquivo de áudio correspondente à hora e ao minuto vigentes em uma pasta dedicada de Hora Certa e o reproduz respeitando a mixagem da programação.

---

## 📜 Sistema de Logs (`data/logs/`)

O **NaxStudio Flow** separa o registro de eventos em duas categorias distintas dentro de `Flow/data/logs/`:

### 1. Log de Sistema e Diagnóstico (`.txt`)
Voltado para depuração técnica e monitoramento da saúde do software. É gravado em arquivos de texto puro (`.txt`) e registra:
- Inicialização da aplicação e carregamento de módulos internos.
- Status de conexão, tentativas de reconexão e respostas da API do `NaxiServer`.
- Alertas de hardware (mudança ou falha em placas de áudio `QAudioDevice`).
- Erros de leitura de arquivos de mídia, falhas de parsing ou erros de comunicação WebSocket (OBS Studio).

### 2. Log de Programação e Execução (SQLite `.db` + Sincronização com Servidor)
Na radiodifusão, o registro fiel de tudo o que foi ao ar é **crucial** para comprovação de irradiação junto aos anunciantes e relatórios de direitos autorais.

- **Armazenamento Local Transacional (SQLite `.db`) e Fila de Envio (*Outbox*)**:
  - Cada execução é salva imediatamente no banco local (`data/logs/playout_logs.db`) com status de sincronização pendente.
  - Em seguida, o Flow envia o registro para o `NaxiServer`. Caso ocorra qualquer erro de rede ou o servidor esteja indisponível, os registros permanecem marcados como não sincronizados no banco local e na próxima conexão bem-sucedida o Flow envia automaticamente todo o lote acumulado.
- **Dados Registrados por Inserção Executada**:
  - **Horário Exato**: Data e hora precisa de início e término da execução.
  - **Modo de Acionamento (`Automático` vs. `Mecânico`)**: Indica se a inserção tocou de forma automática pela sequência da Playlist ou de forma mecânica/manual (acionada pelo operador na Playlist ou na Cartucheira).
  - **Responsável pela Adição / Operação**:
    - Se a inserção veio programada do servidor: registra o **usuário do servidor** que a cadastrou/agendou.
    - Se a inserção foi adicionada ou disparada localmente no Flow: registra o **perfil do Flow** ativo no momento.
  - **ID da Inserção no Banco do Servidor (`serverInsertionId`)**: Quando o item faz parte da programação enviada pelo `NaxiServer`, seu ID único de agendamento é vinculado ao log. Assim, ao receber o log de volta, o servidor identifica exatamente que aquele comercial ou música programado para aquela data e horário específicos foi efetivamente irradiado.
  - **Metadados da Mídia**: Título, caminho/ID do arquivo, categoria (`MediaType`) e bloco de origem.

---

## 📡 Integrações Externas: RDS, Streaming (`.txt`) e OBS Studio

### 1. Exportação de Metadados para RDS e Streaming Online (`.txt`)
- Sempre que o item em estado `Playing` (`NO AR`) muda na Playlist, o Flow atualiza automaticamente um arquivo de texto (`.txt`) em um caminho configurável.
- Esse arquivo é lido em tempo real por softwares transmissores de **Streaming Online** (Icecast, Shoutcast, etc.) e codificadores de **RDS** de transmissores FM para exibir o nome da música/programa no visor dos rádios e nos players web/aplicativos.
- É possível configurar quais categorias (`MediaType`) devem atualizar o arquivo (ex.: exibir nome da música, ou exibir o slogan da rádio durante comerciais e vinhetas).

### 2. Integração com OBS Studio via WebSocket
- Para emissoras que transmitem programação em vídeo (Rádio com Imagem / YouTube / Twitch), o Flow conecta-se ao **OBS Studio** através do protocolo `obs-websocket`.
- **Automações suportadas**:
  - Troca automática de cena no OBS quando a Playlist entra em um **Bloco Comercial** (ex.: mudando da câmera do estúdio para uma cena de patrocinadores/institucional) e retorno automático à câmera do estúdio quando inicia o **Bloco Musical**.
  - Inserção de **Comandos do OBS** diretamente na Playlist (arrastados a partir do Catálogo), permitindo disparar sinais específicos via WebSocket em qualquer ponto exato entre músicas ou vinhetas.
  - Sincronização de fontes de áudio/vídeo conforme o estado da reprodução no Flow.

