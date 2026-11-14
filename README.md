# LowLevelKeyMapper

A lightweight keyboard remapping application for Windows, written in C++.

LowLevelKeyMapper allows users to remap physical keyboard inputs to one or multiple target keys.

## Features

- Remap physical keys to other keys
- Map one physical key to multiple target keys
- Capture source and target keys directly from the keyboard
- Add and remove key mappings
- Clear all mappings
- Save and load mappings through a JSON configuration file
- System tray integration

## Requirements

- Windows
- C++ compiler (GCC / MinGW-w64)
- CMake
- MSYS2 UCRT64 environment (recommended)

## Build

Clone the repository:

```bash
git clone https://github.com/dxtu1/LowLevelKeyMapper.git
cd LowLevelKeyMapper
```

Configure the project:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build --parallel
```

Run:

```powershell
.\build\LowLevelKeyMapper.exe
```

## Configuration

The application uses `config.json` to store keyboard mappings.

If the configuration file does not exist, the application creates one with an empty mapping configuration.

Example:

```json
{
    "mappings": {}
}
```

Configuration files are excluded from version control.

## Project Structure

```text
LowLevelKeyMapper/
├── include/
├── src/
│   ├── input/
│   ├── mapping/
│   └── ui/
├── .clang-format
├── .gitignore
├── CMakeLists.txt
└── README.md
```

## License

This project is licensed under the MIT License.
See the [LICENSE](LICENSE) file for details.