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

if [ -z "$version" ]; then
    echo "make-srpm: failed to extract Version: from $spec_file" >&2
    exit 1
fi

outdir="${outdir:-$repo_root/srpm-out}"
mkdir -p "$outdir/SOURCES"
rm -f "$outdir/$name"-*.src.rpm

tarball="$("$repo_root/scripts/make-tarball.sh" "$ref" "$outdir/SOURCES")"

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
