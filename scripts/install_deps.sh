#!/usr/bin/env bash
set -euo pipefail

if command -v apt >/dev/null 2>&1; then
  sudo apt update
  sudo apt install -y build-essential cmake libssl-dev
  if [[ "${LINKORA_WITH_QT:-0}" == "1" ]]; then
    sudo apt install -y qt6-base-dev
  fi
  exit 0
fi

if command -v dnf >/dev/null 2>&1; then
  sudo dnf install -y gcc-c++ cmake openssl-devel
  if [[ "${LINKORA_WITH_QT:-0}" == "1" ]]; then
    sudo dnf install -y qt6-qtbase-devel
  fi
  exit 0
fi

if command -v pacman >/dev/null 2>&1; then
  sudo pacman -Sy --needed base-devel cmake openssl
  if [[ "${LINKORA_WITH_QT:-0}" == "1" ]]; then
    sudo pacman -Sy --needed qt6-base
  fi
  exit 0
fi

echo "Unsupported package manager. Install CMake and OpenSSL dev package manually."
exit 1
