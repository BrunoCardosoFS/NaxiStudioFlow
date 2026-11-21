# Roadmap de Desenvolvimento - NaxStudio Flow

> **Fase Atual do Projeto: `Alpha (0.x.y)`** — Arquitetura em evolução ativa; formatos de arquivos e configurações ainda podem sofrer alterações até a entrada na fase Beta (`1.0.0-beta.1`). Consulte as regras completas de versionamento em [`conventions/git-and-workflow.md`](conventions/git-and-workflow.md#-sistema-de-versionamento-versioncmake).

Abaixo estão listados os marcos de desenvolvimento do **NaxStudio Flow**, os checklists de entrega para as versões `1.0.0-beta.1` e `1.0.0-rc.1`, e a previsão para a versão `2.0.0`.

**Legenda de Status:**
- ✅ Finalizado
- 🟡 Fazendo
- 🔴 Não iniciado

---

## 📊 Marcos de Desenvolvimento

| # | Marco | Status |
| :---: | :--- | :---: |
| **M0** | Estrutura base da janela principal (`Flow`), painéis acopláveis (`QDockWidget`), relógio em tempo real com fonte tabular (`Roboto tnum`), persistência de layout (`QSettings`) e base de traduções (`.ts`/`.qm`).<br>**Docs:** [Visão Geral](architecture/overview.md#-responsabilidades-de-cada-diretório-em-src) · [UI e i18n](conventions/ui-and-i18n.md) | ✅ |
| **M1** | Catálogo de mídia com arquitetura Model/View/Delegate (`CatalogWidget`, `FolderListModel`, `CatalogListModel`), categorização de pastas a partir de `catalog.json` e escaneamento assíncrono multithread (`CatalogScanner` com cancelamento atômico).<br>**Docs:** [Catálogo](modules/catalog.md) · [Model/View/Delegate](architecture/overview.md#-arquitetura-modelviewdelegate-na-interface) | ✅ |
| **M2** | Interface e modelo da Playlist (`PlaylistListModel`, `PlaylistView`, `PlaylistItemDelegate`) com blocos arredondados, cálculo de durações e horários estimados, trava de segurança (`row <= playingRow`) e reordenação interna via Drag-and-Drop.<br>**Docs:** [Playlist](modules/playlist.md) · [Design System](conventions/ui-and-i18n.md#-paleta-de-cores-oficial) | ✅ |
| **M3** | Interface e modelo da Cartucheira (`CartWallTableModel`, `CartWallView`, `CartWallItemDelegate`) com mapeamento linear para grade dinâmica de colunas, reordenação interna e recepção de mídias arrastadas do Catálogo.<br>**Docs:** [Cartucheira](modules/cartwall.md) · [Drag-and-Drop (Catálogo)](modules/catalog.md#-protocolo-de-drag-and-drop-applicationx-catalog-item) | ✅ |
| **M4** | Player individual de áudio (`PlayerManager`) com automação de volume em curva *equal-power* ($\sin(t \cdot \pi/2)$), *fast-path* de platô e controle de pontos de corte e fade.<br>**Docs:** [Automação de Volume](architecture/audio-engine.md#-automação-de-volume-e-curva-equal-power-playermanager) | ✅ |
| **M5** | Orquestração do `PlaylistManager` com rodízio circular de 3 players, sincronismo antecipado de mixagem ($\text{mixEnd}_{\text{atual}} \leftrightarrow \text{mixStart}_{\text{próximo}}$), pré-carregamento estendido dinâmico, transição de blocos e conexão dos botões de transporte (`Play`, `Pause`, `Stop`, `Next`).<br>**Docs:** [Sincronismo de Mixagem](architecture/audio-engine.md#-sincronismo-dos-pontos-de-mixagem-mixend--mixstart) · [Rodízio de Players](architecture/audio-engine.md#-rodízio-de-players-na-playlist-playlistmanager) | ✅ |
| **M6** | Editor de Mixagem (`MixEditor`) com abertura via menu de contexto (Catálogo e Playlist), visualização da forma de onda (*waveform*), ajuste de pontos de corte/mixagem/fade/envelope de volume e salvamento dual.<br>**Docs:** [Editor de Mixagem](modules/mix-editor.md) · [Hierarquia de Metadados](architecture/server-sync-and-storage.md#-hierarquia-de-metadados-de-mixagem-musicamp3json-vs-inserção) | 🟡 |
| **M7** | Drag-and-Drop de arquivos do Catálogo diretamente para dentro dos blocos da Playlist, leitura da duração real da mídia e remoção de itens futuros.<br>**Docs:** [Drag-and-Drop (Playlist)](modules/playlist.md#-regras-de-segurança-e-drag-and-drop) · [Protocolo MIME (Catálogo)](modules/catalog.md#-protocolo-de-drag-and-drop-applicationx-catalog-item) | ✅ |
| **M8** | Carregamento dinâmico da Playlist (janela deslizante padrão de 4 blocos futuros e histórico de 10 inserções finalizadas considerando apenas mídias e comandos, ajustáveis nas configurações) e botão de *Abrir Programação* por data e horário.<br>**Docs:** [Carregamento Dinâmico e Histórico](modules/playlist.md#-carregamento-dinâmico-de-blocos-abrir-programação-e-retenção-de-histórico) · [Configurações](modules/settings-and-profiles.md#-janela-de-configurações-btnsettings) | 🔴 |
| **M9** | Suporte a itens do tipo **Comando** no Catálogo e na Playlist.<br>**Docs:** [Comandos no Catálogo](modules/catalog.md#itens-de-comando-no-catálogo) · [Anatomia de Bloco](modules/playlist.md#-anatomia-de-um-bloco-na-playlist) | 🔴 |
| **M10** | Padronização da estrutura de diretórios locais (`Flow/bin/` e `Flow/data/`) e suporte a caminhos espelho locais (*mirror paths*) no catálogo.<br>**Docs:** [Estrutura `Flow/bin/` e `data/`](architecture/overview.md#-estrutura-de-instalação-e-diretório-data) · [Espelho Local e Cascata](architecture/server-sync-and-storage.md#-resolução-de-arquivos-de-mídia-em-cascata-rede-e-espelho-local) | 🔴 |
| **M11** | Leitura e gravação de metadados globais de mixagem em arquivos JSON irmãos (`musica.mp3.json`) e resolução de prioridade entre metadados da inserção e metadados globais.<br>**Docs:** [Hierarquia de Metadados](architecture/server-sync-and-storage.md#-hierarquia-de-metadados-de-mixagem-musicamp3json-vs-inserção) · [Salvamento no MixEditor](modules/mix-editor.md#-modalidades-de-salvamento-global-vs-inserção-local) | 🔴 |
| **M12** | Motor de reprodução de áudio da Cartucheira com players independentes por slot (disparo simultâneo), modo Loop individual e fila de reprodução encadeada respeitando os pontos de mixagem.<br>**Docs:** [Motor de Áudio da Cartucheira](architecture/audio-engine.md#-motor-de-áudio-da-cartucheira-cartwall) · [Recursos de Reprodução](modules/cartwall.md#-recursos-de-reprodução-independente-loop-e-encadeamento-ctrl--clique) | 🔴 |
| **M13** | Roteamento de áudio para múltiplas placas de som independentes e implementação do player de Pré-escuta integrado ao Catálogo.<br>**Docs:** [Roteamento de Placas de Som](architecture/audio-engine.md#-roteamento-para-múltiplas-placas-de-som) · [Configurações de Áudio](modules/settings-and-profiles.md#-janela-de-configurações-btnsettings) | 🔴 |
| **M14** | Sistema de Perfis com controle de permissões e vínculo dos slots da Cartucheira por perfil.<br>**Docs:** [Sistema de Perfis](modules/settings-and-profiles.md#-sistema-de-perfis-dataprofiles) · [Escopo de Armazenamento](architecture/server-sync-and-storage.md#-escopo-de-armazenamento-dos-perfis-e-cartucheira) | 🔴 |
| **M15** | Janela de Configurações para seleção das placas de áudio, mapeamento de caminhos do catálogo, limites de blocos/histórico da playlist, configuração do servidor, seleção de idioma e preferências gerais.<br>**Docs:** [Janela de Configurações](modules/settings-and-profiles.md#-janela-de-configurações-btnsettings) · [Internacionalização (i18n)](conventions/ui-and-i18n.md#-internacionalização-i18n) | 🔴 |
| **M16** | Log de Sistema e Diagnóstico em arquivos `.txt` registrando carregamento de módulos, status de conexão com servidor e erros internos.<br>**Docs:** [Log de Sistema (`.txt`)](modules/logging.md#-log-de-sistema-e-diagnóstico-txt) | 🔴 |
| **M17** | Log de Programação e Execução em SQLite (`.db`) com sincronização acumulada para o servidor.<br>**Docs:** [Log de Programação (SQLite)](modules/logging.md#-log-de-programação-e-execução-sqlite-db--sincronização-com-servidor) · [Padrão *Outbox*](architecture/server-sync-and-storage.md#-sincronização-acumulada-de-logs-de-programação-padrão-outbox) | 🔴 |
| **M18** | Integração com o **NaxServer** via API REST mantendo operação *Offline-First*, busca de mídia em cascata, merge não-destrutivo de alterações locais nos blocos e função de *Restaurar Bloco*.<br>**Docs:** [Sincronização com NaxServer](architecture/server-sync-and-storage.md#-dinâmica-servidor-vs-operação-local-data) · [Resolução em Cascata](architecture/server-sync-and-storage.md#-resolução-de-arquivos-de-mídia-em-cascata-rede-e-espelho-local) | 🔴 |
| **M19** | Módulo de anúncio automático e manual de Hora Certa e suporte a atalhos de teclado globais para controle da Playlist e disparo rápido da Cartucheira.<br>**Docs:** [Hora Certa](modules/integrations-and-tools.md#-hora-certa) · [Atalhos de Teclado](modules/settings-and-profiles.md#-janela-de-configurações-btnsettings) | 🔴 |
| **M20** | Exportação automática dos metadados da faixa no ar em arquivo `.txt` para integração com geradores de RDS e transmissores de Streaming Online.<br>**Docs:** [Exportação RDS / Streaming](modules/integrations-and-tools.md#-exportação-de-metadados-para-rds-e-streaming-online-txt) | 🔴 |
| **M21** | Integração com o OBS Studio via protocolo WebSocket (`obs-websocket`) para troca automatizada de cenas, controle de fontes e execução de comandos da playlist.<br>**Docs:** [Integração OBS Studio](modules/integrations-and-tools.md#-integração-com-obs-studio-via-websocket) · [Comandos na Playlist](modules/playlist.md#-anatomia-de-um-bloco-na-playlist) | 🔴 |
| **M22** | Desenvolvimento de engine de áudio própria utilizando diretamente as bibliotecas do FFmpeg para substituir o `Qt6::Multimedia` (planejado para a versão 2.0.0).<br>**Docs:** [Motor de Áudio (Visão v2.0)](architecture/audio-engine.md) | 🔴 |

---

## 🧪 Checklist Geral - Primeiro Beta (`v1.0.0-beta.1`)

O objetivo do **Primeiro Beta** é entregar toda a operação local de estúdio funcional, permitindo programar, mixar e transmitir blocos musicais, comerciais e cartucheira sem dados mockados:

- [ ] Estrutura de diretórios `Flow/bin/` e `Flow/data/` ativa por padrão.
- [x] Reprodução contínua da Playlist no `PlaylistManager` com rodízio dos 3 players, sincronismo antecipado ($\text{mixEnd} \leftrightarrow \text{mixStart}$) e pré-carregamento dinâmico.
- [ ] Carregamento dinâmico da Playlist e botão de *Abrir Programação* por data/hora.
- [x] Controles de transporte (`Play`, `Pause`, `Stop`, `Next`) e atualização da barra de progresso em tempo real na Playlist.
- [x] Adição de mídias do Catálogo para a Playlist via Drag-and-Drop (lendo a duração real do áudio) e remoção de itens futuros.
- [ ] Suporte aos itens de **Comando** essenciais na Playlist (como o comando de *Pausa* e *Hora Certa*).
- [ ] Leitura e escrita de metadados de mixagem nos arquivos irmãos `musica.mp3.json` e nos itens dos blocos locais (`data/blocks/`).
- [ ] Editor de Mixagem (`MixEditor`) funcional com visualização de forma de onda (*waveform*) e salvamento global ou por inserção.
- [ ] Reprodução real de áudio na Cartucheira, incluindo disparo independente simultâneo, modo Loop e encadeamento com `Ctrl + Clique`.
- [ ] Seleção de placas de som separadas e player de Pré-escuta no Catálogo.
- [ ] Remoção completa de todos os carregamentos estáticos de teste.

---

## 🏁 Checklist Geral - Release Candidate da Versão Estável (`v1.0.0-rc.1` / `1.0.0`)

O objetivo do **Release Candidate** é consolidar todas as integrações externas, comunicação em rede com o `NaxServer`, controle de acesso e auditoria para operação 24/7 em produção:

- [ ] Todos os itens do Beta concluídos, testados e estabilizados.
- [ ] Integração completa via API REST com o **NaxServer** (sincronização de `catalog.json` e programação em `data/blocks/`).
- [ ] Funcionamento comprovado do modo *Offline-First*.
- [ ] Algoritmo de **Merge Não-Destrutivo** preservando inserções e edições locais na Playlist ao receber atualizações do servidor, acompanhado da opção de *Restaurar Bloco*.
- [ ] Resolução de mídia em cascata funcionando (caminho primário $\rightarrow$ caminho espelho local $\rightarrow$ carregamento via rede pelo `NaxServer`).
- [ ] Sistema de Perfis com alternância de Cartucheira em tempo real e controle de permissões.
- [ ] Janela de Configurações finalizada com todas as opções operacionais.
- [ ] Gravação confiável de Logs Críticos de Execução para comprovação comercial e ECAD.
- [ ] Exportação automática de arquivo `.txt` para RDS e Streaming Online.
- [ ] Integração com **OBS Studio** via WebSocket funcionando tanto por eventos de bloco quanto por itens de **Comando** inseridos na Playlist.
- [ ] Atalhos de teclado globais configurados e traduções (`pt_BR` e `en_US`) revisadas.

---

## 🚀 Previsão para a Versão `2.0.0`

A versão **2.0.0** terá como foco principal a evolução tecnológica do núcleo de processamento de áudio do **NaxStudio Flow**. Enquanto toda a série `1.x` utiliza o módulo `Qt6::Multimedia`, a versão `2.0.0` substituirá essa camada por uma **engine de áudio própria desenvolvida diretamente sobre as bibliotecas nativas do FFmpeg**.

Com a engine própria na versão `2.0.0`, está previsto:
- **Independência do `Qt6::Multimedia`**: Controle direto de baixo nível sobre decodificação, buffers de memória, threads de áudio de alta prioridade e dispositivos de saída.
- **Precisão Sample-Accurate nas Transições**: Mixagem, pontos de corte (`startPoint`, `endPoint`, `mixStart`, `mixEnd`) e curvas de envelope processados diretamente no fluxo de amostras PCM, garantindo latência mínima e sincronismo absoluto.
- **Geração Integrada de Waveform e Análise de Áudio**: Aproveitamento do mesmo pipeline FFmpeg para renderização instantânea de formas de onda, detecção automática de silêncio nos extremos da faixa e normalização/processamento dinâmico de áudio.
