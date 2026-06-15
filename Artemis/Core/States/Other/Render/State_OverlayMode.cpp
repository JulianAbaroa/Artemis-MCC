#include "pch.h"

#include "State_OverlayMode.h"

OverlayMode State_OverlayMode::Get() const
{
    return static_cast<OverlayMode>(m_Mode.load(std::memory_order_relaxed));
}

void State_OverlayMode::Next()
{
    const uint8_t n = static_cast<uint8_t>(OverlayMode::Count);
    uint8_t cur = m_Mode.load(std::memory_order_relaxed);
    m_Mode.store((cur + 1) % n, std::memory_order_relaxed);
    m_Page.store(0, std::memory_order_relaxed);
}

void State_OverlayMode::Prev()
{
    const uint8_t n = static_cast<uint8_t>(OverlayMode::Count);
    uint8_t cur = m_Mode.load(std::memory_order_relaxed);
    m_Mode.store((cur + n - 1) % n, std::memory_order_relaxed);
    m_Page.store(0, std::memory_order_relaxed);
}

void State_OverlayMode::Reset()
{
    m_Mode.store(static_cast<uint8_t>(OverlayMode::Default),
        std::memory_order_relaxed);
    m_Page.store(0, std::memory_order_relaxed);
}

int State_OverlayMode::GetPage() const 
{ 
    return m_Page.load(std::memory_order_relaxed); 
}

void State_OverlayMode::NextPage()
{
    m_Page.store(m_Page.load(std::memory_order_relaxed) + 1,
        std::memory_order_relaxed);
}

void State_OverlayMode::PrevPage()
{
    int p = m_Page.load(std::memory_order_relaxed);
    if (p > 0) m_Page.store(p - 1, std::memory_order_relaxed);
}

void State_OverlayMode::ResetPage() 
{ 
    m_Page.store(0, std::memory_order_relaxed); 
}

void State_OverlayMode::ClampPage(int maxPage)
{
    int p = m_Page.load(std::memory_order_relaxed);
    if (p > maxPage) m_Page.store(maxPage, std::memory_order_relaxed);
    if (p < 0) m_Page.store(0, std::memory_order_relaxed);
}