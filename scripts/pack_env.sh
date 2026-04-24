#!/usr/bin/env bash

# ==============================================================================
# 此腳本會讀取環境變數來動態決定打包的輸出位置
# ==============================================================================

# 1. 優先檢查自定義變數 PLUGIN_DIST_PATH
# 2. 次要檢查開發環境常用的 SKYRIM_MODS_FOLDER
# 3. 若都沒設定，預設為專案目錄下的 dist
TARGET_DIR="${PLUGIN_DIST_PATH:-$SKYRIM_MODS_FOLDER}"
TARGET_DIR="${TARGET_DIR:-dist}"

echo "--- 環境變數檢查 ---"
echo "PLUGIN_DIST_PATH   : ${PLUGIN_DIST_PATH:-[未設定]}"
echo "SKYRIM_MODS_FOLDER : ${SKYRIM_MODS_FOLDER:-[未設定]}"
echo "最終打包輸出路徑   : $TARGET_DIR"
echo "--------------------"
echo ""

# 取得腳本所在的目錄
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 呼叫原始的 pack.sh 並傳入動態決定的路徑
# "$@" 會將你傳給此腳本的其他參數（例如 --config debug-msvc）傳遞下去
bash "$SCRIPT_DIR/pack.sh" --output-dir "$TARGET_DIR" "$@"
