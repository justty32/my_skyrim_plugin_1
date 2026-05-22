#!/usr/bin/env bash
# Phase 0 native build of the portable quest-engine core + headless CLI harness
# (SPEC appendix A). Builds on the Manjaro host with clang — NO CommonLibSSE /
# Skyrim required. The core (src/core/) has zero RE::/SKSE:: deps; only the
# nlohmann-json header is needed, located from the vcpkg tree.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Locate nlohmann/json.hpp (vcpkg installed tree first, then a broad fallback).
JSON_INC=""
for c in \
    "$ROOT/build/release-clang-cl-linux/vcpkg_installed/x64-windows-skse-clang/include" \
    "$HOME/vcpkg/packages/nlohmann-json_x64-windows-skse-clang/include"; do
    if [ -f "$c/nlohmann/json.hpp" ]; then JSON_INC="$c"; break; fi
done
if [ -z "$JSON_INC" ]; then
    hit="$(find "$HOME/vcpkg" "$ROOT/build" -name json.hpp -path '*nlohmann*' 2>/dev/null | head -1)"
    [ -n "$hit" ] && JSON_INC="${hit%/nlohmann/json.hpp}"
fi
if [ -z "$JSON_INC" ]; then
    echo "error: could not locate nlohmann/json.hpp (build vcpkg deps first?)" >&2
    exit 1
fi
echo "nlohmann include: $JSON_INC"

OUT="$ROOT/build/cli"
mkdir -p "$OUT"
clang++ -std=c++23 -O0 -g -Wall -Wextra \
    -I"$ROOT/src/core" -I"$JSON_INC" \
    "$ROOT/src/core/QuestEngine.cpp" \
    "$ROOT/tools/cli_harness/main.cpp" \
    -o "$OUT/qe_cli"
echo "built: $OUT/qe_cli"
