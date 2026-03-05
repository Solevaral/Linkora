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
- UDP auth handshake в control-plane
- Зашифрованный frame transport (session id + counter + AEAD payload)
- Опциональный Linux TUN + route setup (`LINKORA_USE_TUN=1`)
- Минималистичный desktop GUI (`linkora_gui`) на Qt Widgets
- Единый клиентский UX: одно приложение умеет и хостить, и подключаться

## Примечания по безопасности

- Для data-plane используется AEAD (AES-256-GCM).
- Перед production необходимо гарантировать уникальность nonce.
- Пароли должны храниться как salted hash, а не в открытом виде.
- Текущий код - это стартовый скелет, пока не production-ready.
- В MVP-режиме пароль пока передается в открытом виде внутри локального UDP handshake.
 Используйте только доверенную среду и перед production переходите на challenge-response/TLS.

## Требования

- CMake 3.20+
- Компилятор с поддержкой C++17
- OpenSSL development package
- Qt Widgets (опционально, для `linkora_gui`)

Пример пакетов для Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```

Установка с зависимостями для GUI:

```bash
LINKORA_WITH_QT=1 ./scripts/install_deps.sh
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

Сборка без GUI:

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

- Таргет: `linkora_gui`
- Возможности: единый экран, кнопки `Host Network` и `Connect to Host`, авто-генерация runtime-конфигов, переключатель TUN-режима, live-логи

Быстрый старт для пользователей:

1. Запустите `linkora_gui`.
2. Заполните host address, port, login и password.
3. Если нужно создать сеть, нажмите `Host Network`.
4. Если нужно подключиться, нажмите `Connect to Host`.

Запуск на Linux:

```bash
./build/linux/linkora_gui
```

Запуск на Windows:

```powershell
.\build\windows\Release\linkora_gui.exe
```

## Запуск

Пути для Linux/macOS:

```bash
./build/linux/linkora_host
./build/linux/linkora_client
```

Запуск с явными конфигами:

```bash
./build/linux/linkora_host config/host.yaml
./build/linux/linkora_client config/client.yaml
```

Запуск с поднятием туннеля (Linux, нужны root или CAP_NET_ADMIN):

```bash
LINKORA_USE_TUN=1 ./build/linux/linkora_host config/host.yaml
LINKORA_USE_TUN=1 ./build/linux/linkora_client config/client.yaml
```

Генерация PBKDF2-хеша для `auth.password_hash`:

```bash
./build/linux/linkora_hash "my-password" 120000
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

- Заменить MVP-auth (plain password) на challenge-response и согласование ключей
- Доделать Windows tunnel backend (Wintun)
- Добавить защиту от replay и хранение nonce/counter
- Добавить integration-тесты host/client handshake и зашифрованного обмена
