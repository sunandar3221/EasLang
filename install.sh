#!/usr/bin/env bash
set -e

REPO="sunandar3221/EasLang"
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"

case "$ARCH" in
  x86_64|amd64)
    ARCH_TAG="x64"
    ;;
  aarch64|arm64)
    ARCH_TAG="arm64"
    ;;
  *)
    echo "Unsupported architecture: $ARCH"
    exit 1
    ;;
esac

IS_TERMUX=0
if [ -n "$PREFIX" ] && [ -d "$PREFIX/bin" ]; then
  IS_TERMUX=1
fi

if [ "$IS_TERMUX" -eq 1 ]; then
  BIN_DIR="$PREFIX/bin"
  if [ "$ARCH_TAG" = "arm64" ]; then
    ASSET_NAME="eas-android-arm64"
  else
    ASSET_NAME="eas-linux-x64-static"
  fi
else
  BIN_DIR="/usr/local/bin"
  if [ "$OS" = "linux" ]; then
    if [ "$ARCH_TAG" = "x64" ]; then
      ASSET_NAME="eas-linux-x64"
    else
      ASSET_NAME="eas-linux-arm64-static"
    fi
  else
    echo "Unsupported operating system: $OS"
    exit 1
  fi
fi

DOWNLOAD_URL="https://github.com/$REPO/releases/latest/download/$ASSET_NAME"
FALLBACK_URL="https://github.com/$REPO/releases/download/v1.0.0/$ASSET_NAME"

echo "=========================================="
echo "          EasLang Installer               "
echo "=========================================="
echo "Detected OS: $OS"
echo "Detected Arch: $ARCH"
echo "Install Target: $BIN_DIR/eas"
echo "Downloading $ASSET_NAME..."

TEMP_BIN="$(mktemp)"
HTTP_OK=0

if command -v curl >/dev/null 2>&1; then
  if curl -sSL --fail "$DOWNLOAD_URL" -o "$TEMP_BIN"; then
    HTTP_OK=1
  elif curl -sSL --fail "$FALLBACK_URL" -o "$TEMP_BIN"; then
    HTTP_OK=1
  fi
elif command -v wget >/dev/null 2>&1; then
  if wget -q "$DOWNLOAD_URL" -O "$TEMP_BIN"; then
    HTTP_OK=1
  elif wget -q "$FALLBACK_URL" -O "$TEMP_BIN"; then
    HTTP_OK=1
  fi
else
  echo "Error: curl or wget is required to download EasLang."
  exit 1
fi

if [ "$HTTP_OK" -ne 1 ]; then
  echo "Error: Failed to download $ASSET_NAME from GitHub Releases."
  rm -f "$TEMP_BIN"
  exit 1
fi

chmod +x "$TEMP_BIN"

if ! "$TEMP_BIN" -v >/dev/null 2>&1; then
  if [ "$IS_TERMUX" -eq 1 ] && [ "$ARCH_TAG" = "arm64" ]; then
    echo "Notice: Testing static ARM64 binary fallback..."
    STATIC_URL="https://github.com/$REPO/releases/download/v1.0.0/eas-linux-arm64-static"
    if curl -sSL --fail "$STATIC_URL" -o "$TEMP_BIN" 2>/dev/null || wget -q "$STATIC_URL" -O "$TEMP_BIN" 2>/dev/null; then
      chmod +x "$TEMP_BIN"
    fi
  fi
fi

if [ -w "$BIN_DIR" ]; then
  mv "$TEMP_BIN" "$BIN_DIR/eas"
else
  if command -v sudo >/dev/null 2>&1; then
    sudo mv "$TEMP_BIN" "$BIN_DIR/eas"
  else
    mkdir -p "$HOME/.local/bin"
    mv "$TEMP_BIN" "$HOME/.local/bin/eas"
    BIN_DIR="$HOME/.local/bin"
    echo "Installed to $BIN_DIR/eas. Make sure $BIN_DIR is in your PATH."
  fi
fi

echo "=========================================="
echo " EasLang successfully installed to $BIN_DIR/eas"
echo " Run 'eas' to enter REPL or 'eas <file.eas>'"
echo "=========================================="
