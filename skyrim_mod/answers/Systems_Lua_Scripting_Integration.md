# Skyrim Lua 腳本系統整合架構 (Lua Scripting Integration)

在 Skyrim 中整合 Lua 作為額外的腳本系統，能極大地提昇開發效率，實現動態邏輯更新（Hot Reloading）而無需重啟遊戲。這通常透過 SKSE C++ 插件來實現。

---

## 1. 核心技術棧

- **Lua 解釋器**: 建議使用 **LuaJIT**（高效能）或 **Lua 5.4**。
- **C++ 綁定庫**: 強烈建議使用 **Sol2**。它能極簡化地將 C++ 類別、函數與枚舉導出至 Lua。
- **宿主系統**: SKSE 插件作為宿主，管理 Lua 虛擬機 (LUA STATE) 的生命週期。

---

## 2. 實作流程

### A. 初始化虛擬機
在 `SKSEPluginLoad` 或 `kDataLoaded` 事件中初始化：
```cpp
sol::state lua;
lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math);

// 導出 CommonLibSSE 類別
lua.new_usertype<RE::Actor>("Actor",
    "GetHealth", [](RE::Actor& a) { return a.GetActorValue(RE::ActorValue::kHealth); },
    "SetPos", &RE::Actor::SetPosition
);
```

### B. 腳本加載與執行
建立一個監聽器，每當遊戲啟動或按下特定熱鍵時，讀取 `Data/Scripts/Lua/*.lua` 檔案並執行。

### C. 事件分發 (Event Bridging)
將 Skyrim 的引擎事件傳遞給 Lua：
```cpp
// C++ 監聽事件
void OnHit(RE::TESHitEvent* a_event) {
    // 呼叫 Lua 中定義的函數
    lua["OnNPCHit"](a_event->target, a_event->source);
}
```

---

## 3. 高級特性：熱重載 (Hot Reloading)

Lua 的最大優勢是無需編譯。
- **實作**: 監聽檔案系統變更（或透過遊戲內控制台指令），重新呼叫 `lua.do_file()`。
- **注意**: 重新加載時需要妥善處理全局變數的持久化，避免狀態丟失。

---

## 4. 技術挑戰

- **性能開銷**: 頻繁的 C++/Lua 跨界調用會有負擔。建議將複雜的物理運算留在 C++，Lua 僅處理高層業務邏輯（如任務分支、UI 邏輯）。
- **內存管理**: 必須確保 Lua 指向的 C++ 對象在遊戲中依然有效（使用 `ObjectRefHandle` 而非原始指針）。

---

## 5. 核心類別原始碼標註

- **`RE::BSTEventSink`**: 用於捕獲要傳遞給 Lua 的事件。
- **`RE::TESDataHandler`**: 用於將 Lua 指定的 FormID 轉換為實體對象。

---
*文件路徑：architectures/classified/Systems/Lua_Scripting_Integration.md*
