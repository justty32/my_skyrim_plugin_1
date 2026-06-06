# 室內 Cell 環境光（Ambient）runtime 調亮 — 可行性分析（header-verified）

本檔目的：評估「在 SKSE plugin 執行期把玩家所在的室內 cell 變亮（提升環境光 ambient）」是否可行、走哪條路風險最低。
搭配閱讀：`COMMONLIBSSE_INDEX.md`（class/方法總覽）、`MODDING_COOKBOOK.md`（已驗證模式）、`PITFALLS.md`（編譯/執行雷區）。

**所有 API 結論都標 `header檔名:行號`。** headers 根目錄（下文簡稱 `RE/`）：
`/home/lorkhan/vcpkg/buildtrees/commonlibsse-ng-fork/src/4d01e0ac3d-d525aaea55.clean/include/RE/`

> 重要前提：header 只能證明「資料結構長怎樣、有哪些方法」。**「改了會不會即時生效」header 無法證明**，需要 in-game spike。本檔對「生效路徑」一律誠實標示推測/待確認。

---

## 必答 1：InteriorData 結構（`RE/I/InteriorData.h`）

結構名是 `RE::INTERIOR_DATA`（注意：是 `struct INTERIOR_DATA`，不是 `InteriorData` class）。`sizeof == 0x60`（`InteriorData.h:47`）。光照相關欄位：

| 欄位 | 型別 | 偏移 | header:行 | 說明 |
|------|------|------|-----------|------|
| `ambient` | `RE::Color` | 00 | `InteriorData.h:27` | **環境光顏色（要調亮的主目標）** |
| `directional` | `RE::Color` | 04 | `InteriorData.h:28` | 方向光顏色 |
| `fogColorNear` | `RE::Color` | 08 | `InteriorData.h:29` | 近霧色 |
| `fogNear` | `float` | 0C | `InteriorData.h:30` | 霧起始距離 |
| `fogFar` | `float` | 10 | `InteriorData.h:31` | 霧結束距離 |
| `directionalXY` | `std::uint32_t` | 14 | `InteriorData.h:32` | 方向光 XY 旋轉（packed） |
| `directionalZ` | `std::uint32_t` | 18 | `InteriorData.h:33` | 方向光 Z 旋轉（packed） |
| `directionalFade` | `float` | 1C | `InteriorData.h:34` | 方向光淡出 |
| `clipDist` | `float` | 20 | `InteriorData.h:35` | clip 距離 |
| `fogPower` | `float` | 24 | `InteriorData.h:36` | 霧次方 |
| `directionalAmbientLightingColors` | `RE::BGSDirectionalAmbientLightingColors` | 28 | `InteriorData.h:37` | **六方向 ambient（DALC，見必答 3）** |
| `fogColorFar` | `RE::Color` | 48 | `InteriorData.h:38` | 遠霧色 |
| `fogClamp` | `float` | 4C | `InteriorData.h:39` | 霧 clamp |
| `lightFadeStart` | `float` | 50 | `InteriorData.h:40` | 燈光淡出起點 |
| `lightFadeEnd` | `float` | 54 | `InteriorData.h:41` | 燈光淡出終點 |
| `lightingTemplateInheritanceFlags` | `REX::EnumSet<Inherit, uint32_t>` | 58 | `InteriorData.h:42` | 哪些欄位繼承自 LGTM（見下） |

**型別釐清**：`ambient` / `directional` / 各 fog 色都是 **`RE::Color`**（`RE/C/Color.h`），是 **8-bit RGBA**（`red/green/blue/alpha` 各 `std::uint8_t`，`Color.h:274-277`，`sizeof==0x4`，`Color.h:281`）。**不是 NiColor（float 0-1）**。
→ 要調亮就是把 `ambient.red/green/blue` 從原值往上加（上限 255），構造可用 `Color(r,g,b,a)`（`Color.h:41`）。

**繼承旗標**（`InteriorData::Inherit`，`InteriorData.h:11-24`）：`kAmbientColor = 1<<0`、`kDirectionalColor = 1<<1` … 表示該 cell 的某欄位是否「繼承自 lighting template (LGTM)」。若 `kAmbientColor` 有設，cell 自身的 `ambient` 可能不是引擎實際採用值（實際值來自 LGTM）。**這是最大未知數之一**：直接寫 `INTERIOR_DATA::ambient` 對「繼承 LGTM 的 cell」可能無效，需先清掉對應 inherit bit 或改 LGTM。待 in-game 確認。

---

## 必答 2：TESObjectCELL 怎麼拿到 interior 光照（`RE/T/TESObjectCELL.h`）

**cellData union**（`TESObjectCELL.h:173-178`）：
```cpp
union CellData {
    EXTERIOR_DATA* exterior;  // XCLC（外景）
    INTERIOR_DATA* interior;  // XCLL（室內）—— 我們要的
};
```
這個 union 位於 `RUNTIME_DATA::cellData`（`TESObjectCELL.h:229`），透過 `GetRuntimeData()` 取得（`TESObjectCELL.h:248-256`，AE/SE 偏移用 `RelocateMemberIfNewer`，已處理跨版本）。

**取 InteriorData 的官方捷徑**（最推薦，免自己碰 union）：
- `INTERIOR_DATA* GetLighting();`（`TESObjectCELL.h:206`）—— 直接回 interior 光照指標。**這是首選入口**。

**判斷 cell 是否 interior**：
- `bool IsInteriorCell() const;`（`TESObjectCELL.h:215`）／`IsExteriorCell()`（`:214`）。
- 旗標佐證：`Flag::kIsInteriorCell = 1<<0`（`TESObjectCELL.h:129`），存於 `cellFlags`（`:261`）。

**現成的 fog setter（重要 precedent）**：CELL 自帶
- `SetFogColor(Color near, Color far)`（`TESObjectCELL.h:218`）
- `SetFogPlanes(float near, float far)`（`:219`）、`SetFogPower(float)`（`:220`）

→ 這些是引擎內建的「改 interior 光照欄位」函式，**證明引擎本身有改 cell 光照子集的 API**，但**注意：只有 fog 有 setter，ambient 沒有對應的 `SetAmbient()`**（header 內找不到）。ambient 只能直接寫 `GetLighting()->ambient`。是否需要某個 refresh 才生效 → 待確認（見必答 4）。

**拿玩家當前 cell**（兩條路，皆 header 證實）：
1. `RE::PlayerCharacter::GetSingleton()->GetParentCell()`（`RE/P/PlayerCharacter.h:404`，inline 直接回 `parentCell` 成員 `:494`）。
2. `RE::TES::GetSingleton()`（`RE/T/TES.h:72`）→ 成員 `interiorCell`（`TES.h:231`，`TESObjectCELL* interiorCell`）。玩家在室內時此即當前室內 cell；在外景時為 null。

→ 推薦 `PlayerCharacter::GetParentCell()` + `IsInteriorCell()` 判定，再 `GetLighting()`。

**lighting template 指標**：`RUNTIME_DATA::lightingTemplate`（`TESObjectCELL.h:242`，`BGSLightingTemplate* lightingTemplate; // LTMP`）。若 cell ambient 繼承自 LGTM，要改的是這裡指向的物件（見必答 3）。

---

## 必答 3：BGSDirectionalAmbientLightingColors（`RE/B/BGSDirectionalAmbientLightingColors.h`）

結構（`sizeof == 0x20`，`:37`）：
```cpp
class BGSDirectionalAmbientLightingColors {
    struct Directional {
        template<class T> struct MaxMin { T max; T min; };   // :17-19
        MaxMin<Color> x;  // 00   :24
        MaxMin<Color> y;  // 08   :25
        MaxMin<Color> z;  // 10   :26
    };
    Directional directional;   // 00   :31  ← 六方向 ambient：±X/±Y/±Z 各 max/min
    Color       specular;      // 18   :32  ← 鏡面
    float       fresnelPower;  // 1C   :33  ← fresnel
};
```
- **六方向 ambient**：`directional.x/y/z` 各有 `max`(+方向) 與 `min`(−方向)，共 6 個 `Color`（`:24-26`）。這就是 DALC（directional ambient lighting colors），CK 裡 lighting template 那六格顏色。
- 同時有 **specular**（`:32`）與 **fresnelPower**（`:33`）。

**這是 LGTM 用的嗎？** 是。`RE/B/BGSLightingTemplate.h:34-35` 證實 lighting template 結構為：
```cpp
INTERIOR_DATA                       data;                             // 20 - DATA  :34
BGSDirectionalAmbientLightingColors directionalAmbientLightingColors; // 80 - DALC  :35
```
而 `INTERIOR_DATA` 本身也內嵌一份 DALC（必答 1，`InteriorData.h:37`）。所以 DALC 兩處都有：cell 自身一份、LGTM 一份；採用哪份由必答 1 的 inherit flag 決定。

**runtime 改它的可能性**：欄位全 public、皆 `RE::Color`（8-bit），可直接寫。技術上 100% 可寫。難點同樣是「寫了會不會生效 / 是否被 LGTM 覆寫」——待確認。要整體提亮「方向性 ambient」就把這 6 個 Color 一起往上加。

---

## 必答 4：改了會不會即時生效（最關鍵、也最不確定）

**header 能證明的**：資料結構與「全域已套用的 ambient 鏡像」確實存在；引擎在 cell attach / sky 切換時會把 cell/LGTM 的光照灌進渲染端。

**證據鏈（全域端確實鏡像了 ambient）**：

1. **`RE::Sky`（`RE/S/Sky.h`，singleton `:69`）持有一份已套用的方向性 ambient**：
   - `NiColor directionalAmbientColors[3][2];`（`Sky.h:133`）—— 對應 DALC 的 3 軸 × (max/min)，但這裡是 **`NiColor`（float）**，是「渲染採用值」。
   - `NiColor ambientSpecularTint;`（`:134`）、`float ambientSpecularFresnel;`（`:135`）。
   - `BGSLightingTemplate* extLightingOverride;`（`:87`）—— 外景光照覆寫掛點。
   - `ObjectRefHandle currentRoom / previousRoom`（`:88-89`）+ `lightingTransition`（`:90`）—— 證明 sky 會在房間切換時做光照過渡，亦即**有「重新套用光照」的內部時機**。
   - 取法：`RE::Sky::GetSingleton()`（`Sky.h:69`），或 `TES::GetSingleton()->sky`（`TES.h` 成員 `sky // 100`）。

2. **`BSShaderManager::State`（`RE/B/BSShaderManager.h`，singleton `:30`）持有 shader 端的當前 ambient transform**：
   - `NiTransform directionalAmbientTransform;`（`BSShaderManager.h:64`）—— shader 實際吃的方向性 ambient。
   - `bool interior;`（`:42`）、`ShadowSceneNode* shadowSceneNode[4];`（`:37`，見必答 5）。

→ **推論**：直接寫 `INTERIOR_DATA::ambient` 或 cell DALC，很可能**不會即時反映**，因為渲染端吃的是 `Sky::directionalAmbientColors`（float 鏡像）與 `BSShaderManager::State::directionalAmbientTransform`。要即時，理論上有三條路：
   - (a) 同時改 `Sky::directionalAmbientColors[3][2]`（float）讓渲染端立刻換值——但這值每幀/每次光照更新可能被引擎用 cell/LGTM 重算覆寫回去，需 hook 或持續寫；
   - (b) 觸發一次「房間/光照重套用」：例如重新 attach cell、或呼叫某個 refresh 函式。**header 內找不到公開的 `RefreshLighting()` / `ApplyLighting()`**（`grep` CELL/Sky 皆無）→ 標 **待確認**，可能要靠逆向找位址。
   - (c) 走 `SetFogColor` 等已知 setter 同款的引擎呼叫——但 ambient 沒有對應 setter（必答 2）。

**誠實結論**：**header 看得到全部資料結構，但「改 InteriorData ambient 即時生效」的路徑 header 無法保證。** 最可能需要：① 連 `Sky` float 鏡像一起寫、或 ② hook cell-load（`TESCellAttachDetachEvent`，見 `COMMONLIBSSE_INDEX.md` 事件清單）在每次進室內時重寫、或 ③ 逆向找 refresh 函式。這三者都需要 in-game spike 才能定。

---

## 必答 5：退路 — 掛一盞真實光源（point light）

這是**繞過「改資料能不能生效」整個不確定性**的方案：不動 cell 資料，直接往場景塞一盞無陰影大半徑點光源。

**所需 API（全 header 證實）**：

1. **造光源**：`RE::NiPointLight::Create()`（`RE/N/NiPointLight.h:35-43`，內部 malloc+Ctor）。
2. **設顏色/半徑**：
   - 顏色：`NiPointLight` 繼承 `NiLight`，用 `GetLightRuntimeData()`（`RE/N/NiLight.h:37`）拿到 `diffuse`/`ambient`（`NiColor`，float，`NiLight.h:19-20`）與 `fade`（`:22`）。
   - 半徑：`NiPointLight::SetLightAttenuation(float a_radius)`（`NiPointLight.h:45-50`，已封裝 `RELOCATION_ID(17224, 17626)`）。或直接寫 `radius`（`NiLight.h:21`）/三個 attenuation 係數（`NiPointLight.h:19-21`）。
3. **註冊進渲染（關鍵一步）**：`RE::ShadowSceneNode::AddLight(NiLight*, const LIGHT_CREATE_PARAMS&)`（`RE/S/ShadowSceneNode.h:127`）。
   - `LIGHT_CREATE_PARAMS`（`ShadowSceneNode.h:24-41`）：`dynamic`(:28)、`shadowLight`(:29，設 false = 無陰影、最便宜)、`affectLand`(:31)、`affectWater`(:32)、`neverFades`(:33)、`fov`(:34)、`falloff`(:35) 等。
   - 回傳 `BSLight*`（`ShadowSceneNode.h:127`，`RE/B/BSLight.h:21`），日後用 `RemoveLight(...)`（`ShadowSceneNode.h:132-133`）移除。
4. **拿到 ShadowSceneNode**：`BSShaderManager::State::GetSingleton().shadowSceneNode[N]`（`BSShaderManager.h:30,37`）。索引一般用主場景圖（`sceneGraphIndex`，見 `LIGHT_CREATE_PARAMS::sceneGraphIndex` `:38`；State 另有 `sceneGraph` 旗標 `:62`）—— 確切索引值 **待確認**（常見社群作法取 `[0]`）。
5. **定位光源**：把 light 掛到玩家/相機節點，或每幀更新 `NiPointLight` 的 `local.translate`。玩家 3D 用 `Actor::Get3D()`（見 `COMMONLIBSSE_INDEX.md` §3.2 / `util.h::NifUtil::Armature::GetActorNode`）。`BSLight` 自帶 `worldTranslate`（`BSLight.h:52`）。

**可行性評估**：API 完整、precedent 充足（社群「torch / player light」mod 即此法）。**風險低於改資料**，因為光源是 additive、不依賴引擎重算 cell ambient，掛上即亮、移除即還原，對存檔零污染。代價：是「打一盞燈」而非「均勻提升環境光」，遠角落仍偏暗，且需自管生命週期（cell 切換、存讀檔時 add/remove）。

**雷區（沿用 `PITFALLS.md` 精神）**：
- `Create()` 回的是裸指標，但場景圖節點普遍用 `NiPointer`（`PITFALLS.md:28-31`）；`AddLight` 內部會接管，注意所有權。
- 所有渲染/場景圖操作務必在主執行緒：用 `SKSE::GetTaskInterface()->AddTask`（`COMMONLIBSSE_INDEX.md` §5、§3.7）。
- 跨 SE/AE 版本：`SetLightAttenuation` 已給雙 ID（`NiPointLight.h:48`），自己若再 hook 別的函式需自備 Address Library ID。

---

## 必答 6：既有 util / precedent（repo 內）

- `src/util.h`：**無**現成 lighting / ambient / ShadowSceneNode helper（grep 過）。但有可複用基礎件：
  - `NifUtil::Armature::GetActorNode(actor, nodeName)`（`util.h:529`）/ `AttachToNode`（`util.h:543`）—— 掛 point light 到玩家骨節時可直接用。
  - `NifUtil::Node::AttachToNode`（`util.h:504`）、`GetNiObject`（`util.h:497`）。
  - `MathUtil` 角度/向量工具（`util.h:200+`）—— 算光源相對玩家位置時可用。
- `MODDING_COOKBOOK.md`：核心模式是「純 C++ 動態法術、不靠 ESP、依 `fullName` 前綴 `"C++: "` 分派」（`MODDING_COOKBOOK.md` Part 1.1）。→ 本功能應做成一個 `"C++: Brighten Dungeon"` lesser power，照 `NpcGenerator.cpp::InitializeMagic()` 的骨架加一個分派分支即可，與既有架構一致。**無 lighting/ambient 既有食譜**（grep 無命中），屬全新功能。
- `PITFALLS.md`：相關可複用雷區——主選單 CTD 防禦性 null check（§3）、`NiPointer` 不可塞 `auto*`（§5）、初始化時序用 lazy loading（§1）。

---

## 結論：go / no-go + 推薦方案

**Go（可行）**，但**推薦以「掛 NiPointLight」為主、「改 InteriorData ambient」為輔/實驗**。

理由：
1. **改 InteriorData/DALC**：資料結構完整可寫（必答 1/3），但**生效路徑是最大不確定性**（必答 4）——渲染端吃的是 `Sky` float 鏡像與 `BSShaderManager::State`，直接寫 cell 資料很可能不即時、且可能被引擎重算覆寫、還要處理 LGTM inherit flag。風險高、需逆向。
2. **掛 NiPointLight**：API 鏈完整且全是公開封裝（`Create` → `SetLightAttenuation` → `ShadowSceneNode::AddLight`，必答 5），additive、可逆、對存檔零污染，社群有成熟 precedent。**風險最低、最快出可見效果。**

**推薦落地形式**：做成一個 lesser power（`"C++: Brighten Dungeon"`，照 `MODDING_COOKBOOK.md` 模式），施放時若 `IsInteriorCell()` 則往 `ShadowSceneNode` 加一盞大半徑、`shadowLight=false`、`neverFades=true` 的點光源並掛在玩家身上；再放一次移除。所有操作包進 `GetTaskInterface()->AddTask`。

若日後要「真正的均勻環境光提升」（非單點），再把改 `INTERIOR_DATA::ambient` + 同步 `Sky::directionalAmbientColors` 當第二階段實驗，配合 in-game spike 驗證生效路徑。

---

## 最大未知數（須 in-game spike 才能定）

1. **改 `INTERIOR_DATA::ambient` 是否即時生效**，還是必須同寫 `Sky::directionalAmbientColors[3][2]`（`Sky.h:133`）/ `BSShaderManager::State::directionalAmbientTransform`（`BSShaderManager.h:64`），或觸發一次光照重套用。（必答 4）
2. **是否有公開的「refresh lighting」函式**：header 找不到（CELL/Sky 皆無 `Refresh/Apply Lighting`）；若需要，得逆向找位址。
3. **LGTM inherit flag 的實際行為**：當 cell `lightingTemplateInheritanceFlags` 含 `kAmbientColor`（`InteriorData.h:13,42`）時，寫 cell 自身 `ambient` 是否被 LGTM 覆蓋；需不需要改 `RUNTIME_DATA::lightingTemplate`（`TESObjectCELL.h:242`）那份。
4. **`ShadowSceneNode` 的正確索引**：`shadowSceneNode[4]`（`BSShaderManager.h:37`）哪一格是當前主場景（與 `LIGHT_CREATE_PARAMS::sceneGraphIndex` `ShadowSceneNode.h:38` / State `sceneGraph` `:62` 的對應）。
5. **point light 的生命週期/還原時機**：cell 切換（`TESCellAttachDetachEvent`）、存讀檔時的 add/remove 與 NiPointer 所有權細節。
6. **`Color`(8-bit) ↔ `NiColor`(float) 轉換語意**：cell 資料是 8-bit `Color`，渲染鏡像是 float `NiColor`，提亮幅度的映射需實測。

---

## 後記：in-game spike 結果（2026-06-06，已實作於 `src/AmbientBoost.cpp`）

實測**推翻了「必須重進 cell」的社群常識**：

- **每幀寫 `Sky::directionalAmbientColors`（必答 4 路 a）→ 無效**（被引擎後續光照 pass 蓋掉，且室內未必走此值）。已棄。
- **直接寫 `cell->GetLighting()` 的 `INTERIOR_DATA`：`ambient` + `directional` + 6 個 DALC `Color`，並 `lightingTemplateInheritanceFlags.reset(kAmbientColor, kDirectionalColor)`（清繼承旗標）→ 本機即時生效，不需重進 cell。** 推測：清掉繼承旗標後，引擎當幀的光照更新直接採用 cell 自身值（不再走 LGTM 重算那條需要 cell-load 的路）。
- 採「取 max 抬下限 + 暖色調」提亮：暗處升、亮處留、黑裝因 albedo 低仍黑。存原值、Off 還原。
- 落地形式為熱鍵循環 5 階（非 lesser power），與 `FollowLight`（點光）並用。生效路徑結論：**改 INTERIOR_DATA + 清繼承旗標**即可，毋須 hook refresh、毋須改 Sky/BSShaderManager 鏡像。
