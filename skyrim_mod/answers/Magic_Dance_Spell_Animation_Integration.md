# Skyrim 法術與自定義動畫整合：以「街舞法術」為例

要實現「施法後開始跳街舞」的效果，需要將 Skyrim 的 **魔法系統 (Magic System)** 與 **行為圖系統 (Havok Behavior/Animation)** 進行對接。

---

## 1. 核心流程架構

1.  **Spell (RE::SpellItem)**: 玩家施放的法術物件。
2.  **Magic Effect (RE::EffectSetting)**: 法術包含的效果。
3.  **Script/C++ Logic**: 當效果生效時，向角色發送「動畫事件 (Animation Event)」。
4.  **Behavior Graph**: 角色接收到事件，切換至對應的「街舞」動畫狀態。

---

## 2. 技術組件實作

### A. 定義法術與效果
- **法術類型**: 建議設為 `Constant Effect` (配合能力) 或 `Fire and Forget` (主動施放)。
- **效果性質**: 腳本化效果 (`Scripted Effect`)。

### B. 動畫觸發代碼 (C++ / SKSE)
在 C++ 插件中，當法術命中或啟動時，使用以下方式觸發：

```cpp
void PlayBreakdance(RE::Actor* a_actor) {
    if (!a_actor) return;

    // 方式 1: 直接發送動畫事件
    // "BreakdanceStart" 必須是 Behavior Graph 中定義過的事件名稱
    a_actor->NotifyAnimationGraph("BreakdanceStart");

    // 方式 2: 如果使用 Papyrus
    // a_actor.PlayAnimation("BreakdanceStart")
}
```

---

## 3. 處理自定義動畫檔 (.HKX)

Skyrim 預設沒有「街舞」動作。你必須處理動畫資源的加載：

- **方案 A: 覆蓋原版動作 (不推薦)**
  將街舞動畫重新命名為 `Idle.hkx`，這會導致所有 NPC 都開始跳街舞。
  
- **方案 B: 使用 Open Animation Replacer (OAR) / DAR (強烈推薦)**
  1.  將街舞動畫放入 `meshes\actors\character\animations\OpenAnimationReplacer\CustomFolder\Breakdance.hkx`。
  2.  設定條件 (Conditions)：
      - `HasMagicEffect("YourDanceEffectEDID")`
      - 或者 `IsSpellCast("YourDanceSpell")`
  3.  **優點**: 無需修改 `Behavior Graph`，只要法術生效，OAR 會自動將角色的 Idle 或特定動作替換為街舞。

---

## 4. 關鍵技術挑戰

### A. 施法動作衝突
- **問題**: 施放法術時，角色手部會有「發射」的動作，這會打斷舞蹈。
- **解決方案**: 
    - 使用 `Instant` 施法時間。
    - 在效果啟動時，短暫禁用（Disable）手部的法術節點，或使用 `RE::Actor::StopCasting()`。

### B. 角色移動與重力
- **Root Motion**: 街舞通常包含大量位移（如地板動作）。
- **注意**: 如果動畫沒有包含 Root Motion 數據，角色會原地旋轉或陷入地面。確保 NIF/HKX 檔案處理了位移數據。

### C. 停止舞蹈
- **機制**: 街舞應該在什麼時候停止？
    - 再次施法。
    - 移動方向鍵（透過 Behavior Graph 的 `Moving` 狀態自動切換）。
    - 設置法術持續時間，結束時發送 `BreakdanceStop` 事件。

---

## 5. 核心類別與資源

- **`RE::NotifyAnimationGraph`**: `include/RE/A/Actor.h` - 觸發動畫的核心。
- **`RE::MagicTarget`**: `include/RE/M/MagicTarget.h` - 處理法術命中的接口。
- **OAR Config**: 撰寫 `config.json` 來精確控制動畫觸發條件。

---
*文件路徑：architectures/classified/Magic/Dance_Spell_Animation_Integration.md*
