#!/bin/sh
# Собрать release-архив для GitHub
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/release"
PKG="$OUT/logihub-1.1"

cd "$ROOT"
meson setup build --buildtype=release -Dstrip=true 2>/dev/null || meson setup build --buildtype=release
meson compile -C build

rm -rf "$PKG"
mkdir -p "$PKG"
cp build/logihub "$PKG/"
strip "$PKG/logihub" 2>/dev/null || true

cd "$OUT"
tar czf logihub-1.1-linux-x86_64.tar.gz logihub-1.1

echo ""
echo "Готово:"
echo "  $OUT/logihub-1.1-linux-x86_64.tar.gz"
echo ""
echo "Содержимое:"
tar tzf logihub-1.1-linux-x86_64.tar.gz
