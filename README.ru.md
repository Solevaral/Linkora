# Linkora

Язык: Русский | [English](README.md)

Linkora - кроссплатформенный VPN-подобный проект на C++ (Linux + Windows), вдохновленный инструментами оверлейных LAN-сетей.
Сейчас репозиторий содержит MVP-скелет проекта: примитивы AES-256-GCM,
абстракции туннеля для платформ и точки входа host/client.

## Scope MVP

- Архитектура P2P в первой версии
- Защита данных AES-256-GCM через OpenSSL
- Конфигурация через YAML
- Локальная проверка логина/пароля на стороне host
- CLI first (без GUI в MVP)

## Примечания по безопасности

- Для data-plane используется AEAD (AES-256-GCM).
- Перед production необходимо гарантировать уникальность nonce.
- Пароли должны храниться как salted hash, а не в открытом виде.
- Текущий код - это стартовый скелет, пока не production-ready.

## Требования

- CMake 3.20+
- Компилятор с поддержкой C++17
- OpenSSL development package

Пример пакетов для Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```

Варианты для Windows:

- Visual Studio 2022 (MSVC) + CMake
- Установленный OpenSSL, доступный для CMake

## Сборка

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

## Запуск

Пути для Linux/macOS:

```bash
./build/linux/linkora_host
./build/linux/linkora_client
```

Пути для Windows:

```powershell
.\build\windows\Release\linkora_host.exe
.\build\windows\Release\linkora_client.exe
```

## Тесты

```bash
ctest --test-dir build/linux --output-on-failure
```

## Следующие шаги

- Реализовать Linux TUN и Windows tunnel backend
- Реализовать аутентифицированный control-plane handshake
- Реализовать зашифрованный UDP frame transport
- Добавить настройку и cleanup маршрутов для каждой платформы
