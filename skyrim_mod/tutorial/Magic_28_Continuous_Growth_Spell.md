# 28. 技術實戰：倍化之術 (持續性體型增長技能)

本教學展示如何實現一個「引導類法術」：當玩家按住施法鍵對準一個生物時，該生物的體型會隨著時間不斷變大，直到法術停止或達到上限。

## 1. 核心邏輯
1.  **偵測施法**: 監聽玩家施放特定法術（Concentration 類型）。
2.  **鎖定目標**: 獲取法術當前擊中的目標 NPC。
3.  **每幀更新**: 在法術生效期間，每一幀增加目標的 `Scale`（縮放比例）。
4.  **視覺同步**: 調用引擎函數刷新 3D 模型，防止體型變化出現「閃爍」或「延遲」。

## 2. 代碼實現：生長射線

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

class GrowthSpellManager {
public:
    static void UpdateGrowth(RE::Actor* a_caster) {
        if (!a_caster) return;

        // 1. 檢查玩家是否正在施放特定的「生長法術」
        // (假設法術 ID 為 0x00012345)
        auto magicCaster = a_caster->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
        if (magicCaster && magicCaster->currentSpell && magicCaster->currentSpell->formID == 0x00012345) {
            
            // 2. 獲取法術當前照射的目標 (MagicTarget)
            // 在 Concentration 法術中，我們可以透過目標身上的 ActiveEffect 找到它
            auto targetRef = SKSE::GetMessagingInterface()->GetCrosshairRef(); // 簡化：取準星目標
            
            if (targetRef && targetRef->Is(RE::FormType::ActorCharacter)) {
                auto targetActor = targetRef->As<RE::Actor>();
                GrowActor(targetActor);
            }
        }
    }

private:
    static void GrowActor(RE::Actor* a_target) {
        // 3. 獲取當前縮放比例 (1.0 為標準)
        float currentScale = a_target->GetScale();

        // 4. 設定生長邏輯
        float growthSpeed = 0.01f; // 每一幀增加 1%
        float maxScale = 3.0f;     // 最高變大到 3 倍

        if (currentScale < maxScale) {
            float newScale = currentScale + growthSpeed;
            
            // 5. 修改數據層 (SetScale)
            // 原始碼: include/RE/T/TESObjectREFR.h
            a_target->SetScale(newScale);

            // 6. 關鍵：修改視覺層 (更新 3D 模型)
            // 如果不調用這個，NPC 的碰撞體會變大，但視覺外觀可能不會即時更新
            a_target->Update3DModel();
            
            RE::DebugNotification(fmt::format("目標正在生長: {:.2f}x", newScale).c_str());
        }
    }
};

// 7. 在主循環中調用 (Hook Main::Update)
// 參考教學 22：深層 AI Hooking 或監聽每一幀的更新事件
```

## 3. 關鍵 API 標註
-   **`RE::Actor::GetScale()`**: 獲取當前縮放比例。`include/RE/T/TESObjectREFR.h`
-   **`RE::Actor::SetScale()`**: 設置縮放比例。
-   **`RE::Actor::Update3DModel()`**: 強制引擎重新計算 Mesh 和物理碰撞。
-   **`RE::MagicCaster`**: 管理施法狀態的核心組件。`include/RE/M/MagicCaster.h`

## 4. 3D 技術細節解析 (針對弱項補充)

### A. 為何需要 Update3DModel？
在 3D 引擎中，物體的「縮放」涉及到頂點位置的重新計算。`SetScale` 只是修改了內存中的一個數字，而 `Update3DModel` 則是告訴 GPU：「嘿，把這個模型的所有三角形都往外推一點」。

### B. 碰撞體 (Collision) 的連動
Skyrim 的好處是，當你對一個 `Actor` 使用 `SetScale` 並更新後，Havok 物理引擎會自動按比例放大他的碰撞膠囊。這意味著變大的巨人真的會踩到更寬的區域，且更難穿過窄門。

### C. 性能考慮
每一幀更新 `Update3DModel` 是一個重度操作。在實戰中，建議每 5 幀或 10 幀更新一次視覺，或者僅在縮放變化超過一定閾值（如 0.05）時才觸發更新。

## 5. 擴展思路
-   **反向縮放**: 製作一個「縮小射線」，將 `growthSpeed` 改為負數。
-   **屬性聯動**: 體型變大的同時，利用 `SetActorValue` 同步增加對象的 `Health` 和 `MeleeDamage`。
