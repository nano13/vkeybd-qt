#!/usr/bin/env bash
set -euo pipefail

# Backward-compatible build-and-run entry point; no qmake dependency.
# Usage: ./compile.sh [debug|release]
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
"$root/compile_linux.sh" "$@"
mode="${1:-debug}"
exec "$root/build-$mode/vkeybd-qt"
