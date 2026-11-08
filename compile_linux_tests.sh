#!/usr/bin/env bash
set -euo pipefail

# Configure, compile and run the QtTest suite with CMake + Ninja + CTest.
# Usage: ./compile_linux_tests.sh [debug|release]
if (( $# > 1 )); then
    printf 'Usage: %s [debug|release]\n' "${0##*/}" >&2
    exit 2
fi
mode="${1:-debug}"
case "$mode" in
    debug) build_type=Debug ;;
    release) build_type=Release ;;
    *) printf 'Unknown build type: %s (use debug or release)\n' "$mode" >&2; exit 2 ;;
esac

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
build_dir="$root/build-$mode"

cmake -S "$root" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DBUILD_TESTING=ON
cmake --build "$build_dir" --target tst_core tst_keyboard_qt --parallel
ctest --test-dir "$build_dir" --output-on-failure
