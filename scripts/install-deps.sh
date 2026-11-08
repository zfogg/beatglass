#!/usr/bin/env bash
set -euo pipefail

# Install native prerequisites for the CLI, desktop shell, SQLite prototype and
# PipeWire checks.  The script does not configure or build Beatglass.
# Packages are selected by the host package manager, so the same script works
# on x86_64, arm64 and aarch64 hosts when the distribution provides them.

if [[ "$(uname -s)" == "Darwin" ]]; then
  command -v brew >/dev/null 2>&1 || { echo "Homebrew is required on macOS: https://brew.sh" >&2; exit 1; }
  arch="$(uname -m)"
  echo "Installing Beatglass prerequisites for macOS (${arch})"
  brew update
  brew install cmake ninja git pkg-config python aubio libsndfile sqlite openssl qt pipewire
  echo "Beatglass prerequisites installed for ${arch}. Set CMAKE_PREFIX_PATH=\"$(brew --prefix qt)\" when configuring the Qt desktop."
  exit 0
fi

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "This script is for Linux and macOS. Use scripts/install-deps.ps1 on Windows." >&2
  exit 2
fi

if [[ "$(id -u)" -eq 0 ]]; then
  SUDO=()
elif command -v sudo >/dev/null 2>&1; then
  SUDO=(sudo)
else
  echo "sudo is required when not running as root" >&2
  exit 1
fi

source /etc/os-release
ID_LIKE="${ID_LIKE:-}"
arch="$(uname -m)"
echo "Installing Beatglass prerequisites for ${ID:-unknown} (${arch})"

case "${ID}" in
  ubuntu|debian|linuxmint|pop)
    "${SUDO[@]}" apt-get update
    "${SUDO[@]}" apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build git pkg-config python3 \
      libaubio-dev libsndfile1-dev libsqlite3-dev libssl-dev \
      qt6-base-dev qt6-declarative-dev qt6-shadertools-dev libgl1-mesa-dev \
      qml6-module-qt-labs-folderlistmodel qml6-module-qttest \
      qml6-module-qtquick qml6-module-qtquick-controls \
      qml6-module-qtquick-dialogs qml6-module-qtquick-layouts \
      qml6-module-qtquick-window qml6-module-qtquick-templates \
      qml6-module-qtqml qml6-module-qtqml-models \
      qml6-module-qtqml-workerscript libpipewire-0.3-dev
    ;;
  fedora|rhel|centos|rocky|almalinux)
    "${SUDO[@]}" dnf install -y \
      gcc-c++ cmake ninja-build git pkgconf-pkg-config python3 \
      aubio-devel libsndfile-devel sqlite-devel openssl-devel \
      qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtshadertools-devel \
      pipewire-devel mesa-libGL-devel
    ;;
  arch|manjaro|endeavouros)
    "${SUDO[@]}" pacman -Sy --needed --noconfirm \
      base-devel cmake ninja git pkgconf python \
      aubio libsndfile sqlite openssl qt6-base qt6-declarative \
      qt6-shadertools pipewire mesa
    ;;
  opensuse*|sles)
    "${SUDO[@]}" zypper --non-interactive refresh
    "${SUDO[@]}" zypper --non-interactive install \
      gcc-c++ cmake ninja git pkg-config python3 \
      aubio-devel libsndfile-devel sqlite3-devel libopenssl-devel \
      libqt6-qtbase-devel libqt6-qtdeclarative-devel \
      libqt6-qtshadertools-devel pipewire-devel Mesa-libGL-devel
    ;;
  alpine)
    "${SUDO[@]}" apk add --no-cache \
      build-base cmake ninja git pkgconf python3 \
      aubio-dev libsndfile-dev sqlite-dev openssl-dev \
      qt6-qtbase-dev qt6-qtdeclarative-dev qt6-qtshadertools-dev \
      pipewire-dev mesa-gl
    ;;
  *)
    if [[ " ${ID_LIKE} " == *" debian "* ]]; then
      echo "Unsupported Debian-family ID '${ID}'; run with a compatible apt package set or add it to this script." >&2
    else
      echo "Unsupported Linux distribution '${ID}'. Supported families: Debian/Ubuntu, Fedora/RHEL, Arch, openSUSE, Alpine." >&2
    fi
    exit 3
    ;;
esac

echo "Beatglass prerequisites installed for ${arch}. Run cmake configure from the repository root."
