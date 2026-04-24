#pragma once

namespace NpcGenerator
{
    // 繼承自 ScriptEffect，這是純 C++ 邏輯最常用的基類
    class SpawnNpcEffect : public RE::ScriptEffect
    {
    public:
        // 我們要在 C++ 裡手動給它一個標識，或者直接在創建時替換 VTable
        void Update(float a_delta) override;
        void OnAdd(RE::MagicTarget* a_target) override;
    };

    class CustomizeNpcEffect : public RE::ScriptEffect
    {
    public:
        void OnAdd(RE::MagicTarget* a_target) override;
    };

    // 初始化函式：在 DataLoaded 時呼叫
    void InitializeMagic();
}
