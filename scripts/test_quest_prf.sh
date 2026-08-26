#!/usr/bin/env bash
# quest engine PRF primitives 的可攜離線測試（POSIX 版）。
#
# 這支是 `test_quest_prf.ps1` 的對等物：同一份原始碼、同一組編譯旗標、同一個 g++。
# 之所以要有兩份，只是因為驅動外殼不同——測試本身不依賴 SKSE／CommonLibSSE，
# 純 stdlib，所以 Linux 上直接編得起來、跑得動。
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${QUEST_PRF_TEST_OUTPUT_DIR:-$REPO_ROOT/build/portable-tests-posix}"
EXECUTABLE="$OUTPUT_DIR/quest_prf_test"

mkdir -p "$OUTPUT_DIR"

"${CXX:-g++}" \
  -std=c++23 -O2 -Wall -Wextra -pedantic-errors \
  -finput-charset=UTF-8 -fexec-charset=UTF-8 \
  -I"$REPO_ROOT/src/core" \
  "$REPO_ROOT/src/core/DeterministicRandom.cpp" \
  "$REPO_ROOT/tools/quest_prf_test.cpp" \
  -o "$EXECUTABLE"

"$EXECUTABLE"
