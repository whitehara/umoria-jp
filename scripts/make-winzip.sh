#!/usr/bin/env bash
# Build a portable Windows zip for umoria-jp. Run this in an MSYS2 UCRT64
# shell -- either on the Windows verification machine (pass a tarball path
# as the first argument; no git needed there) or in CI on a windows-latest
# runner via msys2/setup-msys2 (no argument: builds the tarball itself from
# the current git checkout via scripts/make-tarball.sh). Never run this on
# the Fedora host.
#
# Requires zip/unzip, which aren't installed by default: on the
# verification machine, `pacman -S zip unzip` first. In
# .github/workflows/windows.yml, include `zip unzip` in msys2/setup-msys2's
# `install:` list.
set -euo pipefail

if [ "${MSYSTEM:-}" != "UCRT64" ]; then
    echo "make-winzip: must be run from an MSYS2 UCRT64 shell (MSYSTEM=UCRT64), got MSYSTEM='${MSYSTEM:-}'" >&2
    exit 1
fi

name=umoria-jp
outdir="${outdir:-win-out}"
mkdir -p "$outdir"
outdir="$(cd "$outdir" && pwd)"

workdir="$(mktemp -d)"
trap 'rm -rf "$workdir"' EXIT

if [ $# -ge 1 ]; then
    # Argument mode: verification machine. $1 is an already-generated
    # tarball (from scripts/make-tarball.sh on the Fedora host); no git
    # command is used anywhere in this mode.
    tarball="$1"
else
    # No-argument mode: CI. A git checkout is available.
    repo_root="$(git rev-parse --show-toplevel)"
    tarball="$("$repo_root/scripts/make-tarball.sh" HEAD "$workdir")"
fi

tar xzf "$tarball" -C "$workdir"

srcdir="$(find "$workdir" -mindepth 1 -maxdepth 1 -type d -name "${name}-*" | head -n1)"
if [ -z "$srcdir" ]; then
    echo "make-winzip: could not find extracted source directory in $workdir" >&2
    exit 1
fi

version="$(sed -n 's/^Version:[[:space:]]*//p' "$srcdir/$name.spec" | head -n1)"
release="$(sed -n 's/^Release:[[:space:]]*//p' "$srcdir/$name.spec" | head -n1 | sed 's/%{?dist}//')"

if [ -z "$version" ] || [ -z "$release" ]; then
    echo "make-winzip: failed to extract Version/Release from $name.spec" >&2
    exit 1
fi

(cd "$srcdir" && bash scripts/build-windows.sh)

build_umoria_dir="$srcdir/win-out/build/umoria"
exe="$build_umoria_dir/umoria.exe"
if [ ! -f "$exe" ]; then
    echo "make-winzip: expected $exe not found (build-windows.sh should have failed already)" >&2
    exit 1
fi

pkgname="$name-$version-$release-win64"
pkgdir="$workdir/$pkgname"
mkdir -p "$pkgdir"

cp "$exe" "$pkgdir/umoria.exe"
cp -R "$build_umoria_dir/data" "$pkgdir/data"
cp "$build_umoria_dir/scores.dat" "$pkgdir/scores.dat"
cp "$build_umoria_dir/LICENSE" "$pkgdir/LICENSE"
cp "$build_umoria_dir/AUTHORS" "$pkgdir/AUTHORS"
cp "$srcdir/packaging/windows/umoria-ja.cmd" "$pkgdir/umoria-ja.cmd"
cp "$srcdir/packaging/windows/umoria-en.cmd" "$pkgdir/umoria-en.cmd"
cp "$srcdir/packaging/windows/README-windows.txt" "$pkgdir/README-windows.txt"

# --- safety gates (run before zipping) ---

if find "$pkgdir" -path '*/.claude/*' -print -quit | grep -q .; then
    echo "make-winzip: SAFETY GATE TRIPPED: .claude found in package directory:" >&2
    find "$pkgdir" -path '*/.claude/*' >&2
    exit 1
fi

for required in umoria.exe umoria-ja.cmd umoria-en.cmd README-windows.txt LICENSE AUTHORS scores.dat data/lang/ja; do
    if [ ! -e "$pkgdir/$required" ]; then
        echo "make-winzip: SAFETY GATE TRIPPED: required file/dir '$required' missing from package" >&2
        exit 1
    fi
done

dll_lines="$(objdump -p "$pkgdir/umoria.exe" | grep 'DLL Name:' || true)"
for forbidden in 'libstdc++' libgcc_s_seh libwinpthread libncursesw; do
    if echo "$dll_lines" | grep -qi "$forbidden"; then
        echo "make-winzip: SAFETY GATE TRIPPED: unexpected dynamic dependency matching '$forbidden':" >&2
        echo "$dll_lines" >&2
        exit 1
    fi
done

# --- zip ---

zipfile="$outdir/$pkgname.zip"
rm -f "$zipfile"

(cd "$workdir" && zip -rq "$zipfile" "$pkgname")

unzip -l "$zipfile"
