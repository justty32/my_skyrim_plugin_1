# 進階插件開發 - Hooking 與 Thunks (NPCs-Learn-to-Barter)

`NPCs-Learn-to-Barter` 示例展示了如何攔截（Hook）遊戲引擎的內部函數調用，並替換為自定義邏輯。這是在不修改 `.exe` 文件的情況下改變遊戲行為的核心技術。

## 1. 核心概念：Thunk (跳板函數)

**Thunk** 是一個中間函數，它攔截了原本指向引擎函數的調用。
- 你可以獲取傳遞給原函數的參數。
- 你可以在調用原函數之前或之後執行邏輯。
- 你甚至可以完全跳過原函數，直接返回自定義值。

## 2. 實作步驟

### A. 定義 Thunk 結構
通常使用一個結構體來封裝 Thunk，並保存原函數的地址。

```cpp
struct CalculateBaseFactor {
    // 1. 定義與原函數簽名一致的靜態函數
    static void thunk(float clampedSpeech, float* buyMult, float* sellMult) {
        // 執行自定義邏輯...
        
        // 如果需要，調用原函數
        func(clampedSpeech, buyMult, sellMult);
        
        // 修改結果
        *buyMult = 1.5f; 
    }

    // 2. 用於保存原函數地址的成員
    static inline REL::Relocation<decltype(thunk)> func;
};
```

### B. 安裝 Hook
在插件初始化時（通常是 `kPostLoad`），使用 `stl::write_thunk_call` 將引擎中的調用指令指向你的 Thunk。

```cpp
void Install() {
    // 找到包含該調用的引擎函數地址 (BarterMenu 相關邏輯)
    const REL::Relocation<std::uintptr_t> barterMenu{ RELOCATION_ID(50001, 50945) };

    // 在指定的偏移量處寫入 Thunk
    // OFFSET(SE_Offset, AE_Offset) 用於跨版本
    stl::write_thunk_call<CalculateBaseFactor>(barterMenu.address() + OFFSET(0x4E8, 0x7B3));
}
```

## 3. 直接內存修改 (NOPing)

有時候你不需要 Hook，而是想直接禁用某些遊戲邏輯。該插件演示了如何使用 NOP (`0x90`) 指令覆蓋原有的匯編代碼。

```cpp
// 禁用 AE 版本中的某些買入限制邏輯
// safe_fill(地址, 填充值, 數量)
REL::safe_fill(barterMenu.address() + 0x814, 0x90, 8); // 用 8 個 NOP 覆蓋原指令
```

## 4. 跨版本兼容性
該插件大量使用了以下工具：
- **`RELOCATION_ID(SE, AE)`**: 通過 ID 查找函數，不管遊戲版本如何，ID 是固定的。
- **`OFFSET(SE, AE)`**: 處理同一個函數在不同版本中內存偏移量的差異。

## 總結
這個例子展示了 SKSE 插件最強大的一面：**動態修改遊戲引擎的機器碼**。通過 Hook 交易系統的價格計算函數，插件實現了讓 NPC 根據其口才等級動態調整價格的功能。
