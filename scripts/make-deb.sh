#!/usr/bin/env bash
# Build a .deb package for umoria-jp directly from a git ref, without
# touching the working tree. See docs/RELEASING.md for the release workflow.
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

ref="${1:-HEAD}"
name=umoria-jp
spec_file="$name.spec"

version="$(sed -n 's/^Version:[[:space:]]*//p' "$spec_file" | head -n1)"

if [ -z "$version" ]; then
    echo "make-deb: failed to extract Version: from $spec_file" >&2
    exit 1
fi

outdir="${outdir:-$repo_root/deb-out}"
mkdir -p "$outdir"
rm -f "$outdir/${name}"_*.deb

workdir="$(mktemp -d)"
trap 'rm -rf "$workdir"' EXIT

tarball="$("$repo_root/scripts/make-tarball.sh" "$ref" "$workdir")"

srcdir="$workdir/$name-$version"
tar xzf "$tarball" -C "$workdir"

cp -R "$repo_root/debian" "$srcdir/debian"

(
    cd "$srcdir"
    dpkg-buildpackage -b -us -uc
)

mapfile -t debs < <(find "$workdir" -maxdepth 1 -name "${name}_*.deb")

if [ "${#debs[@]}" -ne 1 ]; then
    echo "make-deb: expected exactly 1 generated .deb, found ${#debs[@]}" >&2
    exit 1
fi

deb="${debs[0]}"

# --- post-build safety gates ---

listing="$(dpkg-deb -c "$deb")"
paths="$(echo "$listing" | awk '{print $NF}')"

if echo "$paths" | grep -q '\.claude'; then
    echo "make-deb: SAFETY GATE TRIPPED: .claude found in generated .deb:" >&2
    echo "$paths" | grep '\.claude' >&2
    exit 1
fi

if echo "$paths" | grep -qvE '^\./$|^\./usr/'; then
    echo "make-deb: SAFETY GATE TRIPPED: entries outside /usr found in .deb:" >&2
    echo "$paths" | grep -vE '^\./$|^\./usr/' >&2
    exit 1
fi

for required in ./usr/bin/umoria-jp ./usr/bin/umoria-jp.bin ./usr/share/games/umoria-jp/data/lang/ja/umoria.po; do
    if ! echo "$paths" | grep -qxF "$required"; then
        echo "make-deb: SAFETY GATE TRIPPED: required file '$required' missing from .deb" >&2
        exit 1
    fi
done

depends="$(dpkg-deb -f "$deb" Depends)"
if [ -z "$depends" ]; then
    echo "make-deb: SAFETY GATE TRIPPED: Depends field is empty (dh_shlibdeps may not have run)" >&2
    exit 1
fi

cp "$deb" "$outdir/"
readlink -f "$outdir/$(basename "$deb")"
