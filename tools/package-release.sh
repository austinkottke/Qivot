#!/usr/bin/env bash
# Builds the files a Qivot release ships, into OUTDIR (default: dist/release):
#
#   qivot.hpp                    the single header, stamped with the version
#   qivot-<version>.zip          the single header plus the full sources, to build
#   qivot-<version>.tar.gz       Qivot with qmake or CMake (list models, QML forms…)
#   SHA256SUMS                   checksums of the above
#
# The single header is regenerated from src/ and, unless --no-smoke is given,
# compiled and run against Qt (Core + Sql) before anything is packaged.
#
#   tools/package-release.sh 1.1.0
#   tools/package-release.sh 1.1.0 /tmp/out --no-smoke
#
# Set QT_PREFIX (e.g. ~/Qt/6.8.3/macos) if CMake can't find Qt by itself.
set -euo pipefail

VERSION="${1:-}"
if [[ -z "$VERSION" ]]; then
    echo "usage: $0 <version> [outdir] [--no-smoke]" >&2
    exit 2
fi
VERSION="${VERSION#v}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${2:-$ROOT/dist/release}"
[[ "$OUT" == --* ]] && OUT="$ROOT/dist/release"
SMOKE=1
for arg in "$@"; do [[ "$arg" == "--no-smoke" ]] && SMOKE=0; done

mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "==> Qivot $VERSION"

# 1. The single header, from the sources as they are now
python3 "$ROOT/tools/amalgamate.py" >/dev/null
{
    echo "// Qivot $VERSION - https://github.com/austinkottke/Qivot"
    cat "$ROOT/dist/qivot.hpp"
} > "$WORK/qivot.hpp"
echo "    qivot.hpp: $(wc -l < "$WORK/qivot.hpp" | tr -d ' ') lines"

# 2. It compiles, links and works
if [[ "$SMOKE" == 1 ]]; then
    echo "==> smoke test"
    CMAKE_ARGS=(-S "$ROOT/tools/release" -B "$WORK/smoke" -DQIVOT_HPP_DIR="$WORK" -DCMAKE_BUILD_TYPE=Release)
    [[ -n "${QT_PREFIX:-}" ]] && CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="$QT_PREFIX")
    if ! { cmake "${CMAKE_ARGS[@]}" && cmake --build "$WORK/smoke" -j 4; } > "$WORK/smoke.log" 2>&1; then
        cat "$WORK/smoke.log"
        echo "smoke test: the single header did not build" >&2
        exit 1
    fi
    QT_QPA_PLATFORM=offscreen "$WORK/smoke/smoke"
fi

# 3. The source package: everything needed to build Qivot, nothing else
PKG="qivot-$VERSION"
STAGE="$WORK/$PKG"
mkdir -p "$STAGE"
cp "$WORK/qivot.hpp" "$STAGE/"
cp -R "$ROOT/src" "$STAGE/src"
cp -R "$ROOT/cmake" "$STAGE/cmake"
cp "$ROOT/CMakeLists.txt" "$ROOT/qivot.pro" "$ROOT/README.md" \
   "$ROOT/LICENSE.txt" "$ROOT/NOTICE.txt" "$STAGE/"
[[ -d "$ROOT/drivers" ]] && cp -R "$ROOT/drivers" "$STAGE/drivers"
find "$STAGE" \( -name '*.o' -o -name 'moc_*' -o -name '.DS_Store' -o -name '*.pro.user*' \) -delete
cat > "$STAGE/INSTALL.txt" <<EOF
Qivot $VERSION

Header-only (Qt Core + Sql, nothing to build)
  Copy qivot.hpp into your project. In exactly one .cpp:
      #define QIVOT_IMPLEMENTATION
      #include "qivot.hpp"
  and in every other file just:  #include "qivot.hpp"

qmake (everything, including QiListModel / QiLiveListModel)
  include(path/to/$PKG/src/qivot.pri)
  QML forms as well:  include(path/to/$PKG/src/qivot-qml.pri)

CMake
  add_subdirectory(path/to/$PKG)
  target_link_libraries(app PRIVATE Qivot::qivot)
  QML forms as well:  set(QIVOT_WITH_QML ON) before add_subdirectory, then Qivot::qml

Full documentation: README.md
EOF

if command -v zip >/dev/null; then
    (cd "$WORK" && zip -qr "$OUT/$PKG.zip" "$PKG")
else
    python3 -c 'import shutil, sys; shutil.make_archive(sys.argv[1], "zip", sys.argv[2], sys.argv[3])' \
        "$OUT/$PKG" "$WORK" "$PKG"
fi
(cd "$WORK" && tar -czf "$OUT/$PKG.tar.gz" "$PKG")
cp "$WORK/qivot.hpp" "$OUT/qivot.hpp"

# 4. Checksums
(cd "$OUT" && if command -v sha256sum >/dev/null; then
     sha256sum qivot.hpp "$PKG.zip" "$PKG.tar.gz"
 else
     shasum -a 256 qivot.hpp "$PKG.zip" "$PKG.tar.gz"
 fi > SHA256SUMS)

echo "==> $OUT"
ls -lh "$OUT" | awk 'NR > 1 { print "    " $9 "  " $5 }'
