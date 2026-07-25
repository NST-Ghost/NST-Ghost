    # NST (Novelty Translation Tool)

NST is a desktop application for translating video games. It is built with C++ and the Qt framework.

## Features

*   **Game Engine Support:** Supports translation for games made with RPG Maker and Unity engines.
*   **Translation Interface:** Provides a user-friendly interface for editing and managing translations.
*   **Extensible:** Designed to be extensible with support for new game engines and translation services.

## Getting Started

To build NST from source, you will need:

*   A C++20 compatible compiler
*   CMake 3.16 or later
*   Qt 6 or later

Build instructions:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Image Translation (Optional)

The Image Translation feature uses AI-powered OCR to detect and translate text in screenshots.

### Installing AI Features

Run the included installer script:

```bash
# From extracted AppImage folder:
./usr/bin/scripts/install-ai-features.sh          # CPU version (default)
./usr/bin/scripts/install-ai-features.sh --gpu    # GPU version (requires CUDA)
```

The script uses [uv](https://astral.sh/uv) to ensure correct package versions are installed.

> **Note:** First OCR run will download language models (~100MB).

## Contributing

Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file for details on how to contribute to this project.

## SAST Tools

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](LICENSE) file for details.

