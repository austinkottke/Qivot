#!/usr/bin/env bash
# Points the vcpkg port at a released tag: sets its version and the SHA512 of
# GitHub's tarball for that tag. Run it after the tag is pushed:
#
#   tools/update-package-recipes.sh 1.0.5
#
# It also prints the tarball's SHA256, which a ConanCenter submission needs in
# its conandata.yml. The in-repo Conan recipe (conan/conanfile.py) builds from
# the checkout and reads its version from CMakeLists.txt, so it needs no update.
set -euo pipefail

VERSION="${1:?usage: $0 <version, e.g. 1.0.5>}"
VERSION="${VERSION#v}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="$ROOT/vcpkg/ports/qivot"
URL="https://github.com/austinkottke/Qivot/archive/refs/tags/v$VERSION.tar.gz"

CMAKE_VERSION=$(sed -nE 's/^[[:space:]]*VERSION[[:space:]]+([0-9.]+).*/\1/p' "$ROOT/CMakeLists.txt" | head -1)
if [ "$CMAKE_VERSION" != "$VERSION" ]; then
    echo "warning: CMakeLists.txt says $CMAKE_VERSION, not $VERSION" >&2
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
echo "Downloading $URL"
curl -fsSL "$URL" -o "$TMP/qivot.tar.gz"

SHA512=$(shasum -a 512 "$TMP/qivot.tar.gz" | cut -d' ' -f1)
SHA256=$(shasum -a 256 "$TMP/qivot.tar.gz" | cut -d' ' -f1)

# vcpkg.json: "version": "x.y.z"; portfile.cmake: SHA512 <hash>
sed -i.bak -E "s/(\"version\": *\")[^\"]*\"/\1$VERSION\"/" "$PORT/vcpkg.json"
sed -i.bak -E "s/^([[:space:]]*SHA512[[:space:]]+)[^[:space:]]+(.*)$/\1$SHA512/" "$PORT/portfile.cmake"
rm -f "$PORT/vcpkg.json.bak" "$PORT/portfile.cmake.bak"

echo "vcpkg port -> $VERSION"
echo "  SHA512 $SHA512"
echo "Tarball SHA256 (for a ConanCenter conandata.yml): $SHA256"
