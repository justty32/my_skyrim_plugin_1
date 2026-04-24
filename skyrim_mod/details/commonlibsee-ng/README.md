# Skyrim Modding - CommonLibSSE NG 學習筆記

本目錄包含了關於 `CommonLibSSE NG` 庫的架構分析、核心 RE（逆向工程）層類別說明以及插件開發示例。

## 1. 基礎架構 (Architecture)

- [00_概覽 (Overview)](00_overview.md) - CommonLibSSE NG 的核心目標與特性。
- [01_結構 (Structure)](01_structure.md) - 目錄佈局與關鍵文件說明。
- [NPC 行為解析](../commonlib-se/NPC_Behavior.md) - 深入探討 AI 進程、行為包與戰鬥系統。
- [NPC 生命週期與類別](../commonlib-se/NPC_Lifecycle_and_Types.md) - 詳細解析有名 NPC 與路人 NPC 的區別、死亡清理與 AI 權重計算。
- [NPC 存儲與序列化](../commonlib-se/NPC_Storage_and_Serialization.md) - 詳細解析從 TESNPC 藍圖到 Actor 實體以及存檔中的二進制表現。
- [NPC 反應機制解析](../commonlib-se/NPC_Reaction_Mechanics.md) - 深入探討觀察檢測、故事管理器分發與自動語音輸出邏輯。
- [Radiant AI 架構解析](../commonlib-se/Radiant_AI_Architecture.md) - 深入了解動態任務生成與 NPC 自主日常行為系統。
- [存檔系統與數據存儲](../commonlib-se/Save_System_Architecture.md) - 解析 ESS 結構、ChangeFlags 以及 SKSE 序列化機制。
- [物品定義與存儲架構](../commonlib-se/Item_Definition_and_Storage.md) - 詳細解析武器、藥水在 ESP 與存檔中的「原型+實例」存儲機制。
- [ESS 存檔深度解析](../commonlib-se/ESS_Save_Deep_Dive.md) - 詳細拆解 ChangeForms 邏輯、Papyrus 數據與存檔安全。
- [容器與物品欄架構](../commonlib-se/Container_Architecture.md) - 深入解構 TESObjectCONT、InventoryChanges 以及物品移動邏輯。
- [Actor 解剖學](../commonlib-se/Actor_Anatomy.md) - 了解角色的種族、外型、數值與物品組成。
- [Actor 動作與動畫架構](../commonlib-se/Actor_Animation_Architecture.md) - 解析 Havok Behavior 狀態機、動畫事件與圖變量操控機制。
- [Havok Behavior 深度解析](../commonlib-se/Havok_Behavior_Deep_Dive.md) - 深入探討 .hkx 行為圖的邏輯構造、運作流程與資源管理機制。
- [物品解剖學](../commonlib-se/Item_Anatomy.md) - 了解物件的模型、屬性、效果與事件機制。
- [物品文本與介紹](../commonlib-se/Item_Text_and_Description.md) - 解析名稱、描述、書籍內容與多語言系統。
- [世界交互與觸發](../commonlib-se/World_Interactions_and_Triggers.md) - 深入了解區域觸發、陷阱、門與鎖系統。
- [世界運行機制解析](../commonlib-se/World_Runtime.md) - 了解主循環、離線 AI 與事件觸發原理。
- [對話與任務定義架構](../commonlib-se/Dialogue_Quest_Storage_Architecture.md) - 深入解構 QUST, DIAL, INFO 記錄與任務生命週期存儲。
- [對話話題運作機制](../commonlib-se/Dialogue_Topic_Mechanism.md) - 詳細解析 DIAL 與 INFO 的一對多關聯、條件篩選與運行時對話棧邏輯。
- [任務與對話交互解析](../commonlib-se/Quest_Dialogue_Interactions.md) - 了解任務生命週期、對話觸發動作與獎勵機制。
- [Papyrus 腳本綁定架構](../commonlib-se/Papyrus_Binding_Architecture.md) - 深入了解 Native 函數註冊、類型映射與 C++ 與腳本的交互機制。
- [室內與室外架構對比](../commonlib-se/Exteriors_vs_Interiors.md) - 深度解析 Exterior 與 Interior 在數據、照明與加載機制上的本質區別。
- [投射物與效果架構](../commonlib-se/Projectiles_and_Effects.md) - 詳細講述火球、噴火術等物理實體的工作原理與碰撞機制。
- [模型與物理構造解析](../commonlib-se/Model_and_Physics_Anatomy.md) - 深入了解 NIF 模型幾何、材質渲染、特效掛載與 Havok 物理性質。
- [Actor 模型與布娃娃架構](../commonlib-se/Actor_Model_and_Ragdoll_Anatomy.md) - 解析骨骼節點、Biped 槽位分組、種族視覺影響與 Havok 布娃娃物理系統。
- [骨骼系統深度解析](../commonlib-se/Actor_Skeleton_Deep_Dive.md) - 深入了解骨骼節點層級、父子繼承與頂點權重機制。
- [蒙皮與槽位深度解析](../commonlib-se/Biped_Slots_and_Skinning_Deep_Dive.md) - 深入了解 Biped 槽位、換裝邏輯與動態蒙皮原理。
- [種族視覺深度解析](../commonlib-se/Race_Visual_System_Deep_Dive.md) - 深入了解 Race 模板、皮膚材質覆蓋與非人種族視覺構造。
- [3D 場景圖基礎](../commonlib-se/3D_Scenegraph_Fundamentals.md) - 以「倒掛樹」邏輯解析 NiNode 節點、父子繼承與 Update3D 機制。
- [材質與渲染架構](../commonlib-se/Shader_and_Texture_Fundamentals.md) - 深入了解 BSLightingShaderProperty、DDS 貼圖三位一體與動態材質覆蓋原理。
- [NIF 檔案格式深度解析](../commonlib-se/NIF_File_Format_Deep_Dive.md) - 詳細拆解 NIF 的二進制 Block 結構、節點鏈接與解析流程。
- [NIF 加載與資源管理](../commonlib-se/NIF_Loading_and_Resource_Management.md) - 深入探討模型異步加載時機、引用計數緩存與 QueuedFile 系統。
- [碰撞系統架構](../commonlib-se/Collision_System_Architecture.md) - 深入了解 Havok 物理引擎、碰撞層、剛體屬性與射線偵測機制。
- [貼圖「三位一體」深度解析](../commonlib-se/Texture_Trio_Anatomy.md) - 解析 Diffuse, Normal, Specular 的協作關係、DDS 格式與動態路徑修改。
- [BSA 格式與運作流程](../commonlib-se/BSA_Format_and_Workflow.md) - 詳細解析哈希索引、虛擬檔案系統掛載與資源覆蓋機制。
- [法術系統架構](../commonlib-se/Spell_System_Anatomy.md) - 解析施法屬性讀取、動態數值渲染與魔法效果作用機制。
- [法術定義與資源架構](../commonlib-se/Spell_Definition_and_Resources.md) - 詳細解析 SPEL, MGEF 記錄與魔法視覺資源（NIF/DDS）的關聯。
- [天賦系統架構](../commonlib-se/Perk_System_Architecture.md) - 詳細解析 BGSPerk 定義、星座 UI 運作與動態修改機制。
- [特效系統架構](../commonlib-se/VFX_Architecture.md) - 解析粒子系統、視覺效果定義與動態觸發流程。
- [資源檔案系統架構](../commonlib-se/Resource_Formats_Architecture.md) - 詳細解析 NIF, DDS, HKX 與 BSA 的底層結構與讀取機制。
- [投射物碰撞與效果執行流](../commonlib-se/Projectile_Impact_and_Effect_Flow.md) - 深入解構箭矢與火球從物理撞擊到魔法負載施加的完整邏輯鏈。

## 2. 逆向工程層 (RE Layer)

位於 `RE/` 目錄下，深入分析遊戲引擎內部類。

- [00_RE 層概覽](RE/00_overview.md) - RTTI、VTable 與地址重定位的工作原理。
- [TESForm](RE/TESForm.md) - 所有表單的基礎類。
- [TESObjectREFR](RE/TESObjectREFR.md) - 世界中的對象引用。
- [Actor](RE/Actor.md) - 角色、NPC 與生物。
- [PlayerCharacter](RE/PlayerCharacter.md) - 玩家角色單例。

## 3. 開發示例 (Examples)

基於實際項目的分析，展示常用的開發技巧。

- [00_HelloWorld](examples/00_helloworld.md) - 最簡單的插件結構。
- [01_Hooking 與 Thunks](examples/01_hooking_thunks.md) - 如何攔截與修改引擎函數。
- [02_事件監聽與 Papyrus 交互](examples/02_events_papyrus.md) - 監聽玩家行為與調用遊戲腳本。

## 推薦學習路徑

1. 先閱讀 **概覽** 和 **結構**。
2. 了解 **RE 層** 的基本類別（特別是 `TESForm` 和 `Actor`）。
3. 通過 **HelloWorld** 示例開始動手實踐。
4. 進階學習 **Hooking** 和 **事件監聽** 技術。
