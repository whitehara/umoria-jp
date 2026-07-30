#!/usr/bin/env bash
# Build and test umoria-jp on Windows from an already-extracted tarball
# (no .git present -- this script never invokes git). Run this from inside
# an MSYS2 UCRT64 shell, from the extracted umoria-jp-<Version>/ directory:
#
#   bash scripts/build-windows.sh
set -uo pipefail

if [ "${MSYSTEM:-}" != "UCRT64" ]; then
    echo "build-windows: must be run from an MSYS2 UCRT64 shell (MSYSTEM=UCRT64), got MSYSTEM='${MSYSTEM:-}'" >&2
    echo "RESULT: build=ng ctest=skipped"
    exit 1
fi

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

outdir="win-out"
build_tree="$outdir/build"
mkdir -p "$outdir"

echo "== cmake configure =="
if ! MINGW=ucrt64 cmake -S . -B "$build_tree" -G Ninja -DCMAKE_BUILD_TYPE=Release; then
    echo "RESULT: build=ng ctest=skipped"
    exit 1
fi

echo "== cmake build (umoria) =="
if ! cmake --build "$build_tree" --target umoria; then
    echo "RESULT: build=ng ctest=skipped"
    exit 1
fi

echo "== cmake build (test_helpers test_lang) =="
if ! cmake --build "$build_tree" --target test_helpers test_lang; then
    echo "RESULT: build=ng ctest=skipped"
    exit 1
fi

echo "== ctest =="
ctest_result=ok
if ! ctest --test-dir "$build_tree" --output-on-failure; then
    ctest_result=ng
fi

exe="$build_tree/umoria/umoria.exe"

echo "== objdump -p (DLL Name lines) =="
if [ -f "$exe" ]; then
    objdump -p "$exe" | grep "DLL Name:"
else
    echo "build-windows: expected exe not found at $exe" >&2
fi

echo "RESULT: build=ok ctest=$ctest_result"

if [ "$ctest_result" = "ng" ]; then
    exit 1
fi
