#pragma once

#include <atomic>
#include <cstdint>

enum class OverlayMode : uint8_t
{
    Default = 0,
    Collidable,
    Vitality,
    Fixture,
    Affordance,

    Count
};

class State_OverlayMode
{
public:
    OverlayMode Get() const;
    void Next();
    void Prev();

    void Reset();

    int GetPage() const;
    void NextPage();
    void PrevPage();

    void ResetPage();
    void ClampPage(int maxPage);

private:
    std::atomic<uint8_t> m_Mode{ static_cast<uint8_t>(OverlayMode::Default) };
    std::atomic<int> m_Page{ 0 };
};