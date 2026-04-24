# SKSE 插件基礎 - HelloWorld (HelloWorld)

這是一個最簡單的 SKSE 插件示例，使用了 CommonLibSSE NG 的簡化插件宣告。

## 項目結構
- **`plugin.cpp`**: 插件的主邏輯。
- **`CMakeLists.txt`**: 構建配置。
- **`PCH.h`**: 預編譯頭文件（提高編譯速度）。
- **`vcpkg.json`**: 定義依賴項。

## 核心代碼解析 (`plugin.cpp`)

```cpp
SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    // 1. 初始化 SKSE 接口
    SKSE::Init(skse);

    // 2. 註冊消息監聽器
    // 當遊戲數據加載完成 (kDataLoaded) 時觸發
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            // 3. 在遊戲控制台打印消息
            RE::ConsoleLog::GetSingleton()->Print("Hello, world!");
        }
    });

    return true;
}
```

### 關鍵點：
1. **`SKSEPluginLoad`**: 插件的入口函數，由 SKSE 加載器調用。
2. **`SKSE::Init(skse)`**: 必須首先調用，以設置 CommonLibSSE 與遊戲的通訊。
3. **消息監聽 (Messaging Interface)**: 這是插件與遊戲引擎或其他插件通訊的主要方式。
   - `kDataLoaded`: 保證在此時引擎的大多數單例（如 `ConsoleLog`, `PlayerCharacter`）都已經就緒。
4. **`RE::ConsoleLog`**: 一個 RE 單例，用於訪問遊戲內的控制台。

## 構建配置 (`CMakeLists.txt`)

使用 `add_commonlibsse_plugin` 宏，它會自動處理：
- 生成 `.dll` 文件。
- 自動導出 SKSE 所需的 `SKSEPlugin_Version` 或 `SKSEPlugin_Query`（在 NG 版本中由宏自動完成）。
- 設置正確的編譯選項和鏈接 CommonLibSSE。

```cmake
find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES plugin.cpp)
```

## 總結
這個例子展示了製作插件的最基本步驟：初始化 -> 等待數據加載 -> 執行簡單操作。對於大多數模組，你都會從 `kDataLoaded` 事件開始執行你的初始化邏輯。
