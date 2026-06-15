#pragma once

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

class UI_Tab
{
public:
	virtual ~UI_Tab() = default;

    static bool s_Locked;
    static int s_VisibleCount;

    bool IsVisible() const;
    void ToggleVisible();
    void RequestReset();

    void ApplyResetIfRequested(const char* windowName);
    void FirstDraw();

protected:
	bool m_Visible = false;
	bool m_ResetRequested = false;
};