# Skyrim 3D 自由飛行系統技術架構 (3D Flight System)

Skyrim 引擎原生僅支援陸地移動與簡化的游泳。要實作真正的 3D 自由飛行（如帶翅膀的角色），必須接管玩家的位移計算與重力邏輯。

---

## 1. 核心機制：接管位移控制

### A. 禁用原生重力 (Gravity Cancellation)
- **問題**: 只要玩家腳部離開 Navmesh，引擎就會施加垂直向下的重力。
- **方案**: 
    1.  **飛行狀態標記**: 當進入飛行狀態，每幀手動將垂直速度 `Z-Velocity` 設為 0，或者給予一個向上的平衡力。
    2.  **修改 Actor 狀態**: 透過 `SetGhost(true)` 或修改行為圖變數 `bIsFlying` 來干預引擎的墜落判定。

### B. 三維向量移動 (Vector Movement)
- **技術**: 使用 `RE::NiPoint3` 進行向量運算。
- **公式**: 
    - `前進方向 = 鏡頭前向向量 (Camera Forward Vector)`。
    - `新座標 = 舊座標 + (方向向量 * 速度 * DeltaTime)`。
- **優點**: 玩家可以朝向鏡頭指著的任何方向移動（包括垂直向上）。

---

## 2. 視覺表現：翅膀與動畫

### A. 骨架附加 (Attachment)
- **Node**: 翅膀通常附加在 `NPC Spine2 [Spn2]` 或 `NPC Neck [Neck]` 節點。
- **同步**: 翅膀必須具備獨立的骨架動畫（HKX），並透過行為圖變數同步玩家的飛行狀態（如：快飛時翅膀頻率變快）。

### B. 動畫圖切換
- **狀態機**: 必須新增 `Flight_Idle`, `Flight_Forward`, `Flight_Down` 等動畫區塊。
- **混合 (Blending)**: 實作從「走路」切換到「盤旋」的平滑過渡。

---

## 3. 環境與性能挑戰

### A. 加載速度與 uGrids
- **挑戰**: 飛行速度若過快，引擎加載 5x5 網格的速度會趕不上，導致玩家撞進未加載的「空洞」。
- **優化**: 必須在 C++ 中限制最大飛行速度，或動態觸發預加載。

### B. LOD 與視角
- **Culling**: 在極高空時，地面的小物件會被剔除。
- **LOD**: 確保 Worldspace 的遠景（LOD）數據完整，否則從空中看下去大地會是一片模糊。

---

## 4. 碰撞偵測 (Collision)

- **空氣牆**: 飛行時仍受 `Static` 物件的碰撞影響。
- **著陸判定**: 
    - 透過射線檢測 (Raycast) 向下偵測地面距離。
    - 當距離 < 閾值且速度向下時，切換回 `Walking` 狀態並重新啟用重力。

---

## 5. 核心類別原始碼標註

- **`RE::PlayerCharacter`**: 位移控制的核心。
- **`RE::PlayerCamera`**: 獲取飛行方向的依據。
- **`RE::NiPoint3`**: 3D 座標與向量運算。
- **`RE::CharacterProxy`**: 處理飛行時的物理碰撞。

---
*文件路徑：architectures/classified/World/Answers/3D_Flight_System_Architecture.md*
