#!/usr/bin/env bash
# Build the Qivot Vision example for WebAssembly using Docker — no local Qt-wasm
# toolchain needed. Produces vision.html / vision.js / vision.wasm + qtloader.js
# in ./dist (or the directory given as $1).
#
#   ./build.sh                 # build, output to examples/vision/wasm/dist
#   ./build.sh /some/other/dir # build, output elsewhere
#
# Then:  (cd dist && python3 -m http.server 8000)  and open http://localhost:8000/vision.html
#
# Note: if `docker build` fails with a "docker-credential-*: executable file not
# found" error, put your Docker credential helper on PATH (e.g. Docker Desktop's
# .../Contents/Resources/bin) — the emscripten base image is a public pull.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../../.." && pwd)"          # examples/vision/wasm -> repo root
OUT="${1:-$HERE/dist}"
mkdir -p "$OUT"

echo "==> Building the Qt-wasm toolchain image (cached after the first run)…"
docker build -t qivot-wasm "$HERE"

echo "==> Compiling examples/vision for WebAssembly…"
# Repo mounted read-only; a shadow build in /build keeps the source tree clean.
docker run --rm -v "$REPO":/src:ro -v "$OUT":/out qivot-wasm bash -lc '
  mkdir -p /build && cd /build &&
  qmake /src/examples/vision/vision.pro &&
  make -j"$(nproc)" &&
  cp -v vision.html vision.js vision.wasm qtloader.js qtlogo.svg /out/'

echo
echo "==> Done. Artifacts in: $OUT"
echo "    Serve:  (cd \"$OUT\" && python3 -m http.server 8000)"
echo "    Open:   http://localhost:8000/vision.html"
