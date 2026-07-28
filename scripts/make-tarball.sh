#!/usr/bin/env bash
# Build a safety-gated source tarball for umoria-jp directly from a git ref,
# without touching the working tree. Shared by scripts/make-srpm.sh and
# scripts/make-deb.sh. See docs/RELEASING.md for the release workflow.
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

ref="${1:-HEAD}"
outdir="${2:?usage: make-tarball.sh <ref> <output-directory>}"
name=umoria-jp
spec_file="$name.spec"

version="$(sed -n 's/^Version:[[:space:]]*//p' "$spec_file" | head -n1)"
release="$(sed -n 's/^Release:[[:space:]]*//p' "$spec_file" | head -n1 | sed 's/%{?dist}//')"

if [ -z "$version" ]; then
    echo "make-tarball: failed to extract Version: from $spec_file" >&2
    exit 1
fi
if [ -z "$release" ]; then
    echo "make-tarball: failed to extract Release: from $spec_file" >&2
    exit 1
fi

mkdir -p "$outdir"

tarball="$outdir/$name-$version.tar.gz"
prefix="$name-$version/"

git archive --format=tar.gz --prefix="$prefix" -o "$tarball" "$ref"

listing="$(tar tzf "$tarball")"

if echo "$listing" | grep -q '\.claude/'; then
    echo "make-tarball: SAFETY GATE TRIPPED: .claude/ found in generated tarball:" >&2
    echo "$listing" | grep '\.claude/' >&2
    rm -f "$tarball"
    exit 1
fi

if echo "$listing" | grep -qv "^${prefix}"; then
    echo "make-tarball: SAFETY GATE TRIPPED: entries outside prefix '$prefix' found in tarball:" >&2
    echo "$listing" | grep -v "^${prefix}" >&2
    rm -f "$tarball"
    exit 1
fi

for required in "${prefix}CMakeLists.txt" "${prefix}tests/test_lang.cpp"; do
    if ! echo "$listing" | grep -qF "$required"; then
        echo "make-tarball: SAFETY GATE TRIPPED: required file '$required' missing from tarball" >&2
        rm -f "$tarball"
        exit 1
    fi
done

readlink -f "$tarball"
