#pragma once

#include <memory>

struct Tick;

class State_Render;
class State_Telemetry;
class State_OverlayMode;
class State_Selection;

struct UI_Overlay_Deps
{
    State_Render& State_Render;
    State_Selection& State_Selection;
    State_OverlayMode& State_OverlayMode;
    State_Telemetry& State_Telemetry;
};

class UI_Overlay
{
public:
    UI_Overlay(UI_Overlay_Deps deps) : m_Deps(deps) {}
    ~UI_Overlay() = default;

    void Draw(std::shared_ptr<const Tick> tick);

    bool IsVisible() const { return m_IsVisible; }
    void ToggleVisible() { m_IsVisible = !m_IsVisible; }

private:
    UI_Overlay_Deps m_Deps;
    bool m_IsVisible = false;

    uint32_t m_LastHandle = 0xFFFFFFFF;

    void DrawNavBar();
    void DrawDefault();
    void DrawSelectedPanel(const std::shared_ptr<const Tick>& tick);

    void DrawFPS();
    void DrawTelemetry();
};