#!/bin/sh
# ==========================================
# TeufortBSD - Pyro Media Dispatcher Installer
# ==========================================
set -e

echo "=========================================="
echo "   Instalando o Pyro (Media Dispatcher)"
echo "=========================================="

# 1. Instalar dependências de compilação essenciais
echo "-> Verificando compilador e ferramentas básicas..."
if command -v apt >/dev/null 2>&1; then
    apt update
    apt install -y build-essential
elif command -v pkg >/dev/null 2>&1; then
    pkg install -y gcc gmake
fi

# 2. Perguntar sobre os visualizadores recomendados
printf "Deseja instalar os visualizadores de mídia recomendados (mpv, imv, zathura)? [S/n]: "
read -r user_response
user_response=$(echo "$user_response" | tr '[:upper:]' '[:lower:]')

case "$user_response" in
    ""|s|y|sim|yes)
        echo "-> Instalando visualizadores..."
        if command -v apt >/dev/null 2>&1; then
            apt install -y mpv imv zathura
        elif command -v pkg >/dev/null 2>&1; then
            pkg install -y mpv imv zathura
        fi
        ;;
    *)
        echo "-> Pulando visualizadores. (O Pyro avisará se eles faltarem ao abrir arquivos)."
        ;;
esac

# 3. Compilar e instalar
echo "-> Compilando o Pyro..."
make clean || true
make

echo "-> Instalando o binário no sistema..."
cp pyro /usr/local/bin/pyro
chmod +x /usr/local/bin/pyro

echo "=========================================="
echo "Pyro instalado com sucesso!"
echo "Teste com: pyro ~/caminho/para/arquivo"
echo "=========================================="
