#pragma once

#include <atomic>
#include <cstdint>

inline constexpr uint32_t kNoSelection = 0xFFFFFFFF;

class State_Selection
{
public:
    uint32_t GetSelected() const;
    void SetSelected(uint32_t handle);

    void Clear();

    bool HasSelection() const;
    void RequestPick();
    bool ConsumePick();

private:
    std::atomic<uint32_t> m_Selected{ kNoSelection };
    std::atomic<bool> m_PickPending{ false };
};