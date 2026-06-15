#pragma once

#include "Core/UI/Utils/Icon/IconTexture.h"

#include <vector>

struct IconSet;
class UI_Tab;
class State_Render;

struct UI_Launcher_Deps
{
    std::vector<UI_Tab*> Tabs;
    State_Render& State_Render;
};

class UI_Launcher
{
public:
    UI_Launcher(UI_Launcher_Deps deps) : m_Deps(deps) {};
    ~UI_Launcher() = default;

    static bool s_IsVisible;

    void Draw();
	void DrawDockSpace();

    bool IsVisible() const { return m_IsVisible; }

    void ToggleVisible();
    void ToggleAllTabs();

    const std::vector<UI_Tab*>& GetTabs() const;
    void ResetTabs();

private:
    UI_Launcher_Deps m_Deps;
    
    struct IconSet
    {
        IconTexture Objects;
        IconTexture Players;
        IconTexture Settings;
        IconTexture Scanner;
        IconTexture Logs;
    } m_Icons;

    bool m_IsLoaded = false;
    bool m_IsVisible = false;
    std::vector<bool> m_PreviousTabState;

    void DrawToggleButton(const IconTexture& icon, UI_Tab& tab);
    void LoadIcons();
};