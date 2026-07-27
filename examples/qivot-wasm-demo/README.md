# Qivot Notes — a Qt for WebAssembly demo

A tiny live notes board that runs **in the browser**: the whole page is a Qt/QML
app compiled to WebAssembly, and every note is a row in an **in-browser SQLite
database** driven by Qivot. Add a note and the list updates itself — it's bound to
a *live* `QiListModel`, so there's no manual reload.

It's deliberately small and web-friendly: **single-threaded**, an **in-memory**
database (`:memory:`), and **no FTS** — which is exactly the subset of Qt that
deploys to plain static hosting (e.g. GitHub Pages) with no special headers.

## Run it natively first

```sh
cd examples/qivot-wasm-demo
qmake && make
./qivotwasmdemo
```

## Build it for the web (WebAssembly)

You need a **Qt for WebAssembly** build and the **exact Emscripten (emsdk)
version** that Qt release was built against — this pairing is the one thing that
trips everyone up.

1. **Install a wasm Qt 6 build.** With [`aqtinstall`](https://github.com/miurahr/aqtinstall):
   ```sh
   aqt install-qt all_os wasm 6.7.2 wasm_singlethread
   ```
   (or use the Qt online installer → *Qt 6.7 → WebAssembly (single-threaded)*).

2. **Install the matching emsdk.** Check the required version for your Qt release
   (`Qt 6.7 → emsdk 3.1.50`; each release pins one — the docs list it):
   ```sh
   git clone https://github.com/emscripten-core/emsdk && cd emsdk
   ./emsdk install 3.1.50 && ./emsdk activate 3.1.50
   source ./emsdk_env.sh
   ```

3. **Build with the wasm qmake:**
   ```sh
   cd examples/qivot-wasm-demo
   ~/Qt/6.7.2/wasm_singlethread/bin/qmake
   make -j
   ```
   This produces `qivotwasmdemo.html`, `qivotwasmdemo.js`, `qivotwasmdemo.wasm`, `qtloader.js`,
   and `qivotwasmdemo.data`.

4. **Try it locally** (wasm must be served over HTTP, not `file://`):
   ```sh
   ~/Qt/6.7.2/wasm_singlethread/bin/qtwasmserver   # or: emrun qivotwasmdemo.html
   ```

5. **Deploy** — host these files on any static host (GitHub Pages, Netlify, …).
   Single-thread wasm needs **no** COOP/COEP headers, so it just works. Open
   [`index.html`](index.html) rather than the generated `qivotwasmdemo.html` — it's a
   nicer loading shell (branding, a spinner, an "unsupported browser" fallback)
   that keeps the app on a canvas underneath.

## Automated build + hosting (recommended)

You don't have to install the toolchain at all. [`.github/workflows/wasm.yml`](../../.github/workflows/wasm.yml)
builds this example for wasm in CI (official `emsdk` + wasm Qt) and **publishes it
to GitHub Pages** on every push — so the "try it in your browser" link stays live
on its own. To turn it on: repo **Settings → Pages → Source: GitHub Actions**. The
two version pins in that file (Qt ↔ emsdk) must match; bump them together.

## Why these choices (the wasm gotchas)

| Choice | Why |
|---|---|
| `:memory:` database | The browser sandbox has no persistent filesystem for a native `.db` file. These notes are seeded at startup, so in-memory is the natural fit. |
| `wasm: QTPLUGIN += qsqlite` (in [`qivot-wasm-demo.pro`](qivot-wasm-demo.pro)) | wasm is a **static** Qt build — plugins aren't loaded at runtime, so the SQLite driver must be linked in. Native/dynamic builds ignore this line. |
| Single-threaded | No `SharedArrayBuffer` / cross-origin-isolation headers required — deploys to any static host. (So this demo avoids `QtConcurrent`/`QiAsync`.) |
| No FTS5 | Qt's bundled wasm SQLite may not enable FTS5. Full-text demos (`lexica`, `clinic`) need it verified first; this one doesn't use it. |

## Files

| File | Role |
|---|---|
| `note.h` | The one model — `text`, `color`, `createdAt`. |
| `notestore.h` / `.cpp` | A **live** `QiListModel` (`setLive`) + add / remove / clear. |
| `main.cpp` | Opens the in-memory DB, seeds a few notes, loads the UI. |
| `main.qml` | The header, the add bar, and the colored note cards. |
| `index.html` | Custom wasm loading shell (branding + spinner + fallback). |
