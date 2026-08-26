#!/usr/bin/env bash
# pack.sh 的打包契約測試（POSIX 版）。
#
# 這支是 `test_packaging.ps1` 的對等物，但測的是 `pack.sh` 而不是 `pack.ps1`——
# 兩支打包腳本各自獨立，Linux 那條路以前完全沒有回歸防護。
#
# 手法跟 .ps1 版一樣：在暫存目錄造一個最小 fixture（假的 CMakeCache、2 bytes 的
# 假 DLL、真的 config/），跑 pack.sh，然後驗 zip 裡的路徑佈局。
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACK_SCRIPT="${PACK_SCRIPT:-$REPO_ROOT/scripts/pack.sh}"
FIXTURE="$(mktemp -d "${TMPDIR:-/tmp}/daylight-dungeon-packaging-XXXXXX")"
trap 'rm -rf "$FIXTURE"' EXIT

failures=0
check() {
  if [[ "$1" == "0" ]]; then echo "PASS: $2"; else echo "FAIL: $2" >&2; failures=$((failures + 1)); fi
}

BUILD_DIR="$FIXTURE/build/release-clang-cl-linux"
mkdir -p "$FIXTURE/scripts" "$FIXTURE/config" "$BUILD_DIR"
cp "$PACK_SCRIPT" "$FIXTURE/scripts/pack.sh"
cp "$REPO_ROOT/config/FollowLight.ini" "$FIXTURE/config/"
cat > "$BUILD_DIR/CMakeCache.txt" <<'CACHE'
CMAKE_PROJECT_NAME:STATIC=DaylightDungeon
CMAKE_PROJECT_VERSION:STATIC=0.0.1
PLUGIN_CONFIG_FOLDER:STRING=DaylightDungeon
CACHE
printf 'MZ' > "$BUILD_DIR/DaylightDungeon.dll"

DIST="$FIXTURE/test-dist"
STARTING_PWD="$PWD"
"$FIXTURE/scripts/pack.sh" --output-dir "$DIST" >/dev/null
check "$([[ "$PWD" == "$STARTING_PWD" ]] && echo 0 || echo 1)" "pack.sh 沒有改變呼叫端的工作目錄"

ZIP="$DIST/DaylightDungeon-0.0.1.zip"
check "$([[ -f "$ZIP" ]] && echo 0 || echo 1)" "產生了 DaylightDungeon-0.0.1.zip"
if [[ -f "$ZIP" ]]; then
  entries="$(unzip -Z1 "$ZIP")"
  check "$(grep -qx 'Data/SKSE/Plugins/DaylightDungeon.dll' <<<"$entries" && echo 0 || echo 1)" \
        "DLL 在 Data/SKSE/Plugins/ 底下"
  check "$(grep -qx 'Data/SKSE/Plugins/DaylightDungeon/FollowLight.ini' <<<"$entries" && echo 0 || echo 1)" \
        "設定檔在 runtime 的 DaylightDungeon/ 子目錄底下"
  # 注意：這裡要用 `! grep -q`，不是 `grep -qv`——後者只要有任一行不符就回 0，恆真。
  check "$(grep -q '^pack/' <<<"$entries" && echo 1 || echo 0)" \
        "壓縮檔裡沒有 staging 目錄本身"
fi

# 拒絕把輸出指到 pack/ 內（會在重建 staging 時被刪、或把 zip 自己遞迴打包）
set +e
"$FIXTURE/scripts/pack.sh" --output-dir "$FIXTURE/pack/nested" >/dev/null 2>&1
rc=$?
set -e
# 要驗特定的 exit 2，不能只驗「非零」——把防護整段拿掉之後它還是會失敗（exit 15），
# 因為 staging 重建會把輸出目錄一起刪掉。只驗非零的話這條斷言等於沒測到防護本身。
check "$([[ "$rc" -eq 2 ]] && echo 0 || echo 1)" "--output-dir 指向 pack/ 內時明確拒絕（要 exit 2，實得 $rc）"

if [[ "$failures" -ne 0 ]]; then echo "$failures failure(s)" >&2; exit 1; fi
echo "all packaging contract tests passed"
