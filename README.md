# Pyro - Media Dispatcher para TeufortBSD

O **Pyro** é o despachante de mídia oficial do TeufortBSD. Ele detecta o tipo de arquivo (MIME type) lendo seus primeiros bytes (magic numbers) e abre no visualizador correto, com o mínimo de dependências possível.

## Funcionalidades
- **Imagens:** JPEG, PNG, GIF, WebP, BMP, TIFF, ICO (via `imv`).
- **Áudio:** MP3, FLAC, OGG, WAV (via `mpv` com interface gráfica).
- **Vídeo:** MP4, MKV, AVI (via `mpv` borderless).
- **Documentos:** PDF (via `zathura`).
- **Fallback:** Qualquer outro formato tenta abrir com `xdg-open`.

## Instalação Rápida (TeufortBSD)
```bash
git clone https://github.com/armarouge-lab/pyro-md.git
cd pyro-md
sudo ./install.sh
