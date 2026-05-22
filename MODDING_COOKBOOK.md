# SKSE Mod 實作食譜（給 AI 看的）

這份文件是寫給「之後幫忙做 mod 的 AI」看的。目的：讓 AI 在不踩雷的前提下，照著本專案**已經驗證可運作**的模式快速產出功能。
搭配閱讀：`COMMONLIBSSE_INDEX.md`（有哪些 class/方法）、`CLAUDE.md`（專案規則）、`PITFALLS.md`（編譯/連結雷區）、`FOR_AGENT.md`（編譯/打包/驗證流程）。

> **權威範例**：`src/NpcGenerator.cpp` 是本專案的標準範本。要新增功能時，**先讀它**，照同樣的結構寫，不要自創一套。

---

# Part 1 — AI 携手指南（核心概念與必避雷區）

## 1.1 本專案的核心模式：純 C++ 動態法術，不靠 ESP

本專案**不用 Creation Kit、不做 ESP/ESM**。所有「玩家可施放的功能」都是在執行期用程式碼動態生出來的法術（lesser power）。整套流程在 `NpcGenerator.cpp::InitializeMagic()`，分四步：

1. **動態造一個 base 魔法效果（MGEF）**：用 `IFormFactory` 造 `EffectSetting`，archetype 設 `kScript`、`kHideInUI`、`kFireAndForget` + `kSelf`。它本身不做事，只是讓法術合法。
2. **動態造法術（SpellItem）**：同樣用 `IFormFactory` 造 `SpellItem`，型別設 `kLesserPower`（這樣玩家在「能力」選單就能裝來用、不耗魔），掛上一個指向 base MGEF 的 `Effect`。
3. **命名作為分派依據**：每個法術的 `fullName` 用統一前綴（本專案是 `"C++: "`）。例如 `"C++: Spawn NPC"`。
4. **訂閱施法事件，依名稱分派**：用 `BSTEventSink<TESSpellCastEvent>` 監聽，誰施放了哪個 `"C++: ..."` 法術，就呼叫對應的 C++ 函式。

**為什麼這樣設計**：免去 ESP 依賴、免去硬編 FormID、玩家裝好 .dll 即可用。新增一個功能 = 多造一個法術 + 在分派處多一個 `else if`。

```cpp
// 取自 NpcGenerator.cpp，新增功能就照這個骨架擴充
auto CreateSpell = [&](RE::SpellItem*& a_ptr, const char* a_name) {
    a_ptr = spellFactory->Create()->As<RE::SpellItem>();
    if (a_ptr) {
        a_ptr->fullName = a_name;
        a_ptr->data.spellType   = RE::MagicSystem::SpellType::kLesserPower;
        a_ptr->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
        a_ptr->data.delivery    = RE::MagicSystem::Delivery::kSelf;
        auto* effect = new RE::Effect();
        effect->baseEffect = baseEffect;          // 上面動態造的 base MGEF
        effect->effectItem.magnitude = 0.0f;
        effect->effectItem.duration  = 0;
        a_ptr->effects.push_back(effect);
    }
};
```

## 1.2 生命週期：什麼時機做什麼事（`plugin.cpp`）

`MessageHandler` 是所有初始化的入口。**時機選錯是最常見的失敗原因**（form 還沒載入就去查 → 拿到 nullptr）。

| SKSE 訊息 | 時機 | 在這裡做什麼 |
|-----------|------|--------------|
| `kPostLoad` | 所有 SKSE plugin 已載入 | 跟其他 plugin 溝通、註冊 API |
| `kDataLoaded` | **遊戲資料已載入、主選單就緒** | **造 form、查 form、註冊事件 sink**（本專案 `InitializeMagic()` 在這） |
| `kNewGame` | 開新遊戲 | 給玩家法術/物品 |
| `kPreLoadGame` | 開始讀存檔 | 清狀態 |
| `kPostLoadGame` | 存檔讀取完成 | 重新給玩家法術、還原狀態 |

**鐵則**：任何 `LookupByID` / `IFormFactory` / `AddEventSink` 都要等到 `kDataLoaded` 之後。動態造的法術不會存進存檔，所以 `kNewGame` 和 `kPostLoadGame` 都要再 `AddSpell` 給玩家一次（本專案 `GiveSpellsToPlayer()`）。

## 1.3 必避雷區（按踩雷頻率排序）

1. **主執行緒**：絕大多數遊戲操作（生成、移動、改 3D、開選單、加物品）**必須在主執行緒**。事件 sink 的 `ProcessEvent` 本來就在主執行緒，可直接做。但若你在背景（自己的 thread、計時器、hook 的非主執行緒路徑）就**必須**包進：
   ```cpp
   SKSE::GetTaskInterface()->AddTask([=]() { /* 動遊戲狀態的程式碼 */ });
   ```
2. **每個指標都要檢查 null**：`GetSingleton()`、`LookupByID`、`As<T>()`、`crosshair->target[0].get()` 全都可能回 nullptr。本專案習慣 early-return + 寫 log。
3. **Debug build 要狂寫 log**：在 Proton 下難 attach debugger，**唯一的除錯手段是 log**。每個關鍵步驟、每次 form 建立成功與否、每個 null 檢查失敗都要 `SKSE::log::info/error`。（見 memory：Debug 要 aggressive log，Release 可安靜。）
4. **form 查找方式的取捨**（見 1.4）：硬編 FormID 在動態環境會失準，本專案多用 `LookupByEditorID` 或 `TESDataHandler::LookupForm`。
5. **新檔要註冊**：新 `.cpp` 加進 `cmake/sourcelist.cmake`、新 `.h` 加進 `cmake/headerlist.cmake`。CMake **不 glob**，漏了就不會編譯。
6. **不要手寫 `SKSEPluginInfo(...)`**：由 `CMakeLists.txt` 的 `add_commonlibsse_plugin` 自動產生，手寫會 LNK2005 重複符號（見 `PITFALLS.md`）。
7. **跨遊戲版本**：呼叫遊戲內部函式要用 `RELOCATION_ID(seID, aeID)`，不要寫死位址。

## 1.4 三種 form 查找方式，怎麼選

| 方式 | 用法 | 何時用 |
|------|------|--------|
| `LookupByID<T>(0x00000007)` | 絕對 FormID | 查 **Skyrim.esm 的 base 遊戲 form**（load order 固定在 0x00 開頭），如玩家 NPC `0x7` |
| `LookupByEditorID<T>("TreeFloraJuniper01")` | EditorID 字串 | 比硬編 FormID 可靠，本專案生成樹/石頭時優先用 |
| `TESDataHandler::LookupForm(0x123, "MyMod.esp")` | 相對 ID + 檔名 | 查 **某個 mod 內**的 form，跨 load order 安全（`FormUtil::Parse` 已封裝） |

---

# Part 2 — 實作食譜（Recipes）

每則食譜假設你在事件 sink 的 `ProcessEvent`（即主執行緒）裡，或在被它呼叫的函式裡。`anchor` 通常是施法者（`a_event->object.get()`）。

## R1. 新增一個「C++ 法術」功能（最常見）
在 `NpcGenerator.cpp`（或新模組）做三件事：
```cpp
// (1) 宣告指標
RE::SpellItem* g_myNewSpell = nullptr;
// (2) InitializeMagic() 裡造它
CreateSpell(g_myNewSpell, "C++: My New Thing");
// (3) SpellCastHandler::ProcessEvent 的分派加一條
else if (spellName == "C++: My New Thing") { DoMyNewThing(anchor); }
// (別忘了) GiveSpellsToPlayer() 裡 AddSpell(g_myNewSpell);
```

## R2. 抓玩家準星指向的目標
```cpp
RE::TESObjectREFR* target = RE::CrosshairPickData::GetSingleton()->target[0].get().get();
if (!target) { SKSE::log::warn("沒有準星目標"); return; }
auto* actorTarget = target->As<RE::Actor>();   // 想要 actor 就這樣轉，可能是 nullptr
```

## R3. 在玩家面前生成一個物件（base form → 世界實例）
```cpp
auto* base = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("TreeFloraJuniper01");
if (!base) return;
auto spawned = anchor->PlaceObjectAtMe(base, false);   // 最可靠的生成法
if (spawned) {
    float z = anchor->data.angle.z;
    RE::NiPoint3 pos = anchor->GetPosition();
    pos.x += std::sin(z) * 200.0f;   // 往面朝方向推 200 單位
    pos.y += std::cos(z) * 200.0f;
    pos.z += 10.0f;
    spawned->SetPosition(pos);
    spawned->SetAngle(anchor->data.angle);
}
```
要更精準的座標控制改用 `TESDataHandler::CreateReferenceAtLocation(base, pos, angle, cell, worldspace, ...)`。

## R4. 動態複製並客製一個 NPC
```cpp
auto* templateNPC = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);          // 玩家 NPC 當模板
auto* factory     = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
auto* newBase     = factory->Create()->As<RE::TESNPC>();
newBase->Copy(templateNPC);
newBase->fullName = "Generated Citizen";
auto spawned = anchor->PlaceObjectAtMe(newBase, false);   // spawned 是 Actor
```

## R5. 改 actor 屬性 / 外觀
```cpp
auto* actor = target->As<RE::Actor>();
actor->ModActorValue(RE::ActorValue::kHealth, 50.0f);          // 加 50 血上限
actor->SetActorValue(RE::ActorValue::kStamina, 200.0f);
// 改外觀（透過 base）：
auto* base = actor->GetActorBase();
base->weight = 100.0f;
actor->DoReset3D(true);                                        // 改完外觀要 reset 3D 才會生效
```

## R6. 給玩家法術 / 物品 / 裝備
```cpp
auto* player = RE::PlayerCharacter::GetSingleton();
player->AddSpell(g_myNewSpell);                                 // 法術
auto* gold = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0000000F);
player->AddObjectToContainer(gold, nullptr, 100, nullptr);      // 100 金幣
auto* sword = RE::TESForm::LookupByEditorID<RE::TESObjectWEAP>("IronSword");
player->AddObjectToContainer(sword, nullptr, 1, nullptr);
RE::ActorEquipManager::GetSingleton()->EquipObject(player, sword);  // 直接裝上
```

## R7. 訂閱一個遊戲事件（hook 行為）
照 `SpellCastHandler` 的結構，換掉事件型別即可。可用事件見 `COMMONLIBSSE_INDEX.md` 3.6（50+ 種）。
```cpp
class HitHandler : public RE::BSTEventSink<RE::TESHitEvent> {
public:
    static HitHandler* GetSingleton() { static HitHandler s; return &s; }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* e,
        RE::BSTEventSource<RE::TESHitEvent>*) override {
        if (!e) return RE::BSEventNotifyControl::kContinue;
        // e->target / e->cause / e->source ...
        return RE::BSEventNotifyControl::kContinue;   // 一定要回這個，否則吃掉事件
    }
};
// 在 kDataLoaded 時註冊：
RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESHitEvent>(HitHandler::GetSingleton());
```

## R8. 開 / 關一個遊戲選單
```cpp
auto* q = RE::UIMessageQueue::GetSingleton();
q->AddMessage(RE::RaceSexMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);   // 開
q->AddMessage(RE::RaceSexMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);   // 關
// 查某選單開著沒：
bool open = RE::UI::GetSingleton()->IsMenuOpen(RE::InventoryMenu::MENU_NAME);
```

## R9. 把工作丟回主執行緒（在背景時必用）
```cpp
SKSE::GetTaskInterface()->AddTask([=]() {
    // 這裡才安全地動遊戲狀態
    player->AddSpell(g_myNewSpell);
});
```

## R10. 印 debug 訊息
```cpp
SKSE::log::info("生成成功: {:X}", spawned->GetFormID());     // 寫進 log 檔
RE::ConsoleLog::GetSingleton()->Print("Hello from C++");      // 印到遊戲 ~ 主控台
RE::DebugNotification("螢幕左上角的小提示");                    // 螢幕通知
```

## R11. 善用 `src/util.h` 既有 helper（不要重造輪子）
- 數學/角度：`MathUtil::Angle::GetForwardVector(quat)`、`DegreeToRadian`、`NormalRelativeAngle`、`MathUtil::Clamp`。
- 平滑移動：`MathUtil::Interp::InterpTo`、`ObjectUtil::Transform::InterpAngleTo`。
- form 解析：`FormUtil::Parse::GetFormFromConfigString("0x123~MyMod.esp")`。
- 場景圖/骨骼：`NifUtil::Armature::GetActorNode(actor, "NPC Head [Head]")`、`AttachToNode(...)`、`NifUtil::Node::Clone(...)`。
- 碰撞：`NifUtil::Collision::ToggleMeshCollision(...)`。
- 播放動畫：`AnimUtil::Idle::Play(idle, actor, action, target)`。
- 設定檔列舉：`SystemUtil::File::GetConfigs(folder, suffix, ".ini")`。
新工具優先**擴充這些既有 namespace**，而非另起爐灶（見 `CLAUDE.md`）。

---

## 新增模組的標準步驟（總結）
1. 讀 `NpcGenerator.cpp` 當範本。
2. 新增 `src/MyFeature.{h,cpp}`，照同樣的 namespace + `Initialize*()` + event sink + dispatch 結構。
3. 把兩個檔分別登記到 `cmake/sourcelist.cmake` / `cmake/headerlist.cmake`。
4. 在 `plugin.cpp` 的 `kDataLoaded`（初始化）、`kNewGame`/`kPostLoadGame`（給玩家法術）接上呼叫。
5. 全程寫 log；每個指標檢查 null；背景操作包 `AddTask`。
6. 依 `FOR_AGENT.md` 編譯、打包、看 log 驗證。
