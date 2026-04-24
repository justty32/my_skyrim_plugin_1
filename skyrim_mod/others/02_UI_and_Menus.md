# 02. UI 與選單系統 (UI & Menus)

Skyrim 的 UI 是基於 Scaleform（Flash ActionScript 2/3）構建的。你可以通過 C++ 檢查 UI 狀態、發送消息給 UI，甚至調用 UI 內部的函數。

## 核心類別

- **`RE::UI`**: 
  - **路徑**: `include/RE/U/UI.h`
  - 管理所有選單的單例。可以用來檢查某個選單是否開啟。
- **`RE::UIMessageQueue`**: 
  - **路徑**: `include/RE/U/UIMessageQueue.h`
  - 用於向 UI 系統發送異步消息（如打開、關閉選單）。
- **`RE::IMenu`**: 
  - **路徑**: `include/RE/I/IMenu.h`
  - 所有選單的基類。
- **`RE::ConsoleLog`**: 
  - **路徑**: `include/RE/C/ConsoleLog.h`
  - 用於向遊戲內的 `~` 控制台輸出文本。

## 常用功能與範例

### 1. 在控制台輸出訊息
調試時最常用的功能。
```cpp
RE::ConsoleLog::GetSingleton()->Print("這是一條來自 C++ 插件的訊息！");
```

### 2. 檢查選單狀態
例如，檢查玩家是否打開了物品欄或暫停選單。這在處理輸入或更新邏輯時很重要，防止在暫停時執行代碼。

```cpp
auto ui = RE::UI::GetSingleton();

// 檢查暫停選單是否打開
if (ui->IsMenuOpen(RE::JournalMenu::MENU_NAME)) { // 路徑: include/RE/J/JournalMenu.h
    // 遊戲已暫停
}

// 檢查是否沒有任何選單打開（即玩家處於正常遊玩狀態）
if (!ui->GameIsPaused()) {
    // 執行遊戲邏輯
}
```

### 3. 強制關閉或打開選單
使用 `UIMessageQueue` 發送指令。

```cpp
auto msgQueue = RE::UIMessageQueue::GetSingleton();

// 強制關閉對話選單
msgQueue->AddMessage(RE::DialogueMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr); // 路徑: include/RE/D/DialogueMenu.h

// 打開地圖選單
msgQueue->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr); // 路徑: include/RE/M/MapMenu.h
```

### 4. 顯示屏幕通知 (Notification)
- **路徑**: `include/RE/M/Misc.h` (函數: `RE::DebugNotification`)
在屏幕左上角顯示一條提示信息（類似於“已添加 100 金幣”）。

```cpp
RE::DebugNotification("這是一條屏幕通知！");
```
