<div align="center">

# NaxStudio Flow

**Software Moderno de Automação e Playout para Emissoras de Rádio**

[![Licença: GPL v3](https://img.shields.io/badge/Licen%C3%A7a-GPLv3-blue.svg)](COPYING)
[![Releases](https://img.shields.io/badge/Releases-latest-blue?logo=github)](https://github.com/BrunoCardosoFS/NaxiStudioFlow/releases)

[English](README.md) • [Português](README_pt.md)

</div>

---

## 📻 Sobre o NaxStudio Flow

O **NaxStudio Flow** é um software livre e de código aberto para playout e automação de transmissão, projetado para emissoras de rádio comerciais, comunitárias ou web que necessitam de estabilidade, praticidade e alta fidelidade em sua programação, garantindo transmissão ininterrupta e transições musicais perfeitas.

---

## ✨ Principais Recursos

- **Playlists Automatizadas e Programação Contínua**
  - Reprodução sequencial automatizada, garantindo transmissões estáveis 24 horas por dia, 7 dias por semana.

  - **Editor de Mixagem integrado** para ajustar o comportamento exato de cada faixa de áudio.
  
  - Catálogo Inteligente e Escaneamento de Mídia

  - **Cartucheira Integrada:** Disparo rápido e instantâneo de efeitos sonoros, vinhetas, chamadas e carimbos.

---

### Compilação a partir do Código-Fonte

#### Pré-requisitos

- **Compilador C++**: MSVC 2022, GCC 11+ ou Clang 13+
- **CMake**: versão 3.16 ou superior
- **Qt 6**: versão 6.5 ou superior (6.8 recomendada) com os módulos:
  - `Core`, `Widgets`, `Multimedia` (com backend FFmpeg) e `LinguistTools`

#### Passo a Passo de Compilação (PowerShell / Terminal)

1. **Clone o repositório:**
   ```bash
   git clone https://github.com/BrunoCardosoFS/NaxiStudioFlow.git
   cd NaxiStudioFlow
   ```

2. **Configure o projeto com o CMake:**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   ```

3. **Compile o executável:**
   ```bash
   cmake --build build --config Release
   ```

---

## 🔗 Links Úteis

- 🌐 **Site Oficial**: [https://naxstudio.pages.dev](https://naxstudio.pages.dev)
- 📖 **Documentação e Guias**: [Wiki do NaxStudio Flow](https://github.com/BrunoCardosoFS/NaxiStudioFlow/wiki)
- 📦 **Instruções de Instalação**: [Guia de Instalação](https://github.com/BrunoCardosoFS/NaxiStudioFlow/wiki/Install-Instructions)
- 💖 **Apoie o Projeto**: [Contribuir financeiramente com o NaxStudio](https://naxstudio.pages.dev/contribua)
- 🐛 **Rastreador de Problemas**: [GitHub Issues](https://github.com/BrunoCardosoFS/NaxiStudioFlow/issues)

---

## 🤝 Como Contribuir

Contribuições são muito bem-vindas! Você pode colaborar de diversas maneiras:
- Abra uma [Issue](https://github.com/BrunoCardosoFS/NaxiStudioFlow/issues) relatando problemas encontrados ou sugerindo novas funcionalidades.
- Envie um [Pull Request](https://github.com/BrunoCardosoFS/NaxiStudioFlow/pulls) com melhorias de código, novas implementações ou otimizações.
- Ajude a aprimorar as traduções nos arquivos de recursos em `src/lang/ts/`.

---

## 📄 Licença

Este projeto é distribuído sob os termos da licença **GNU General Public License v3 (GPLv3)** (ou qualquer versão posterior). Consulte o arquivo [COPYING](COPYING) para mais detalhes.
