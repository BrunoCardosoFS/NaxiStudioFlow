# ADR 003: Arquitetura Offline-First, Merge Local de Blocos e Sidecar JSON

- **Status**: Aceito
- **Contexto**: Persistência de dados em `Flow/data/`, integração com `NaxServer` e metadados de mixagem

---

## Contexto e Problema

Emissoras de rádio não podem depender de conectividade de rede 100% estável para manter sua programação no ar. Se o servidor central (`NaxServer`) reiniciar, sofrer manutenção ou a rede local oscilar:
1. O playout no estúdio não pode parar nem perder a programação do dia.
2. O locutor precisa ter liberdade para adicionar ou reordenar músicas localmente durante seu programa ao vivo sem alterar o planejamento original no banco de dados do servidor, mas sem perder suas adições caso o servidor envie uma atualização comercial para aquele bloco.
3. Os pontos de mixagem de cada arquivo de áudio precisam funcionar tanto em modo conectado quanto em modo 100% offline/portátil, além de suportar ajustes exclusivos para uma única inserção na grade.

---

## Decisão

1. **Armazenamento Local Obrigatório (`Flow/data/`)**:
   - Toda programação e catálogo recebidos do `NaxServer` são gravados em arquivos locais dentro de `Flow/data/` (`catalog.json`, `blocks/`).
   - Se o servidor estiver offline e o bloco já existir em `data/blocks/`, o Flow carrega o arquivo local; se não existir, cria blocos vazios para operação normal.
2. **Isolamento de Edições Locais e Merge Não-Destrutivo**:
   - Modificações feitas em um bloco dentro do Flow são salvas apenas localmente naquele computador.
   - Ao sincronizar atualizações vindas do `NaxServer`, o Flow realiza um *merge* preservando os itens inseridos ou movidos localmente na mesma posição, oferecendo também um comando explícito **"Restaurar Bloco"** para descartar as edições locais quando desejado.
3. **Metadados de Mixagem em Dois Níveis (Inserção vs. Sidecar `musica.mp3.json`)**:
   - Cada arquivo de áudio possui um arquivo JSON irmão na mesma pasta (`musica.mp3` $\rightarrow$ `musica.mp3.json`) que armazena seus pontos globais de corte, mixagem e fade.
   - Inserções individuais na programação podem trazer (do servidor ou do `MixEditor` local) um override específico para aquela execução, que tem precedência sobre o arquivo irmão `musica.mp3.json`.
4. **Resolução de Mídia em Cascata**:
   - A busca do arquivo físico segue a ordem: `path` primário $\rightarrow$ caminho espelho local (*mirror path*) $\rightarrow$ stream/download via API do `NaxServer` usando o `id` da pasta.

---

## Consequências

- **Positivas**:
  - Resiliência total contra quedas de rede ou de servidor durante a transmissão ao vivo.
  - Portabilidade dos metadados globais de áudio: ao copiar uma pasta de músicas juntamente com seus arquivos `.mp3.json` para outro computador ou backup, toda a mixagem acompanha os arquivos sem depender de banco de dados externo.
  - Flexibilidade para o locutor alterar a programação ao vivo sem conflitar com atualizações feitas pelo departamento comercial no servidor.

