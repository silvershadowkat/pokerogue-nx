#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export DEVKITA64="$DEVKITPRO/devkitA64"
cmake -S . -B build-switch -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DPKR_COMMIT="$(git rev-parse --short HEAD)-switch" -DPKR_DEBUG=ON -DPKR_OFFLINE_TEST=ON
cmake --build build-switch -j2
