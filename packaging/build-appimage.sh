#!/usr/bin/env bash
# Builds dist/Snout_and_Siege-<version>-x86_64.AppImage.
# The binary is compiled inside an Ubuntu 22.04 container (glibc 2.35) so the
# AppImage runs on older distros too. Requires podman (or docker), ImageMagick,
# and appimagetool (https://github.com/AppImage/appimagetool) on PATH or in
# $APPIMAGETOOL.
# Usage: packaging/build-appimage.sh [version]
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$PWD"
VERSION="${1:-$(git describe --tags --always 2>/dev/null || echo dev)}"
WORK="$ROOT/build/appimage"
APPDIR="$WORK/AppDir"
OUT="$ROOT/dist/Snout_and_Siege-${VERSION}-x86_64.AppImage"
ENGINE="$(command -v podman || command -v docker)"
TOOL="${APPIMAGETOOL:-appimagetool}"

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/snout-and-siege" "$ROOT/dist"

echo ">> Compiling in ubuntu:22.04 container"
"$ENGINE" run --rm -v "$ROOT":/src:ro,Z -v "$WORK":/out:Z docker.io/library/ubuntu:22.04 bash -euc '
    apt-get update -qq >/dev/null
    DEBIAN_FRONTEND=noninteractive apt-get install -y -qq g++ >/dev/null
    g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter /src/src/*.cpp \
        -o /out/furious_swine -static-libstdc++ -static-libgcc -ldl -lm -lpthread'
install -m755 "$WORK/furious_swine" "$APPDIR/usr/bin/furious_swine"

echo ">> Staging assets"
git ls-files -z assets | xargs -0 -I{} cp --parents {} "$APPDIR/usr/share/snout-and-siege/"
echo "$VERSION" > "$APPDIR/usr/share/snout-and-siege/release.txt"

install -m755 packaging/AppRun "$APPDIR/AppRun"
cp packaging/snout-and-siege.desktop "$APPDIR/"
magick assets/sprites/pig_trotter.bmp -resize 256x256 "$APPDIR/snout-and-siege.png"

echo ">> Packing $OUT"
ARCH=x86_64 "$TOOL" --appimage-extract-and-run "$APPDIR" "$OUT"
echo ">> Done: $OUT"
