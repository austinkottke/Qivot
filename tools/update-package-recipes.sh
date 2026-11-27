#!/usr/bin/env bash
# Points the package recipes at a released tag. Run it after the tag is pushed:
#
#   tools/update-package-recipes.sh 1.0.6
#
# It downloads GitHub's tarball for the tag and writes
#   vcpkg/ports/qivot/vcpkg.json            "version"
#   vcpkg/ports/qivot/portfile.cmake        SHA512 of the tarball
#   conan/center/qivot/config.yml           the version -> folder "all"
#   conan/center/qivot/all/conandata.yml    the tarball URL and SHA256
#
# The in-repo Conan recipe (conan/conanfile.py) builds from the checkout and
# reads its version from CMakeLists.txt, so it needs no update.
#
# To copy the recipes into local clones of the two registries, ready to commit
# and open a pull request from (see docs/registry-submission.md):
#
#   tools/update-package-recipes.sh 1.0.6 --submit \
#       --vcpkg ~/src/vcpkg --conan-center ~/src/conan-center-index
#
# --submit runs `vcpkg format-manifest` in the vcpkg clone, so that clone must
# have a bootstrapped vcpkg executable, and prints the commit steps. It never
# commits, pushes or opens anything.
set -euo pipefail

usage() {
    echo "usage: $0 <version> [--submit [--vcpkg <vcpkg clone>] [--conan-center <conan-center-index clone>]]" >&2
    exit 2
}

[ $# -ge 1 ] || usage
VERSION="${1#v}"
shift
SUBMIT=0
VCPKG_REPO=""
CCI_REPO=""
while [ $# -gt 0 ]; do
    case "$1" in
        --submit) SUBMIT=1 ;;
        --vcpkg) VCPKG_REPO="${2:?}"; shift ;;
        --conan-center) CCI_REPO="${2:?}"; shift ;;
        *) usage ;;
    esac
    shift
done
if [ $SUBMIT = 1 ] && [ -z "$VCPKG_REPO$CCI_REPO" ]; then
    echo "--submit needs --vcpkg and/or --conan-center" >&2
    exit 2
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="$ROOT/vcpkg/ports/qivot"
CCI="$ROOT/conan/center/qivot"
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

# ConanCenter lists one version per recipe folder; this carries only the newest.
cat > "$CCI/config.yml" <<EOF
versions:
  "$VERSION":
    folder: all
EOF
cat > "$CCI/all/conandata.yml" <<EOF
sources:
  "$VERSION":
    url: "$URL"
    sha256: "$SHA256"
EOF

echo "vcpkg port      -> $VERSION  (SHA512 $SHA512)"
echo "ConanCenter     -> $VERSION  (SHA256 $SHA256)"

[ $SUBMIT = 1 ] || exit 0

if [ -n "$VCPKG_REPO" ]; then
    VCPKG_REPO="$(cd "$VCPKG_REPO" && pwd)"
    [ -x "$VCPKG_REPO/vcpkg" ] || { echo "$VCPKG_REPO/vcpkg not found: run bootstrap-vcpkg.sh there first" >&2; exit 1; }
    rm -rf "$VCPKG_REPO/ports/qivot"
    cp -R "$PORT" "$VCPKG_REPO/ports/qivot"
    (
        cd "$VCPKG_REPO"
        ./vcpkg format-manifest ports/qivot/vcpkg.json
    )
    # x-add-version records the port's committed git tree, so it runs after the
    # port is committed. That and the commits are left to you.
    echo "vcpkg: ports/qivot copied into $VCPKG_REPO. There:"
    echo "  git add ports/qivot && git commit -m \"[qivot] new port\""
    echo "  ./vcpkg x-add-version qivot && git add versions && git commit --amend --no-edit"
fi

if [ -n "$CCI_REPO" ]; then
    CCI_REPO="$(cd "$CCI_REPO" && pwd)"
    DEST="$CCI_REPO/recipes/qivot"
    rm -rf "$DEST"
    mkdir -p "$DEST/all"
    cp "$CCI/config.yml" "$DEST/"
    cp "$CCI/all/conanfile.py" "$CCI/all/conandata.yml" "$DEST/all/"
    cp -R "$ROOT/conan/test_package" "$DEST/all/test_package"
    rm -rf "$DEST/all/test_package/build" "$DEST/all/test_package/CMakeUserPresets.json"
    echo "ConanCenter: recipes/qivot written in $CCI_REPO"
fi
