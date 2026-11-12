module;

#include "External/imgui/imgui.h"

module DevTools.Gui.Widget.System;
import :DisabledScope;
import :Status;

import std;

namespace DevTools::Gui::Widget::System
{
    DisabledScope::DisabledScope(bool isDisabled, const char* reason) :
        m_IsDisabled(isDisabled),
        m_Reason(reason)
    {
        ImGui::BeginDisabled(isDisabled);
    }

    DisabledScope::~DisabledScope()
    {
        ImGui::EndDisabled();
    }

    auto DisabledScope::Tooltip(const char* enabledText) const -> void
    {
        StatusGuiService::ShowTooltip(m_IsDisabled ? m_Reason : enabledText);
    }
}