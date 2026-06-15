#pragma once

#include <Windows.h>
#include <atomic>

class State_FlyCamera
{
public:
    bool IsActive() const;
    void SetActive(bool v);

    void SetKey(WPARAM vk, bool down);
    void ResetKeys();

    bool KeyW() const;
    bool KeyA() const;
    bool KeyS() const;
    bool KeyD() const;
    bool KeyUp() const;
    bool KeyDown() const;
    bool KeyShift() const;
    bool KeyCtrl() const;

    void AccumulateMouseDelta(float dx, float dy);
    void ConsumeMouseDelta(float& outX, float& outY);

    bool FollowEnabled() const;
    void SetFollowEnabled(bool v);

private:
    std::atomic<bool> m_Active{ false };

    std::atomic<bool> m_KeyW{ false };
    std::atomic<bool> m_KeyA{ false };
    std::atomic<bool> m_KeyS{ false };
    std::atomic<bool> m_KeyD{ false };
    std::atomic<bool> m_KeyUp{ false };
    std::atomic<bool> m_KeyDown{ false };
    std::atomic<bool> m_KeyShift{ false };
    std::atomic<bool> m_KeyCtrl{ false };

    std::atomic<float> m_MouseDeltaX{ 0.0f };
    std::atomic<float> m_MouseDeltaY{ 0.0f };

    std::atomic<bool> m_FollowEnabled{ false };
};