#!/bin/sh
# ==========================================
# TeufortBSD - Pyro Media Dispatcher Installer
# ==========================================
set -e

echo "=========================================="
echo "   Installing Pyro (Media Dispatcher)"
echo "=========================================="

# 1. Detect package manager
if command -v apt >/dev/null 2>&1; then
    PKG_MGR="apt install -y"
    apt update
    apt install -y build-essential git
elif command -v pkg >/dev/null 2>&1; then
    PKG_MGR="pkg install -y"
    pkg update
    pkg install -y gcc gmake git
else
    echo "Error: Unsupported package manager."
    exit 1
fi

# 2. Ask for media viewers
printf "Install recommended media viewers (mpv, imv, zathura)? [S/n]: "
read -r response_media
response_media=$(echo "$response_media" | tr '[:upper:]' '[:lower:]')

case "$response_media" in
    ""|s|y|sim|yes)
        echo "-> Installing media viewers..."
        $PKG_MGR mpv imv zathura
        ;;
    *)
        echo "-> Skipping media viewers."
        ;;
esac

# 3. Ask for text editor (Neovim)
printf "Install Neovim for text/code files? [S/n]: "
read -r response_editor
response_editor=$(echo "$response_editor" | tr '[:upper:]' '[:lower:]')

case "$response_editor" in
    ""|s|y|sim|yes)
        echo "-> Installing Neovim..."
        $PKG_MGR neovim
        ;;
    *)
        echo "-> Skipping Neovim. Pyro will fallback to vim or custom editor."
        ;;
esac

# 4. Compile and install
echo "-> Compiling Pyro..."
make clean || true
make

echo "-> Installing binary to /usr/local/bin..."
cp pyro /usr/local/bin/pyro
chmod +x /usr/local/bin/pyro

echo "=========================================="
echo "Pyro installed successfully!"
echo "Test with: pyro ~/path/to/file"
echo "=========================================="
