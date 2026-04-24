# 步進式教學：建立並實作 Monster Hunter 大型魔物 (Tutorial)

本教學將帶領你完成從 MH 模型匯入到建立一個具備「怒氣機制」的 Skyrim 魔物。

---

## 難度等級：專家 (Expert)

### 準備工具：
1. **Blender**: 搭配 MMD/FBX 工具。
2. **Havok Content Tools**: 用於編輯 `.hkx` 動畫圖。
3. **Creation Kit (CK)**: 用於定義 `TESRace` 與 `Actor`。

---

## 步驟一：模型與骨架準備
1.  **導入模型**: 確保魔物的大小符合 Skyrim 尺度（如雄火龍翼展應約為 1500 單位）。
2.  **綁定權重**: 將魔物網格綁定到自定義骨架上。
3.  **導出 NIF**: 確保包含 `bhkRigidBody` 以實現物理碰撞。

---

## 步驟二：在 CK 中建立新種族
1.  **New Race**: 建立 `MHRaceRathalos`。
2.  **General Data**: 設定其為 `Large` 體型，取消勾選 `Can Fly`（如果使用自定義飛行動畫）。
3.  **Attack Data**: 填入每個動作對應的 `Animation Event` 名稱（如 `Attack_Bite`, `Attack_TailSpin`）。

---

## 步驟三：實作怒氣機制 (Enraged State) - C++
利用 SKSE 監聽魔物的生命值與受擊次數。

```cpp
void UpdateMonsterState(RE::Actor* a_monster) {
    auto& data = GetCustomData(a_monster);
    
    // 當憤怒值滿時
    if (data.rageMeter >= 100.0f) {
        // 1. 切換動畫變量
        a_monster->SetGraphVariableBool("bIsEnraged", true);
        
        // 2. 套用視覺效果 (眼部紅光 VFX)
        a_monster->ApplyArtObject(RageVFX);
        
        // 3. 提升攻擊速度
        a_monster->GetHostileAnimationGraph()->SetSpeed(1.25f);
    }
}
```

---

## 步驟四：招式 AI 分發
1.  **監聽 `WeaponSwing`**: 
    在 C++ 中動態修改魔物的 `CombatStyle`。
2.  **位置判定**:
    如果 `GetDistance(Player) < 300` 且玩家在後方 180 度範圍，強制發送 `TailSwipe` 事件給行為圖。

---

## 驗證方法
1.  **AI 測試**: 使用控制台 `tai` 開啟 AI，觀察魔物是否能正確執行自定義的轉身動作。
2.  **碰撞測試**: 攻擊魔物的不同部位，確認是否能觸發正確的硬直動畫（依賴先前實作的部位破壞系統）。
3.  **怒氣測試**: 持續攻擊魔物，確認其是否會變紅並獲得全新的攻擊招式。

---
*文件路徑：architectures/classified/NPC/Tutorials/Tutorial_Monster_Hunter_Creature_Creation.md*
