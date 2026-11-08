# Ferramentas de Estúdio e Integrações Externas

Este documento descreve as ferramentas auxiliares de transmissão ao vivo e as integrações do **NaxStudio Flow** com serviços externos de áudio e vídeo.

---

## 🕒 Hora Certa

O recurso de **Hora Certa** permite anunciar o horário atual automaticamente (por meio de comandos na programação/transições) ou manualmente (via botão ou atalho de teclado):
- Seleciona automaticamente o arquivo de áudio correspondente à hora e ao minuto vigentes em uma pasta dedicada de Hora Certa e o reproduz respeitando a mixagem da programação.

---

## 📻 Exportação de Metadados para RDS e Streaming Online (`.txt`)

- Sempre que o item em estado `Playing` (`NO AR`) muda na Playlist, o Flow atualiza automaticamente um arquivo de texto (`.txt`) em um caminho configurável.
- Esse arquivo é lido em tempo real por softwares transmissores de **Streaming Online** (Icecast, Shoutcast, etc.) e codificadores de **RDS** de transmissores FM para exibir o nome da música ou programa no visor dos rádios e nos players web/aplicativos.
- É possível configurar quais categorias (`MediaType`) devem atualizar o arquivo (ex.: exibir o título da música durante faixas musicais ou o nome/slogan da rádio durante comerciais e vinhetas).

---

## 🎥 Integração com OBS Studio via WebSocket

Para emissoras que transmitem programação em vídeo (Rádio com Imagem / YouTube / Twitch), o Flow conecta-se ao **OBS Studio** através do protocolo `obs-websocket`.

### Automações Suportadas
- **Por Evento de Bloco**: Troca automática de cena no OBS quando a Playlist entra em um **Bloco Comercial** (ex.: mudando da câmera do estúdio para uma cena de patrocinadores/institucional) e retorno automático à câmera do estúdio quando inicia o **Bloco Musical**.
- **Por Item de Comando na Playlist**: Inserção de **Comandos do OBS** diretamente na Playlist (arrastados a partir do Catálogo), permitindo disparar sinais específicos via WebSocket em qualquer ponto exato entre músicas ou vinhetas.
- **Controle de Fontes**: Sincronização de fontes de áudio/vídeo conforme o estado da reprodução no Flow.

