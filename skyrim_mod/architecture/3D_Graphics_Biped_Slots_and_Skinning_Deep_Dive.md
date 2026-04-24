# 深度解析：蒙皮與分組系統 (Biped & Slots)

Actor 的視覺外貌是由多個幾何體「拼裝」而成的。為了實現換裝功能，引擎使用了 **Biped 槽位系統**。

---

## 1. 槽位概念 (Biped Slots)
- **原始碼**: `include/RE/B/BGSBipedObjectForm.h`
- **機制**: 引擎將人體劃分為 32 個槽位（如：30-Head, 31-Hair, 32-Body, 33-Hands）。
- **邏輯**: 當你裝備一件手套時，引擎會：
    1.  檢查該手套佔用的槽位（33）。
    2.  隱藏（Disable）身體模型中原本屬於該槽位的 Mesh（手部皮膚）。
    3.  顯示（Enable）手套模型中的 Mesh。

---

## 2. 動態蒙皮 (Skinning)
雖然护甲是獨立的模型，但它必須共用 Actor 的骨骼。
- **共享骨骼**: 護甲的 `.nif` 文件中不包含完整的骨骼，只包含骨骼節點的引用。
- **渲染時綁定**: 當護甲被穿上後，其頂點會自動根據 Actor 目前的骨骼位置進行變換。

---

## 3. 分組與隱藏邏輯
有些裝備會同時佔用多個槽位。
- **全封閉頭盔**: 會同時佔用 Hair, Circlet 和 Head 槽位，這會導致玩家的頭髮 Mesh 被強制隱藏，防止穿模。

---

## 4. C++ 操控技巧
你可以透過 C++ 強行隱藏 NPC 的某個部位：
```cpp
void HideBodyPart(RE::Actor* a_actor) {
    auto biped = a_actor->GetBiped();
    if (biped) {
        // 強行卸下某個槽位的視覺表現
        // biped->RemoveObject(RE::BIPED_OBJECT::kBody);
    }
}
```

---

## 5. 核心類別原始碼標註
- **`RE::BGSBipedObjectForm`**: `include/RE/B/BGSBipedObjectForm.h` - 定義物品佔用的槽位掩碼。
- **`RE::TESModelTri`**: `include/RE/T/TESModelTri.h` - 處理頂點形變（如：體重增加導致的肌肉變大）。
- **`RE::BipedAnim`**: `include/RE/B/BipedAnim.h` - 運行時管理 Actor 身上所有已加載裝備的類別。
