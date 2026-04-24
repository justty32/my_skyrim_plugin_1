# Skyrim 地圖載入、過場畫面與世界轉場邏輯 (Loading & Transitions)

理解 Skyrim 引擎如何處理區域切換（Teleporting）與載入畫面，是實作流暢轉場或自定義載入資訊的基礎。

---

## 1. 轉場機制 (The Transition)

### A. 載入門 (Load Doors)
- **原理**: 當玩家觸發 `RE::TESObjectREFR` (Door) 且該門設有 `Teleport` 目錄時，引擎會調用 `RE::PlayerCharacter::MoveTo()` 並觸發載入程序。

### B. 無縫轉場 (Seamless Transition)
- **挑戰**: 引擎預設會在 Cell 切換時暫停並顯示載入畫面。
- **SKSE 解決方案**: 
    1. 透過 `RE::TESWorldSpace` 的 `Open World` 屬性。
    2. 如果是室內轉室外，可以嘗試使用「大門動畫」配合異步加載（但這需要極高的性能優化，否則會看到空洞）。

---

## 2. 自定義載入畫面 (Custom Loading Screens)

### A. 數據定義 (`RE::TESLoadScreen`)
- **FormType**: 60。
- **內容**: 定義了要顯示的 3D 模型、背景圖片以及隨機出現的提示文字（Loading Tip）。
- **條件**: 你可以設置條件，例如：只有當玩家進入「冬堡」時，才顯示與魔法相關的載入畫面。

### B. C++ 攔截載入狀態
你可以透過 SKSE 監聽載入進度：
```cpp
struct LoadingHandler : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, ...) override {
        // 當 Cell 加載完成時觸發
        // 這是移除「轉場黑幕」或執行初始化代碼的最佳時機
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

---

## 3. 轉場效果優化

1.  **黑屏淡入淡出 (Fade To Black)**: 
    - 使用 `RE::Main::GetSingleton()->ScreenFadeOut()`。
    - 這能遮蓋物件生成時的閃爍感。
2.  **背景音樂銜接**: 在載入期間，透過 `RE::BSSoundHandle` 控制音樂不會因為切換區域而中斷。

---

## 4. 核心類別原始碼標註

- **`RE::TESLoadScreen`**: `include/RE/T/TESLoadScreen.h` - 載入畫面數據。
- **`RE::TESCellFullyLoadedEvent`**: `include/RE/T/TESCellFullyLoadedEvent.h` - 加載完成事件。
- **`RE::PlayerCharacter::MoveTo`**: 核心位移函數。

---
*文件路徑：architectures/classified/Systems/Loading_Screens_and_Transitions.md*
