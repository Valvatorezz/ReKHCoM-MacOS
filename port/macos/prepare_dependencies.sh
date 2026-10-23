#!/bin/sh
# Install build-only dependencies under the repository; no Homebrew changes.
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
DEPS="$PWD/build/macos/deps"
mkdir -p "$DEPS"
[ -x .venv/bin/python ] || python3 -m venv .venv
.venv/bin/python -m pip install 'pyyaml>=6,<7' 'cmake>=3.25,<5' 'ninja>=1.11,<2'
fetch() {
    url="$1"; archive="$2"; checksum="$3"
    [ -f "$DEPS/$archive" ] || curl -fL "$url" -o "$DEPS/$archive"
    actual=$(shasum -a 256 "$DEPS/$archive" | cut -d ' ' -f 1)
    [ "$actual" = "$checksum" ] || { echo "Checksum mismatch: $archive" >&2; exit 1; }
    tar -xzf "$DEPS/$archive" -C "$DEPS"
}
fetch 'https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-34/wasi-sdk-34.0-arm64-macos.tar.gz' wasi-sdk.tar.gz 9c59398106b417f8f14913380fdf0097a8cc0ff4af9eb3ce0065a859e88d49e9
fetch 'https://github.com/WebAssembly/binaryen/releases/download/version_133/binaryen-version_133-arm64-macos.tar.gz' binaryen.tar.gz ad66da82ac13f163e424b1643f16c6dfcccc98b5966296b43e52d3cab04f84a8
fetch 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz' SDL3.tar.gz 9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
W2C2_COMMIT=700f556539ea7c7995e6645b9b80e3edf9c9b271
if [ ! -d "$DEPS/w2c2" ]; then
    git clone --no-checkout https://github.com/turbolent/w2c2.git "$DEPS/w2c2"
    git -C "$DEPS/w2c2" checkout --detach "$W2C2_COMMIT"
fi
[ "$(git -C "$DEPS/w2c2" rev-parse HEAD)" = "$W2C2_COMMIT" ] || { echo 'w2c2 revision differs; use a separate dependency directory.' >&2; exit 1; }
make -C "$DEPS/w2c2/w2c2" -j4
.venv/bin/cmake -S "$DEPS/SDL3-3.4.18" -B "$DEPS/sdl-build" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$PWD/.venv/bin/ninja" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
    -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_HIDAPI_LIBUSB=OFF \
    -DCMAKE_INSTALL_PREFIX="$DEPS/sdl"
.venv/bin/cmake --build "$DEPS/sdl-build" -j4
.venv/bin/cmake --install "$DEPS/sdl-build"
