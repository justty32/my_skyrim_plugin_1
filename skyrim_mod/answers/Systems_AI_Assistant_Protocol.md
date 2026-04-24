# AI 助手回覆協議 (AI Assistant Protocol)

本文件記錄了使用者於 2026年4月8日 指示的回覆規範。

---

## 1. 運作準則
- **文檔化回覆**：後續所有關於 Skyrim Modding、代碼分析、架構說明或教學的回答，將不再直接輸出於對話框，而是寫入 `architectures/classified/` 目錄下的 `.md` 檔案中。
- **分類存放**：
    - `3D_Graphics`: 涉及模型、貼圖、渲染與物理。
    - `Dialogue_Quest`: 涉及任務邏輯、對話系統與 Story Manager。
    - `Items`: 涉及物品、容器與裝備數據。
    - `Magic`: 涉及法術、附魔、投影物與視覺特效。
    - `NPC`: 涉及 AI 行為、動畫、屬性與生命週期。
    - `Save_Data`: 涉及存檔結構與序列化。
    - `Systems`: 涉及引擎底層、編譯環境、基礎類別（如 TESForm）與助手協議。
    - `World`: 涉及開放世界、網格加載、單元管理與環境系統。

## 2. 檔案命名規範
- 格式：`Response_[Topic_Name].md` 或根據內容性質命名。
- 若內容涉及多個分類，助手將選擇最核心的一個，或在相關分類中建立交叉引用。

## 3. 執行狀態
- **當前環境**: Windows PowerShell。
- **目標根目錄**: `C:\code\important_2\skyrim_modding\architectures\classified\`。

---
*本協議由 AI 助手自動生成，以確認指令已正確接收。*
