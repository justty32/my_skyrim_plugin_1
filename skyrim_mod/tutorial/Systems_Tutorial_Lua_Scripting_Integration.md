# 實戰教學：Skyrim Lua 腳本系統整合 (Lua Scripting Integration)

本教學將引導你如何在 SKSE 插件中整合 Lua 虛擬機，實現高效能、可即時更新 (Hot Reload) 的腳本開發環境。

## 難度等級與準備工作
- **難度**: 極高 (Very Hard)
- **準備工具**:
    - **Visual Studio 2022**: 開發 C++ 插件。
    - **Sol2**: C++/Lua 綁定庫。
    - **LuaJIT**: 高效能 Lua 解釋器。

---

## 實作步驟

### 步驟一：環境搭建與 Sol2 初始化
1. 在你的 SKSE 專案中引入 `sol2` 頭文件。
2. 在插件啟動時建立一個 `sol::state` 對象。
3. 開啟基礎庫：`lua.open_libraries(sol::lib::base, sol::lib::package);`

### 步驟二：導出 CommonLibSSE 類別
1. 選擇你想在 Lua 中使用的類別（如 `RE::Actor`）。
2. 使用 `lua.new_usertype` 定義映射，導出成員函數或變數。
3. 導出單例獲取函數，如 `RE::PlayerCharacter::GetSingleton()`。

### 步驟三：實作腳本加載與熱重載
1. 建立一個專用的資料夾 `Data/Scripts/Lua/`。
2. 在 C++ 中封裝一個 `ReloadScripts()` 函數，調用 `lua.do_file()` 重新執行入口文件。
3. 監聽一個控制台命令或熱鍵，觸發此重新加載邏輯。

### 步驟四：事件分發 (C++ -> Lua)
1. 在 C++ 中繼承 `RE::BSTEventSink`。
2. 在事件回調中，檢查 Lua 全局表中是否存在對應的處理函數。
3. 如果存在，則呼叫該函數並傳入參數。

---

## 代碼實踐 (C++ - Sol2 範例)

```cpp
#include <sol/sol.hpp>
#include <RE/Skyrim.h>

sol::state g_lua;

void InitLua() {
    g_lua.open_libraries(sol::lib::base);

    // 導出 Actor 類別
    g_lua.new_usertype<RE::Actor>("Actor",
        "GetName", [](RE::Actor& a) { return a.GetFullName(); },
        "GetHealth", [](RE::Actor& a) { return a.GetActorValue(RE::ActorValue::kHealth); }
    );

    // 導出獲取玩家的函數
    g_lua.set_function("GetPlayer", []() { return RE::PlayerCharacter::GetSingleton(); });

    // 執行 Lua 文件
    g_lua.do_file("Data/Scripts/Lua/Main.lua");
}

// 在 Lua 文件 (Main.lua) 中可以寫：
/*
player = GetPlayer()
print("玩家姓名: " .. player:GetName())
*/
```

---

## 常見問題與驗證
- **驗證方式**: 在 Lua 中寫一個 `print`，檢查遊戲啟動時 `skse64.log` 是否出現對應文字。
- **問題 A**: 重新加載後變數重置？
    - *解決*: Lua 的全局變數在 `do_file` 時會被覆蓋。若要保留狀態，需在 Lua 中將數據存入一個不被覆蓋的「持久化表」中。
- **問題 B**: 指針崩潰？
    - *解決*: 永遠不要在 Lua 中長時間持有 C++ 對象的指針。建議傳遞 `FormID` 並在 Lua 每次需要時重新獲取物件。
- **效能提示**: 避免在 `OnUpdate` 每幀事件中進行大量 Lua 計算，這會顯著降低 FPS。
