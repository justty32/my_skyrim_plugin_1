# CommonLibSSE NG 概覽 (00_overview)

CommonLibSSE NG 是一個針對《上古卷軸5：天際》（Skyrim）的 C++ 庫，它是原始 CommonLibSSE 的一個分支（Fork），旨在簡化 SKSE 插件的開發，並提供跨多個運行版本的兼容性。

## 核心目標
CommonLibSSE NG 的主要目標是讓開發者能夠編寫一份代碼，編譯出一個 DLL，就能在不同版本的 Skyrim 中運行，包括：
- **Skyrim Special Edition (SE)**: 1.5.x 版本。
- **Skyrim Anniversary Edition (AE)**: 1.6.x 版本。
- **Skyrim VR**: 虛擬實境版本。

## 主要特性

### 1. 多運行時目標支持 (Multi-Runtime Targeting)
這是 NG 版本最顯著的特性。它允許開發者在一個插件中同時支持 SE、AE 和 VR。通過抽象層，開發者無需為每個版本分別編寫代碼或維護多個項目分支。

### 2. 簡化的插件宣告
傳統的 SKSE 插件需要手動定義版本查詢和初始化函數。CommonLibSSE NG 通過 CMake 宏 `add_commonlibsse_plugin` 自動生成這些樣板代碼，簡化了開發流程。

### 3. 現代 C++ 標准
項目使用了現代 C++ 特性（當前為 C++23），充分利用了編譯器的優化和現代語言特性。

### 4. 逆向工程 (RE) 層
包含了大量的 Skyrim 引擎內部類、函數和結構體的定義（位於 `src/RE` 和 `include/RE`）。這使得開發者可以直接與遊戲引擎的內部邏輯進行交互，而無需從零開始逆向。

### 5. 地址庫集成 (Address Library)
利用 Address Library (REL) 技術，通過 ID 而不是硬編碼的內存地址來定位引擎函數。這保證了插件在遊戲更新後（地址發生變化時）依然能夠正常工作。

## 與 SKSE 的關係
CommonLibSSE NG 並不取代 SKSE，而是構建在 SKSE 之上（或與之並行）。它提供了更高級的抽象和更完整的引擎內部類定義。插件運行時仍需要安裝 SKSE 核心組件。
