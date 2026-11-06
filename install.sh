#!/bin/sh
set -e

echo "Installing Pyro..."

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

printf "Install recommended viewers (mpv, imv, zathura) and neovim? [S/n]: "
read -r response
response=$(echo "$response" | tr '[:upper:]' '[:lower:]')

case "$response" in
    ""|s|y|sim|yes)
        $PKG_MGR mpv imv zathura neovim
        ;;
    *)
        echo "Skipping extra dependencies."
        ;;
esac

make clean || true
make

echo "Installing binary and resources..."
cp pyro /usr/local/bin/pyro
chmod +x /usr/local/bin/pyro

mkdir -p /usr/local/share/pyro
if [ -f "res/cd-icon.png" ]; then
    cp res/cd-icon.png /usr/local/share/pyro/
else
    echo "Warning: res/cd-icon.png not found. Audio window will be blank."
fi

echo "Pyro installed successfully."