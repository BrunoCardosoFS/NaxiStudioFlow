<div align="center">

# NaxStudio Flow

**Modern Radio Playout & Broadcast Automation Software**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](COPYING)
[![Releases](https://img.shields.io/badge/Releases-latest-blue?logo=github)](https://github.com/BrunoCardosoFS/NaxiStudioFlow/releases)

[English](README.md) • [Português](README_pt.md)

</div>

---

## 📻 About NaxStudio Flow

**NaxStudio Flow** is a free and open-source playout and broadcast automation software designed for commercial, community, or web radio stations that require stability, practicality, and high fidelity in their programming, ensuring uninterrupted broadcasting and seamless music transitions.

---

## ✨ Key Features

- **Automated Playlists and Continuous Programming**
  - Automated sequential playback, ensuring stable 24/7 broadcasts.

  - **Integrated Mix Editor** to fine-tune the exact behavior of each audio track.
  
  - Smart Media Catalog and Scanning

  - **Integrated Cartwall:** Instant triggering of sound effects, jingles, station IDs, and voiceovers.

---

### Building from Source

#### Prerequisites

- **C++ Compiler**: MSVC 2022, GCC 11+ or Clang 13+
- **CMake**: version 3.16 or higher
- **Qt 6**: version 6.5 or higher (6.8 recommended) with modules:
  - `Core`, `Widgets`, `Multimedia` (with FFmpeg backend) and `LinguistTools`

#### Step-by-Step Build Instructions (PowerShell / Terminal)

1. **Clone the repository:**
   ```bash
   git clone https://github.com/BrunoCardosoFS/NaxiStudioFlow.git
   cd NaxiStudioFlow
   ```

2. **Configure the project with CMake:**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build the executable:**
   ```bash
   cmake --build build --config Release
   ```

---

## 🔗 Useful Links

- 🌐 **Official Website**: [https://naxstudio.pages.dev](https://naxstudio.pages.dev)
- 📖 **Documentation and Guides**: [NaxStudio Flow Wiki](https://github.com/BrunoCardosoFS/NaxiStudioFlow/wiki)
- 📦 **Installation Instructions**: [Installation Guide](https://github.com/BrunoCardosoFS/NaxiStudioFlow/wiki/Install-Instructions)
- 💖 **Support the Project**: [Contribute financially to NaxStudio](https://naxstudio.pages.dev/contribua)
- 🐛 **Issue Tracker**: [GitHub Issues](https://github.com/BrunoCardosoFS/NaxiStudioFlow/issues)

---

## 🤝 How to Contribute

Contributions are very welcome! You can collaborate in several ways:
- Open an [Issue](https://github.com/BrunoCardosoFS/NaxiStudioFlow/issues) reporting bugs found or suggesting new features.
- Submit a [Pull Request](https://github.com/BrunoCardosoFS/NaxiStudioFlow/pulls) with code improvements, new implementations, or optimizations.
- Help improve translations in the resource files in `src/lang/ts/`.

---

## 📄 License

This project is distributed under the terms of the **GNU General Public License v3 (GPLv3)** (or any later version). See the [COPYING](COPYING) file for more details.