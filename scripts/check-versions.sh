#!/usr/bin/env bash
# Verify that umoria-jp.spec and debian/changelog agree on Version/Release,
# and (if a tag name is given) that the tag matches jp-v<Version>-<Release>.
# Used by .github/workflows/deb.yml before building packages. copr.yml keeps
# its own inline spec-only version check (no debian/changelog to compare
# there) rather than calling this script, per the D2 constraint that
# copr.yml's diff stay limited to the shared concurrency block.
# Deliberately avoids dpkg-parsechangelog so this also runs on Fedora hosts.
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

spec_file=umoria-jp.spec
changelog_file=debian/changelog

spec_version="$(sed -n 's/^Version:[[:space:]]*//p' "$spec_file" | head -n1)"
spec_release="$(sed -n 's/^Release:[[:space:]]*//p' "$spec_file" | head -n1 | sed 's/%{?dist}//')"

if [ -z "$spec_version" ] || [ -z "$spec_release" ]; then
    echo "check-versions: failed to extract Version/Release from $spec_file" >&2
    exit 1
fi

changelog_first_line="$(head -n1 "$changelog_file")"
changelog_version_release="$(echo "$changelog_first_line" | sed -n 's/^umoria-jp (\([^)]*\)).*/\1/p')"
changelog_version="${changelog_version_release%-*}"
changelog_release="${changelog_version_release##*-}"

if [ -z "$changelog_version" ] || [ -z "$changelog_release" ]; then
    echo "check-versions: failed to extract Version/Release from $changelog_file" >&2
    exit 1
fi

if [ "$spec_version" != "$changelog_version" ] || [ "$spec_release" != "$changelog_release" ]; then
    echo "check-versions: MISMATCH - $spec_file has $spec_version-$spec_release, $changelog_file has $changelog_version-$changelog_release" >&2
    exit 1
fi

echo "check-versions: $spec_file and $changelog_file agree on $spec_version-$spec_release"

if [ "$#" -ge 1 ]; then
    tag="$1"
    expected="jp-v${spec_version}-${spec_release}"
    if [ "$tag" != "$expected" ]; then
        echo "check-versions: tag '$tag' does not match spec-derived tag '$expected'" >&2
        exit 1
    fi
    echo "check-versions: tag '$tag' matches"
fi
