# 進階 MH AI 與物理邏輯實作 (Advanced Implementation)

本文件提供實作魔物核心邏輯的 C++ (CommonLibSSE-NG) 參考代碼。

---

## 1. 斷尾邏輯：模型動態切換

當尾部受到足夠斬擊傷害時，執行以下代碼：

```cpp
void SeverTail(RE::Actor* a_monster) {
    auto rootNode = a_monster->Get3D();
    if (!rootNode) return;

    // 1. 找到尾部節點
    auto tailNode = rootNode->GetObjectByName("NPC Tail [Tail]");
    if (tailNode) {
        // 2. 獲取座標以便生成掉落物
        RE::NiPoint3 dropPos = tailNode->world.translate;

        // 3. 隱藏原有的完整尾部，顯示斷裂面模型 (通常透過切換 NiNode 的 AppCulled 標籤)
        tailNode->AsNode()->SetAppCulled(true);
        auto brokenTailNode = rootNode->GetObjectByName("NPC Tail Broken");
        if (brokenTailNode) brokenTailNode->AsNode()->SetAppCulled(false);

        // 4. 在世界中生成可採集的尾部實體
        SpawnTailItem(dropPos);
    }
}
```

---

## 2. 動態招式權重系統 (Weighted Move Selection)

AI 不應該隨機出招。我們建立一個基於距離與角度的權重矩陣：

```cpp
RE::BSFixedString SelectBestMove(RE::Actor* a_monster, RE::Actor* a_target) {
    float distance = a_monster->GetDistance(a_target);
    float angle = GetRelativeAngle(a_monster, a_target);

    std::map<RE::BSFixedString, float> moveWeights;

    if (distance > 800.0f) {
        moveWeights["DashAttack"] = 80.0f;
        moveWeights["FireBall"] = 20.0f;
    } else if (angle > 150.0f || angle < -150.0f) {
        moveWeights["TailSwipe"] = 90.0f;
        moveWeights["TurnBack"] = 10.0f;
    }

    return WeightedRandom(moveWeights);
}
```

---

## 3. 數據持久化：保存魔物狀態

使用 `SKSE::Serialization` 確保部位損壞不會因為讀檔而重置：

```cpp
void SaveMonsterData(SKSE::SerializationInterface* a_intfc) {
    for (auto& [handle, data] : g_monsterRegistry) {
        if (auto monster = handle.get()) {
            a_intfc->WriteRecord('MNST', 1, &data, sizeof(MonsterCustomData));
        }
    }
}
```

---

## 4. 核心類別原始碼標註

- **`RE::NiAVObject`**: 模型節點的基底。
- **`SKSE::SerializationInterface`**: 存檔擴展接口。
- **`RE::CharacterProxy`**: 處理角色的物理膠囊體。

---
*文件路徑：architectures/classified/NPC/Answers/Advanced_MH_AI_and_Physics_Implementation.md*
