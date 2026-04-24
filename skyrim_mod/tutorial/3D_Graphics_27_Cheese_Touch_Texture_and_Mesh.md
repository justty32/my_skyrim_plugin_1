# 27. 技術實戰：起司之觸 (材質覆蓋與模型掛載)

本教學將帶領您進入模組開發的「藝術領域」。我們將實現一個荒誕又酷炫的功能：施展龍吼後，你手中的武器不僅會覆蓋上一層起司材質，還會在劍柄上掛載一個真實的起司裝飾品。

## 1. 核心邏輯
1.  **材質覆蓋 (Texture Overriding)**: 使用 `ExtraTextureSet` 將武器原本的金屬/木頭材質替換為起司貼圖。
2.  **模型掛載 (Mesh Attachment)**: 透過 `NiNode` 操作，在武器的模型樹中動態插入一個起司的 `.nif` 模型。

## 2. 代碼實現

### 第一部分：覆蓋起司材質
這會讓整把劍看起來像是用起司雕刻而成的。

```cpp
#include <RE/Skyrim.h>

void ApplyCheeseTexture(RE::TESObjectREFR* a_itemRef) {
    // 1. 獲取起司材質集 (PlaceHolder ID: 0x起司TextureSetID)
    auto cheeseTextureSet = RE::TESForm::LookupByID<RE::BGSTextureSet>(0x000669A3);
    
    if (a_itemRef && cheeseTextureSet) {
        // 2. 創建材質覆蓋數據
        auto extraTexture = new RE::ExtraTextureSet(cheeseTextureSet);
        
        // 3. 掛載到物品的 ExtraDataList
        a_itemRef->extraList.Add(extraTexture);
        
        // 4. 強制刷新視覺
        a_itemRef->Update3DModel();
        RE::DebugNotification("武器已被起司化！");
    }
}
```

### 第二部分：掛載起司裝飾品 (Mesh)
這會在武器上增加一個實體物件。

```cpp
void AttachCheeseDecoration(RE::Actor* a_player) {
    // 1. 獲取右手武器的 3D 根節點
    auto weaponNode = a_player->Get3D(false); 
    if (!weaponNode) return;

    // 2. 加載起司模型 (include/RE/N/NiStream.h)
    RE::NiStream stream;
    if (stream.Load("Meshes\\Clutter\\Food\\CheeseWheel01.nif")) {
        auto cheeseMesh = stream.GetObjectAt<RE::NiAVObject>(0);
        
        if (cheeseMesh) {
            // 3. 調整裝飾品大小與位置 (縮小到像個吊飾)
            cheeseMesh->local.scale = 0.1f; 
            cheeseMesh->local.translate = { 0.0f, 0.0f, 10.0f }; // 往上偏移到護手處

            // 4. 掛載到武器節點
            auto weaponNiNode = weaponNode->AsNode();
            weaponNiNode->AttachChild(cheeseMesh, true);
            
            // 5. 更新模型狀態
            cheeseMesh->Update(nullptr);
            RE::DebugNotification("添加了起司裝飾品。");
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`RE::BGSTextureSet`**: 材質定義集。`include/RE/B/BGSTextureSet.h`
-   **`RE::ExtraTextureSet`**: 實體引用的材質覆蓋組件。`include/RE/E/ExtraTextureSet.h`
-   **`RE::NiStream`**: NIF 檔案讀取流。`include/RE/N/NiStream.h`
-   **`RE::NiNode::AttachChild()`**: 將一個 3D 對象設為另一個的子節點。`include/RE/N/NiNode.h`

## 4. 實戰建議
-   **材質拉伸**: 如果武器的 UV 展開與起司貼圖不匹配，覆蓋後的效果可能會很奇怪。這通常需要專業的材質調優。
-   **座標補償**: 不同武器的中心點（Pivot）不同，起司裝飾品的位置可能需要根據武器類型動態調整。
-   **永久性**: 透過 `AttachChild` 添加的物體在存檔後會消失（因為它是內存中的 3D 樹操作）。若要永久保留，請參考「終極挑戰」中的持久化技術。
