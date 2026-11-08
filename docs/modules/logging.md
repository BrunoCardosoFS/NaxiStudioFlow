# Sistema de Logs

O **NaxStudio Flow** separa o registro de eventos em duas categorias distintas dentro de `Flow/data/logs/`: um log técnico voltado ao diagnóstico do software e um log de programação/irradiação voltado à comprovação comercial e auditoria.

---

## 🛠️ Log de Sistema e Diagnóstico (`.txt`)

Voltado para depuração técnica e monitoramento da saúde do software. É gravado em arquivos de texto puro (`.txt`) e registra:
- Inicialização da aplicação e carregamento de módulos internos.
- Status de conexão, tentativas de reconexão e respostas da API do `NaxServer`.
- Alertas de hardware (mudança ou falha em placas de áudio `QAudioDevice`).
- Erros de leitura de arquivos de mídia, falhas de parsing ou erros de comunicação WebSocket (OBS Studio).

---

## 📜 Log de Programação e Execução (SQLite `.db` + Sincronização com Servidor)

Na radiodifusão comercial e comunitária, o registro fiel de tudo o que foi ao ar é **crucial** para comprovação de irradiação junto aos anunciantes e relatórios de direitos autorais (ECAD).

### Armazenamento Local Transacional e Fila de Envio (*Outbox*)
- Cada execução é salva imediatamente no banco local (`data/logs/playout_logs.db`) com status de sincronização pendente.
- Em seguida, o Flow envia o registro para o `NaxServer`. Caso ocorra qualquer erro de rede ou o servidor esteja indisponível, os registros permanecem marcados como não sincronizados no banco local e, na próxima conexão bem-sucedida, o Flow envia automaticamente todo o lote acumulado.

### Dados Registrados por Inserção Executada
- **Horário Exato**: Data e hora precisa de início e término da execução.
- **Modo de Acionamento (`Automático` vs. `Mecânico`)**: Indica se a inserção tocou de forma automática pela sequência da Playlist ou de forma mecânica/manual (acionada pelo operador na Playlist ou na Cartucheira).
- **Responsável pela Adição / Operação**:
  - Se a inserção veio programada do servidor: registra o **usuário do servidor** que a cadastrou/agendou.
  - Se a inserção foi adicionada ou disparada localmente no Flow: registra o **perfil do Flow** ativo no momento.
- **ID da Inserção no Banco do Servidor (`serverInsertionId`)**: Quando o item faz parte da programação enviada pelo `NaxServer`, seu ID único de agendamento é vinculado ao log. Assim, ao receber o log de volta, o servidor identifica exatamente que aquele comercial ou música programado para aquela data e horário específicos foi efetivamente irradiado.
- **Metadados da Mídia**: Título, caminho/ID do arquivo, categoria (`MediaType`) e bloco de origem.

