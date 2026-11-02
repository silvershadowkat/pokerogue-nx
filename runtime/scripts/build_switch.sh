#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export DEVKITA64="$DEVKITPRO/devkitA64"
pkr_revision="${PKR_COMMIT:-}"
if [[ -z "$pkr_revision" ]]; then pkr_revision="$(git rev-parse --short HEAD)"; fi
cmake -S . -B build-switch -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DPKR_COMMIT="${pkr_revision}-switch" -DPKR_DEBUG=OFF -DPKR_DEV=OFF -DPKR_OFFLINE_TEST=OFF
cmake --build build-switch -j2
