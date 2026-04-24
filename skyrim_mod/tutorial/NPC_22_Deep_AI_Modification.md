# 22. 高級技術：Hook AI 更新循環 (Deep AI Hooking)

本教學展示如何透過 Hook `RE::AIProcess::Update` 函數，實現對 NPC 決策邏輯的深度控制。我們將實現一個功能：**強行讓某個區域內的所有 NPC 停止思考，保持原地發呆。**

## 1. 核心邏輯
1.  找到 `RE::AIProcess::Update` 的函數地址。
2.  安裝一個 `Thunk` 攔截該調用。
3.  在 Thunk 中判斷：如果該 NPC 滿足特定條件，則跳過原始的 `Update` 邏輯。

## 2. 代碼實現

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

struct AIProcess_Update {
    // 1. 定義 Thunk 函數
    // 注意：AIProcess 的成員函數第一個參數是 this 指針
    static void thunk(RE::AIProcess* a_this, RE::Actor* a_actor) {
        // 獲取當前 NPC 名稱
        std::string name = a_actor->GetName();

        // 2. 判斷邏輯：如果是我們想要「凍結」的目標
        // 例如：名字裡帶有 "Guard" 的衛兵
        if (name.find("Guard") != std::string::npos) {
            // 跳過原有的 AI 更新邏輯，NPC 將失去「思考」能力
            // 他們會保持當前的姿勢，不再尋路，不再攻擊
            return; 
        }

        // 3. 否則，執行原始邏輯
        func(a_this, a_actor);
    }

    // 保存原函數地址
    static inline REL::Relocation<decltype(thunk)> func;
};

// 安裝 Hook
void InstallAIHook() {
    // 找到 AIProcess::Update 的 ID (這是示例 ID)
    // 原始碼參考: include/RE/A/AIProcess.h
    const REL::Relocation<std::uintptr_t> aiProcessVTable{ RELOCATION_ID(38456, 39432) }; // PlaceHolder IDs

    // 寫入我們的 Thunk
    // stl::write_vfunc<0x...>(aiProcessVTable, AIProcess_Update::thunk);
    
    SKSE::log::info("深層 AI Hook 已安裝。");
}
```

## 3. 關鍵 API 標註
-   **`RE::AIProcess`**: AI 邏輯的運行實體。`include/RE/A/AIProcess.h`
-   **`REL::Relocation`**: 用於定位遊戲內存中的函數。
-   **`thunk`**: 作為中間人攔截並修改函數執行。

## 4. 進階應用思路
-   **集體戰術**: 在 `Update` 中檢查周圍隊友的血量，如果隊友過低，強迫 NPC 執行掩護動作。
-   **動態感知**: 透過修改 `a_this->high` 內部的感知數據，讓 NPC 「視而不見」或是「聽覺加倍」。
-   **自定義尋路**: 在 `Update` 之前手動注入新的路徑點。

## 5. 警告
-   **性能**: `AIProcess::Update` 是極高頻觸發的函數。在 Thunk 中執行複雜的字符串查找或循環會嚴重拖慢遊戲幀率。
-   **穩定性**: 跳過某些關鍵的 AI 更新可能會導致 NPC 動畫卡死或狀態機異常。請確保你清楚跳過邏輯的後果。
