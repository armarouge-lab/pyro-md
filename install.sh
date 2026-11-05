#!/bin/sh
set -e
echo "Instalando o Pyro..."
printf "Deseja instalar os visualizadores recomendados? [S/n]: "
read -r resposta
resposta=$(echo "$resposta" | tr '[:upper:]' '[:lower:]')
case "$resposta" in
    ""|s|y|sim|yes)
        if command -v apt >/dev/null 2>&1; then apt install -y mpv imv zathura; fi ;;
    *) echo "Pulando visualizadores." ;;
esac
make
cp pyro /usr/local/bin/pyro
chmod +x /usr/local/bin/pyro
echo "Pyro instalado!"
