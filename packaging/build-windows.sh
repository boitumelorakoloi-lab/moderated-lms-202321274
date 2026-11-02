#!/usr/bin/env bash
# build-windows.sh - builds a stand-alone Windows lms.exe on Linux (Phase 7 packaging).
# The .exe is statically linked, so the school only copies one file - no DLLs, no installer.
# Needs: mingw-w64 (sudo apt install mingw-w64), cmake, node/npm, curl, zip.
# Usage: packaging/build-windows.sh            -> dist/moderated-lms-<version>-windows.zip
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$ROOT/build-windows"
VERSION="$(grep -oP 'project\(ModeratedLMS VERSION \K[0-9.]+' "$ROOT/CMakeLists.txt")"
CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc

mkdir -p "$WORK/deps" "$ROOT/dist"
cd "$WORK/deps"

# 1. Third-party code, pinned and checked (supply-chain safety).
if [ ! -f sodium.tar.gz ]; then
    curl -sSL -o sodium.tar.gz \
        https://github.com/jedisct1/libsodium/releases/download/1.0.20-RELEASE/libsodium-1.0.20-mingw.tar.gz
fi
echo "01fb81c10ae3d249bfe5890ce59544571469e16691994ab6702a531533c6d726  sodium.tar.gz" | sha256sum -c -
# The SQLite 3.46.1 amalgamation (sqlite3.c / sqlite3.h) as shipped inside an npm package.
if [ ! -f better-sqlite3-11.3.0.tgz ]; then
    npm pack better-sqlite3@11.3.0 > /dev/null
fi
echo "084d83c773b14073d4825c9daadfc9d241eca065037a01782517e92db24da104  better-sqlite3-11.3.0.tgz" | sha256sum -c -
rm -rf sodium sqlite && mkdir sodium sqlite
tar xzf sodium.tar.gz -C sodium
tar xzf better-sqlite3-11.3.0.tgz -C sqlite
SODIUM="$WORK/deps/sodium/libsodium-win64"
SQLITE="$WORK/deps/sqlite/package/deps/sqlite3"

# 2. The schema header that CMake normally generates from db/schema.sql.
mkdir -p "$WORK/gen"
cmake -DIN="$ROOT/src/data/SchemaSql.h.in" -DSQL="$ROOT/db/schema.sql" -DOUT="$WORK/gen/SchemaSql.h" \
    -P "$ROOT/packaging/make_schema_header.cmake"

# 3. Compile.
cd "$WORK"
$CC -O2 -DSQLITE_THREADSAFE=1 -c "$SQLITE/sqlite3.c" -o sqlite3.o
$CXX -std=c++17 -O2 -Wall -Wextra -DSODIUM_STATIC -I"$ROOT/src" -I"$WORK/gen" -I"$SQLITE" \
    -I"$SODIUM/include" $(find "$ROOT/src" -name '*.cpp' | sort) sqlite3.o "$SODIUM/lib/libsodium.a" \
    -static -static-libgcc -static-libstdc++ -s -o lms.exe

# 4. Package: the program, two start-up scripts and the guides.
PKG="moderated-lms-$VERSION-windows"
rm -rf "$PKG" && mkdir "$PKG"
cp lms.exe "$PKG/"
cp "$ROOT/packaging/windows/"*.bat "$PKG/"
cp "$ROOT/packaging/windows/README-WINDOWS.txt" "$PKG/"
cp "$ROOT/docs/INSTALL.md" "$ROOT/docs/USER_MANUAL.md" "$ROOT/RELEASE_NOTES.md" "$PKG/"
rm -f "$ROOT/dist/$PKG.zip"
zip -qr "$ROOT/dist/$PKG.zip" "$PKG"
echo "Built dist/$PKG.zip"
