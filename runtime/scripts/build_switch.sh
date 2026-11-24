#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-only
set -euo pipefail
cd "$(dirname "$0")/.."
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export DEVKITA64="$DEVKITPRO/devkitA64"
pkr_revision="${PKR_COMMIT:-}"
if [[ -z "$pkr_revision" ]]; then pkr_revision="$(git rev-parse --short HEAD)"; fi
pkr_build="${PKR_SWITCH_BUILD_DIR:-build-switch}"
cmake -S . -B "$pkr_build" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DPKR_COMMIT="${pkr_revision}-switch" -DPKR_DEBUG=OFF -DPKR_DEV=OFF -DPKR_OFFLINE_TEST=OFF
cmake --build "$pkr_build" -j2
