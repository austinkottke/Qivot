# Qt-for-WebAssembly toolchain, mirroring the repo's wasm.yml CI:
#   Qt 6.7.2  +  Emscripten 3.1.50  (the version pairing Qt pins)
# Host Qt provides moc/rcc/qmlcachegen; wasm Qt is the build target.
# Build the image once (cached), then compile any example against it — see build.sh.
FROM emscripten/emsdk:3.1.50

RUN apt-get update && apt-get install -y --no-install-recommends \
        python3-pip libglib2.0-0 && rm -rf /var/lib/apt/lists/*
RUN pip3 install -U aqtinstall

# Host Qt (linux desktop) — the tool binaries the wasm build runs on the host.
RUN python3 -m aqt install-qt linux desktop 6.7.2 linux_gcc_64 -O /qt
# Target Qt — the single-threaded WebAssembly build (host "all_os" needs a recent aqt).
RUN python3 -m aqt install-qt all_os wasm 6.7.2 wasm_singlethread -O /qt

ENV QT_HOST_PATH=/qt/6.7.2/gcc_64
ENV PATH=/qt/6.7.2/wasm_singlethread/bin:${PATH}
WORKDIR /src
