# 終極挑戰：純代碼創世之舉 (概念與極限)

建立一個城鎮是 Skyrim 模組開發中最龐大的工程之一。如果你挑戰**「完全透過 C++ 純插件在運行時動態生成一個城鎮」**，這在技術上是可行的，但你將面臨引擎最底層的諸多限制。

本系列將深入探討這個瘋狂想法的實現步驟，並嘗試突破它的邊界。

## 1. 核心邏輯與步驟

要用代碼建城，你需要依序完成以下步驟：

### 第一步：地基與建築 (Statics)
你需要大量的坐標數據，精確地 `PlaceAtMe` 生成房屋外殼、城牆、地板。
```cpp
void BuildTownStructures(RE::NiPoint3 centerPos) {
    auto player = RE::PlayerCharacter::GetSingleton();
    
    // 1. 生成酒館外殼 (PlaceHolder: 0x酒館模型ID)
    auto tavernBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x12345);
    auto tavern = player->PlaceAtMe(tavernBase, 1, true, false); // 必須設為持久化(Persistent)
    tavern->SetPosition({centerPos.x + 1000, centerPos.y + 1000, centerPos.z});
}
```

### 第二步：定義區域 (BGSLocation)
建築有了，但引擎不知道這是一個“城鎮”。你需要動態創建一個 `BGSLocation`。
```cpp
void CreateTownLocation() {
    auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSLocation>();
    if (factory) {
        auto newTownLoc = factory->Create()->As<RE::BGSLocation>();
        // 添加 Keyword (例如 LocTypeCity, LocTypeTown)
    }
}
```

## 2. 致命的技術限制

在純插件環境下，你無法解決以下三大引擎限制：

### 限制 A：導航網格 (Navmesh) 缺失
動態生成的建築不會產生導航網格。NPC 根本不知道地上有房子，他們會直接撞牆卡住，或者無法在你的城鎮裡行走。Navmesh 是在 Creation Kit 中預先烘焙的，純 C++ 無法實時生成數萬個多邊形的尋路網格。

### 限制 B：遠景模型 (LOD) 缺失
你的城鎮只有在玩家走得很近時才會突然“彈出（Pop-in）”。LOD 是預先烘焙的低模地貌文件，運行時動態生成的物體沒有遠景數據。

### 限制 C：遮擋剔除 (Occlusion Culling) 與性能崩潰
純代碼生成的幾百個獨立物件無法參與引擎的預合併（Precombined Meshes），會讓 Draw Call 暴增，導致嚴重的掉幀。

---

## 3. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/T/TESForm.h` - 基礎表單查找。
- `include/RE/T/TESObjectREFR.h` - 世界實體操作。
- `include/RE/I/IFormFactory.h` - 動態對象創建。

### 推薦使用的函數
- `RE::TESForm::LookupByID<T>(FormID)`：獲取建築模板。
- `RE::TESObjectREFR::PlaceAtMe(BaseForm, Count, Persistent, Disabled)`：在世界中生成物體。
- `RE::TESObjectREFR::SetPosition(NiPoint3)`：設置物體物理坐標。
- `RE::TESObjectREFR::SetAngle(float x, float y, float z)`：設置旋轉角度。
- `RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSLocation>()`：獲取區域工廠。
