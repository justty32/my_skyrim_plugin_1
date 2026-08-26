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
"$FIXTURE/scripts/pack.sh" --output-dir "$DIST" >/dev/null

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

# cwd 契約：pack.sh 開頭那行 `cd "$REPO_ROOT"` 的可觀測語意，是「相對的 --output-dir
# 要解析到 pack.sh 自己的 repo root，不是呼叫端的 cwd」。所以就從別的目錄、用相對路徑
# 呼叫它，然後看 zip 掉在哪裡。
#
# 這裡不能只寫 `[[ "$PWD" == "$STARTING_PWD" ]]`——那條是恆真的。pack.sh 是子行程，
# 它內部怎麼 cd 都改不到父 shell 的 $PWD。實測：把 `cd /` 加到 pack.sh 尾巴，那條斷言
# 照樣是綠的；連真正的 pack.sh 結束時本來就已經站在 $REPO_ROOT 而不是呼叫端的目錄。
# $PWD 比對可以留著當附加條件，但不能是唯一條件。
CALLER_CWD="$FIXTURE/caller-cwd"
mkdir -p "$CALLER_CWD"
REL_DIST="rel-dist"
STARTING_PWD="$PWD"
set +e
( cd "$CALLER_CWD" && "$FIXTURE/scripts/pack.sh" --output-dir "$REL_DIST" ) >/dev/null 2>&1
rel_rc=$?
set -e
check "$([[ "$rel_rc" -eq 0 ]] && echo 0 || echo 1)" \
      "從別的 cwd 用相對 --output-dir 呼叫 pack.sh 會成功（要 exit 0，實得 $rel_rc）"
check "$([[ -f "$FIXTURE/$REL_DIST/DaylightDungeon-0.0.1.zip" ]] && echo 0 || echo 1)" \
      "相對的 --output-dir 解析到 pack.sh 的 repo root（$REL_DIST/ 在 repo root 底下）"
check "$([[ -e "$CALLER_CWD/$REL_DIST" ]] && echo 1 || echo 0)" \
      "相對的 --output-dir 沒有落在呼叫端 cwd 底下"
check "$([[ "$PWD" == "$STARTING_PWD" ]] && echo 0 || echo 1)" \
      "呼叫端的 \$PWD 沒有被改動（恆真的附加條件，契約在上面三條）"

# 拒絕把輸出指到 pack/ 內（會在重建 staging 時被刪、或把 zip 自己遞迴打包）
set +e
"$FIXTURE/scripts/pack.sh" --output-dir "$FIXTURE/pack/nested" >/dev/null 2>&1
rc=$?
set -e
# 要驗特定的 exit 2，不能只驗「非零」——把防護整段拿掉之後它還是會失敗（exit 15），
# 因為 staging 重建會把輸出目錄一起刪掉。只驗非零的話這條斷言等於沒測到防護本身。
check "$([[ "$rc" -eq 2 ]] && echo 0 || echo 1)" "--output-dir 指向 pack/ 內時明確拒絕（要 exit 2，實得 $rc）"

# 沒有 pre-rename 的 Template_Plugin 殘留（`.ps1` 版也驗這件事，但它是靜態 grep 原始碼，
# 且只在有 pwsh 的機器上跑得到）。這裡驗的是打出來的成品，不是原始碼文字。
if [[ -f "$ZIP" ]]; then
  check "$(grep -q '^Data/SKSE/Plugins/Template_Plugin/' <<<"$(unzip -Z1 "$ZIP")" && echo 1 || echo 0)" \
        "壓縮檔裡沒有 pre-rename 的 Template_Plugin 目錄"
fi

# CLI 契約：--help 要成功，參數錯誤要 exit 2。`.ps1` 版用 regex 比對 pack.sh 的原始碼來驗
# `--help`，那種寫法改個排版就會失效；這裡直接跑。
set +e
"$FIXTURE/scripts/pack.sh" --help >/dev/null 2>&1; help_rc=$?
"$FIXTURE/scripts/pack.sh" --config >/dev/null 2>&1; missing_rc=$?
"$FIXTURE/scripts/pack.sh" --bogus >/dev/null 2>&1; bogus_rc=$?
set -e
check "$([[ "$help_rc" -eq 0 ]] && echo 0 || echo 1)" "--help 回報成功（要 exit 0，實得 $help_rc）"
check "$([[ "$missing_rc" -eq 2 ]] && echo 0 || echo 1)" "--config 缺值時 exit 2（實得 $missing_rc）"
check "$([[ "$bogus_rc" -eq 2 ]] && echo 0 || echo 1)" "未知參數時 exit 2（實得 $bogus_rc）"

if [[ "$failures" -ne 0 ]]; then echo "$failures failure(s)" >&2; exit 1; fi
echo "all packaging contract tests passed"
