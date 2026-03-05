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
- UDP control-plane auth handshake
- Encrypted frame transport (session id + counter + AEAD payload)
- Optional Linux TUN + route setup (`LINKORA_USE_TUN=1`)
- Minimalist desktop GUI (`linkora_gui`) based on Qt Widgets
- One-client UX: same app can both host or connect

## Security Notes

- Data-plane encryption uses AEAD (AES-256-GCM).
- Nonce uniqueness must be guaranteed before production use.
- Passwords must be stored as salted hashes, not plaintext.
- Current code is a bootstrap skeleton and not production-ready yet.
- Current auth handshake still sends password in plaintext over local UDP during MVP mode.
 Use trusted local environments only and switch to challenge-response/TLS before production.

## Requirements

- CMake 3.20+
- C++17 compiler
- OpenSSL development package
- Qt Widgets (optional, for `linkora_gui`)

Linux packages example (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```

Install with GUI dependencies:

```bash
LINKORA_WITH_QT=1 ./scripts/install_deps.sh
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

Build without GUI:

```bash
cmake -S . -B build/linux -DCMAKE_BUILD_TYPE=Release -DLINKORA_BUILD_GUI=OFF
cmake --build build/linux -j
```

### Windows (Visual Studio Generator)

```powershell
cmake -S . -B build/windows -G "Visual Studio 17 2022"
cmake --build build/windows --config Release
```

## GUI

- Target: `linkora_gui`
- Features: one-screen setup, `Host Network` and `Connect to Host` buttons, auto-generated runtime configs, toggle TUN mode, live logs

Quick start for end users:

1. Launch `linkora_gui`.
2. Enter host address, port, login, and password.
3. If you want to create a network, click `Host Network`.
4. If you want to join, click `Connect to Host`.

Run on Linux:

```bash
./build/linux/linkora_gui
```

Run on Windows:

```powershell
.\build\windows\Release\linkora_gui.exe
```

## Run

Linux/macOS style paths:

```bash
./build/linux/linkora_host
./build/linux/linkora_client
```

Run with explicit configs:

```bash
./build/linux/linkora_host config/host.yaml
./build/linux/linkora_client config/client.yaml
```

Enable tunnel setup (Linux, requires root or CAP_NET_ADMIN):

```bash
LINKORA_USE_TUN=1 ./build/linux/linkora_host config/host.yaml
LINKORA_USE_TUN=1 ./build/linux/linkora_client config/client.yaml
```

Generate PBKDF2 hash for `auth.password_hash`:

```bash
./build/linux/linkora_hash "my-password" 120000
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

- Replace plain-password MVP auth with challenge-response and key agreement
- Complete Windows tunnel backend (Wintun)
- Add replay protection and nonce/counter persistence
- Add integration tests for host/client handshake and encrypted exchange
