#!/usr/bin/env bash

# 當腳本執行出錯時立即停止
set -e

# ==============================================================================
# 設定區域 (Settings)
# ==============================================================================

# 1. 預設的 CMake 編譯配置 (通常是 release-msvc 或 debug-msvc)
CONFIG="release-msvc"

# 2. 【標註：在此設置 .zip 的輸出位置】
# 你可以修改這裡的 "dist" 成任何你想要的路徑 (例如 "/home/user/my_mods")
OUTPUT_DIR="dist"

# 3. 插件設定檔資料夾名稱 (需與 CMakeLists.txt 中的 CONFIG_FOLDER 一致)
CONFIG_FOLDER_NAME="Template_Plugin"

# ==============================================================================

# 幫助訊息功能
usage() {
    echo "用法: $0 [--config <release-msvc|debug-msvc>] [--output-dir <路徑>]"
    echo "說明:"
    echo "  --config      指定要打包的編譯資料夾 (預設: release-msvc)"
    echo "  --output-dir  指定 ZIP 輸出的目的地 (預設: $OUTPUT_DIR)"
    exit 1
}

# 解析命令列參數
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --config) CONFIG="$2"; shift ;;
        --output-dir) OUTPUT_DIR="$2"; shift ;; # 也可以透過執行時傳參來臨時改變輸出位置
        -h|--help) usage ;;
        *) echo "未知參數: $1"; usage ;;
    esac
    shift
done

# 取得專案根目錄路徑
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# 定義編譯路徑與 CMake 快取檔案路徑
BUILD_DIR="build/$CONFIG"
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"

# 檢查編譯快取是否存在
if [[ ! -f "$CACHE_FILE" ]]; then
    echo "錯誤: 找不到 '$CACHE_FILE'。請先執行 'cmake --preset build-$CONFIG'。"
    exit 1
fi

# 內部函式：從 CMakeCache.txt 提取數值
get_cache_value() {
    local key=$1
    # 搜尋 Key，取得 '=' 後的內容，並移除 Windows 的 \r 換行符
    local value=$(grep "^$key:[^=]*=" "$CACHE_FILE" | head -n 1 | cut -d'=' -f2- | tr -d '\r')
    if [[ -z "$value" ]]; then
        echo "錯誤: 快取中找不到鍵值 '$key' ($CACHE_FILE)。" >&2
        exit 1
    fi
    echo "$value"
}

# 讀取專案名稱與版本號
PLUGIN_NAME=$(get_cache_value "CMAKE_PROJECT_NAME")
PLUGIN_VERSION=$(get_cache_value "CMAKE_PROJECT_VERSION")

# 檢查 DLL 是否已生成
DLL_PATH="$BUILD_DIR/$PLUGIN_NAME.dll"
if [[ ! -f "$DLL_PATH" ]]; then
    echo "錯誤: 找不到 DLL '$DLL_PATH'。請先編譯專案。"
    exit 1
fi

# 準備打包用的暫存資料夾結構 (pack/Data/SKSE/Plugins)
PACK_DIR="$REPO_ROOT/pack"
PLUGINS_DIR="$PACK_DIR/Data/SKSE/Plugins"

echo "正在準備暫存檔案至 $PACK_DIR..."
rm -rf "$PACK_DIR"
mkdir -p "$PLUGINS_DIR"

# 複製編譯好的 DLL
cp "$DLL_PATH" "$PLUGINS_DIR/"

# 如果專案目錄下有 config/ 資料夾，也一併複製進去
CONFIG_SRC="$REPO_ROOT/config"
if [[ -d "$CONFIG_SRC" ]]; then
    CONFIG_DST="$PLUGINS_DIR/$CONFIG_FOLDER_NAME"
    cp -r "$CONFIG_SRC" "$CONFIG_DST"
    echo "已包含設定檔: config/ -> Data/SKSE/Plugins/$CONFIG_FOLDER_NAME/"
fi

# 建立輸出目錄
mkdir -p "$OUTPUT_DIR"

# 決定 ZIP 檔名 (如果是 Debug 版會加上 -Debug 後綴)
BUILD_TAG=""
if [[ "$CONFIG" == "debug-msvc" ]]; then
    BUILD_TAG="-Debug"
fi
ZIP_NAME="$PLUGIN_NAME-$PLUGIN_VERSION$BUILD_TAG.zip"
ZIP_PATH="$OUTPUT_DIR/$ZIP_NAME"

# 檢查環境中是否有 zip 指令
if ! command -v zip &> /dev/null; then
    echo "錯誤: 找不到 'zip' 指令。請先安裝 (例如: sudo apt install zip)。"
    exit 1
fi

# 執行壓縮
rm -f "$ZIP_PATH"
echo "正在壓縮檔案至 $ZIP_PATH..."
(cd "$PACK_DIR" && zip -r "../../$ZIP_PATH" .) > /dev/null

echo ""
echo "打包成功: $ZIP_PATH"
echo "  您可以將此 .zip 檔案直接拖入 MO2 的安裝界面。"
