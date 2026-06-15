#include "pch.h"

#include "State_FlyCamera.h"

bool State_FlyCamera::IsActive() const 
{ 
    return m_Active.load(); 
}

void State_FlyCamera::SetActive(bool v) 
{ 
    m_Active.store(v); 
}

void State_FlyCamera::SetKey(WPARAM vk, bool down)
{
    switch (vk)
    {
    case 'W':        m_KeyW.store(down); break;
    case 'A':        m_KeyA.store(down); break;
    case 'S':        m_KeyS.store(down); break;
    case 'D':        m_KeyD.store(down); break;
    case 'R':        m_KeyUp.store(down); break;
    case 'F':        m_KeyDown.store(down); break;
    case VK_SHIFT:   m_KeyShift.store(down); break;
    case VK_CONTROL: m_KeyCtrl.store(down); break;
    }
}

void State_FlyCamera::ResetKeys()
{
    m_KeyW.store(false); m_KeyA.store(false);
    m_KeyS.store(false); m_KeyD.store(false);
    m_KeyUp.store(false); m_KeyDown.store(false);
    m_KeyShift.store(false);
}

bool State_FlyCamera::KeyW() const 
{ 
    return m_KeyW.load(); 
}

bool State_FlyCamera::KeyA() const 
{ 
    return m_KeyA.load(); 
}

bool State_FlyCamera::KeyS() const 
{ 
    return m_KeyS.load(); 
}

bool State_FlyCamera::KeyD() const 
{ 
    return m_KeyD.load(); 
}

bool State_FlyCamera::KeyUp() const 
{ 
    return m_KeyUp.load(); 
}

bool State_FlyCamera::KeyDown() const 
{ 
    return m_KeyDown.load(); 
}

bool State_FlyCamera::KeyShift() const 
{ 
    return m_KeyShift.load(); 
}

bool State_FlyCamera::KeyCtrl() const
{
    return m_KeyCtrl.load();
}

void State_FlyCamera::AccumulateMouseDelta(float dx, float dy)
{
    m_MouseDeltaX.store(m_MouseDeltaX.load() + dx);
    m_MouseDeltaY.store(m_MouseDeltaY.load() + dy);
}

void State_FlyCamera::ConsumeMouseDelta(float& outX, float& outY)
{
    outX = m_MouseDeltaX.exchange(0.0f);
    outY = m_MouseDeltaY.exchange(0.0f);
}

bool State_FlyCamera::FollowEnabled() const
{
    return m_FollowEnabled.load(std::memory_order_relaxed);
}

void State_FlyCamera::SetFollowEnabled(bool v)
{
    m_FollowEnabled.store(v, std::memory_order_relaxed);
}