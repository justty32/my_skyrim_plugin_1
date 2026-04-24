# Skyrim 世界交互與觸發機制：陷阱、區域、門與鎖

Skyrim 的沉浸感很大程度上來自於與環境的交互。從踩到陷阱到破解門鎖，這些行為在引擎中都有一套標準的「偵測 -> 處理 -> 響應」流程。

---

## 1. 觸發區域 (Trigger Zones)
觸發區域通常是世界中不可見的幾何體，當 Actor 進入或離開時觸發腳本。
- **類別**: `RE::TESObjectREFR` (Base Form 類型為 `BGSTriggerInstance`)
- **關鍵組件**: 
    - **Primitive**: 定義區域的物理形狀（立方體、球體）。
    - **原始碼**: `include/RE/B/BGSPrimitive.h`
- **事件流程**:
    1. 物理引擎檢測到 `Actor` 的碰撞。
    2. 發送 `OnTriggerEnter` 事件。
    3. C++ 插件可透過 Hook `Activate` 函數或監聽特定的 `ScriptEvent` 來攔截這些動作。

---

## 2. 陷阱系統 (Traps)
陷阱是觸發器與動作對象（如箭矢發射器、擺錘）的結合。
- **機制**: 陷阱通常由一個「傳感器（如地壓板、細線）」和一個「啟動器」組成。
- **傳感器**: 也是一種 `TESObjectREFR`，繼承了 `Activate` 邏輯。
- **動作觸發**: 當玩家踩到壓力板時，傳感器調用 `Activate()`，這會向關聯的啟動器發送 `RE::UI_MESSAGE_TYPE::kActivate` 消息。

---

## 3. 門與加載門 (Doors)
門不僅是模型，它是空間連接器。
- **原始碼**: `include/RE/T/TESObjectDOOR.h`
- **組成內容**:
    - **Open/Close Sound**: 開關音效。
    - **Teleport Data**: 前往另一個單元的數據（見教學 Ultimate Town Challenge 02）。
- **交互標記**: `RE::TESObjectREFR` 的 `ExtraLock` 或 `ExtraTeleport` 定義了門的特殊行為。

---

## 4. 鎖系統 (Locking System)
任何可交互的 `TESObjectREFR` (箱子、門) 都可以帶有鎖。
- **原始碼**: `include/RE/E/ExtraLock.h`
- **數據結構 (`RE::REFR_LOCK`)**:
    - **`level`**: 鎖的難度（新手、專家、大師）。
    - **`key`**: 關聯的鑰匙 FormID。
    - **`isLocked`**: 鎖定狀態標記。
- **C++ 介入**:
    ```cpp
    auto lock = ref->extraList.GetByType<RE::ExtraLock>();
    if (lock && lock->lock->isLocked) {
        // 插件可以直接強制開鎖
        lock->lock->isLocked = false;
    }
    ```

---

## 5. 交互事件鏈 (Interaction Chain)

當你按下 `E` 鍵對準一扇門時：
1.  **Input**: 發送 `InputEvent` 到引擎。
2.  **Pick**: 引擎射線檢測，找到準星指向的 `RE::TESObjectREFR`。
3.  **Check**: 檢查 `ExtraLock`。如果鎖住了且玩家沒鑰匙，進入開鎖界面（`LockpickingMenu`）。
4.  **Activate**: 如果沒鎖，調用 `ref->Activate(player, ...)`。
5.  **Teleport**: 如果有 `ExtraTeleport`，執行單元跳轉。

---

## 6. 技術總結：如何控制這些事件？

- **陷阱無效化**: 透過 C++ 尋找帶有 `Trap` 標籤的 `REFR` 並將其 `Disable()`。
- **自動開鎖**: 攔截 `LockpickingMenu` 的加載事件，直接設置 `ExtraLock` 為非鎖定狀態。
- **區域感知**: 透過 Hook `RE::BGSTriggerInstance` 的處理函數，你可以實現“玩家進入某個神秘山洞時自動存檔”的功能。

## 7. 核心類別原始碼標註

- **`RE::ExtraLock`**: `include/RE/E/ExtraLock.h` - 鎖定數據。
- **`RE::TESObjectDOOR`**: `include/RE/T/TESObjectDOOR.h` - 門的定義。
- **`RE::BGSPrimitive`**: `include/RE/B/BGSPrimitive.h` - 觸發區域幾何體。
- **`RE::REFR_LOCK`**: `include/RE/R/REFR_LOCK.h` - 具體的鎖狀態。
