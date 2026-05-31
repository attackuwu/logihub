#!/bin/sh
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/release"
PKG="$OUT/logihub-1.0"

cd "$ROOT"
meson setup build --buildtype=release -Dstrip=true 2>/dev/null || meson setup build --buildtype=release
meson compile -C build

rm -rf "$PKG"
mkdir -p "$PKG/udev"
cp build/logihub "$PKG/"
cp udev/99-logitech-hub.rules "$PKG/udev/"
strip "$PKG/logihub" 2>/dev/null || true

cd "$OUT"
tar czf logihub-1.0-linux-x86_64.tar.gz logihub-1.0

echo ""
echo "Готово:"
echo "  $OUT/logihub-1.0-linux-x86_64.tar.gz"
echo ""
echo "Содержимое:"
tar tzf logihub-1.0-linux-x86_64.tar.gz
