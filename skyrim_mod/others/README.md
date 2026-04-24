# Skyrim 引擎功能總覽 (RE 層)

`CommonLibSSE-NG` 的 `RE` 命名空間映射了 Skyrim 引擎的內部結構。由於類別極多，本系列文檔將按遊戲子系統為您整理最常用、最具實戰價值的 API。

## 目錄

1. [事件系統 (Events)](01_Events.md) - 如何監聽遊戲內發生的事情（如攻擊、裝備、死亡）。
2. [UI 與選單系統 (UI & Menus)](02_UI_and_Menus.md) - 如何與遊戲介面（Scaleform/Flash）交互。
3. [魔法與法術系統 (Magic & Spells)](03_Magic_and_Spells.md) - 施法、魔法效果與附魔。
4. [物品欄與裝備 (Inventory & Items)](04_Inventory_and_Items.md) - 獲取玩家物品、修改裝備。
5. [Papyrus 腳本交互 (Papyrus Scripting)](05_Papyrus_Scripting.md) - C++ 與 Papyrus 腳本的雙向通訊。
6. [輸入系統 (Input System)](06_Input_System.md) - 監聽鍵盤、滑鼠與手柄操作。

> **提示**: 在實際開發中，你通常需要包含 `#include <RE/Skyrim.h>` 來獲取這些類別的定義。
