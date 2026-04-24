# 實戰教學：法術與自定義動畫整合 (Dance Spell)

本教學將教你如何製作一個有趣的法術：讓命中的 NPC 開始跳街舞。這涉及法術效果與動畫圖 (Havok Behavior) 的聯動。

## 難度等級與準備工作
- **難度**: 高階 (Hard)
- **準備工具**:
    - **Creation Kit (CK)**: 建立法術與效果。
    - **Open Animation Replacer (OAR)**: 處理自定義動畫。
    - **CommonLibSSE-NG**: (選用) 用於精確控制。

---

## 實作步驟

### 步驟一：準備街舞動畫
1. 獲取一個街舞動畫檔案 (`Dance.hkx`)。
2. 使用 OAR 工具將其放入 `animations/OpenAnimationReplacer` 目錄。
3. 設定觸發條件為：`HasMagicEffect("MyDanceEffect")`。

### 步驟二：建立法術效果
1. 在 CK 中建立一個 `Magic Effect` (EDID: `MyDanceEffect`)。
2. 設置 `Archetype` 為 `Script` 或 `Value Modifier`。
3. 確保 `Magic Effect` 具備一個唯一的關鍵字或 EditorID 以供 OAR 辨識。

### 步驟三：建立施法法術
1. 建立一個 `Spell`。
2. 將剛建立的 `Magic Effect` 加入法術效果清單。
3. 設置投射類型（如 `Aim` 或 `Target`）。

### 步驟四：實作觸發與停止
1. 當 NPC 被法術命中時，由於具備了 `MyDanceEffect`，OAR 會自動將其 Idle 動畫替換為街舞。
2. 當法術持續時間結束，效果消失，NPC 會恢復正常行為。

---

## 代碼實踐 (C++ 強制播放動畫)

如果你想不透過 OAR，直接在插件中控制：

```cpp
void ForceNPCDance(RE::Actor* a_target) {
    if (a_target) {
        // 向動畫圖發送事件
        // 必須確保該事件名稱已在 Behavior Graph 中定義
        a_target->NotifyAnimationGraph("StartDanceEvent");
        
        RE::DebugNotification("音樂響起，NPC 開始跳舞！");
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 對準一個守衛施放法術，觀察他是否立即停止巡邏並開始跳舞。
- **問題 A**: NPC 跳舞時浮在空中？
    - *解決*: 這是動畫的 `Root Motion` 問題。檢查 `.hkx` 檔案是否包含正確的位移數據。
- **問題 B**: 法術結束後 NPC 還是停不下來？
    - *解決*: 確保法術效果結束時會發送一個 `Stop` 事件，或者 OAR 的條件判斷能即時刷新。
- **提示**: 可以加入背景音樂 `RE::BSSoundHandle`，讓舞蹈效果更有臨場感。
