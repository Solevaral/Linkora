# Linkora

Language: English | [Russian](README.ru.md)

Linkora is a cross-platform C++ VPN-like project (Linux + Windows) inspired by LAN overlay tools.
This repository currently contains an MVP-oriented project skeleton with AES-256-GCM primitives,
platform tunnel abstractions, and host/client entry points.

## MVP Scope

- P2P first architecture
- AES-256-GCM data protection using OpenSSL
- YAML configuration files
- Host-local login/password validation
- CLI first (no GUI in MVP)

## Security Notes

- Data-plane encryption uses AEAD (AES-256-GCM).
- Nonce uniqueness must be guaranteed before production use.
- Passwords must be stored as salted hashes, not plaintext.
- Current code is a bootstrap skeleton and not production-ready yet.

## Requirements

- CMake 3.20+
- C++17 compiler
- OpenSSL development package

Linux packages example (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```

Windows options:

- Visual Studio 2022 (MSVC) + CMake
- OpenSSL installed and visible to CMake

## Build

### Linux

```bash
cmake -S . -B build/linux -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux -j
```

### Windows (Visual Studio Generator)

```powershell
cmake -S . -B build/windows -G "Visual Studio 17 2022"
cmake --build build/windows --config Release
```

## Run

Linux/macOS style paths:

```bash
./build/linux/linkora_host
./build/linux/linkora_client
```

Windows style paths:

```powershell
.\build\windows\Release\linkora_host.exe
.\build\windows\Release\linkora_client.exe
```

## Tests

```bash
ctest --test-dir build/linux --output-on-failure
```

## Planned Next Steps

- Implement Linux TUN and Windows tunnel backend
- Implement authenticated control-plane handshake
- Implement encrypted UDP frame transport
- Add routing setup/cleanup per platform
