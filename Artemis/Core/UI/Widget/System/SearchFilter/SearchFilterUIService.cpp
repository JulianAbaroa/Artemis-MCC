module;

#include "External/imgui/imgui.h"

module UI.Widget.System;
import :SearchFilter;

import std;

namespace
{
    auto ToLower(unsigned char c) -> char
    {
        return static_cast<char>(std::tolower(c));
    }
}

namespace UI::Widget::System
{
    auto SearchFilterUIService::DrawSearchBar(const char* hint) -> void
    {
        ImGui::SetNextItemWidth(-1.0f);

        if (ImGui::InputTextWithHint("##search", hint, m_Query, sizeof(m_Query)))
        {
            this->RefreshLowerQuery();
        }
    }

    auto SearchFilterUIService::GetQuery() const -> std::string_view
    {
        return std::string_view(m_Query);
    }

    auto SearchFilterUIService::Matches(std::string_view text) const -> bool
    {
        if (m_LowerQuery.empty()) return true;
        if (text.size() < m_LowerQuery.size()) return false;

        const auto result = std::ranges::search(text, m_LowerQuery,
            [](char a, char b) { return ToLower(static_cast<unsigned char>(a)) == b; });

        return !result.empty();
    }

    auto SearchFilterUIService::RefreshLowerQuery() -> void
    {
        m_LowerQuery.clear();

        for (const char c : this->GetQuery())
        {
            m_LowerQuery.push_back(ToLower(static_cast<unsigned char>(c)));
        }
    }
}