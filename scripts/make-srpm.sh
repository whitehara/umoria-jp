#!/usr/bin/env bash
# Build a source RPM (SRPM) for umoria-jp directly from a git ref, without
# touching the working tree. See docs/RELEASING.md for the release workflow.
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

ref="${1:-HEAD}"
name=umoria-jp
spec_file="$name.spec"

version="$(sed -n 's/^Version:[[:space:]]*//p' "$spec_file" | head -n1)"
release="$(sed -n 's/^Release:[[:space:]]*//p' "$spec_file" | head -n1 | sed 's/%{?dist}//')"

if [ -z "$version" ]; then
    echo "make-srpm: failed to extract Version: from $spec_file" >&2
    exit 1
fi
if [ -z "$release" ]; then
    echo "make-srpm: failed to extract Release: from $spec_file" >&2
    exit 1
fi

outdir="${outdir:-$repo_root/srpm-out}"
mkdir -p "$outdir/SOURCES"
rm -f "$outdir/$name"-*.src.rpm

tarball="$outdir/SOURCES/$name-$version.tar.gz"
prefix="$name-$version/"

git archive --format=tar.gz --prefix="$prefix" -o "$tarball" "$ref"

listing="$(tar tzf "$tarball")"

if echo "$listing" | grep -q '\.claude/'; then
    echo "make-srpm: SAFETY GATE TRIPPED: .claude/ found in generated tarball:" >&2
    echo "$listing" | grep '\.claude/' >&2
    rm -f "$tarball"
    exit 1
fi

if echo "$listing" | grep -qv "^${prefix}"; then
    echo "make-srpm: SAFETY GATE TRIPPED: entries outside prefix '$prefix' found in tarball:" >&2
    echo "$listing" | grep -v "^${prefix}" >&2
    rm -f "$tarball"
    exit 1
fi

for required in "${prefix}CMakeLists.txt" "${prefix}tests/test_lang.cpp"; do
    if ! echo "$listing" | grep -qF "$required"; then
        echo "make-srpm: SAFETY GATE TRIPPED: required file '$required' missing from tarball" >&2
        rm -f "$tarball"
        exit 1
    fi
done

workdir="$(mktemp -d)"
trap 'rm -rf "$workdir"' EXIT
mkdir -p "$workdir"/{BUILD,BUILDROOT,RPMS,SPECS,SRPMS}

rpmbuild -bs \
    --define "_topdir $workdir" \
    --define "_sourcedir $outdir/SOURCES" \
    --define "_srcrpmdir $outdir" \
    "$spec_file"

mapfile -t srpms < <(find "$outdir" -maxdepth 1 -name "$name-$version-*.src.rpm")

if [ "${#srpms[@]}" -ne 1 ]; then
    echo "make-srpm: expected exactly 1 generated .src.rpm, found ${#srpms[@]}" >&2
    exit 1
fi

readlink -f "${srpms[0]}"
