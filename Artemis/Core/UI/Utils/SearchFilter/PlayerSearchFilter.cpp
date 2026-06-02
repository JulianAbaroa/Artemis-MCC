#include "pch.h"

#include "PlayerSearchFilter.h"

#include "External/imgui/imgui.h"

#include <algorithm>

static std::string ToLower(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return out;
}

static bool ContainsQuery(const std::string& haystack, const std::string& query)
{
    return ToLower(haystack).find(query) != std::string::npos;
}

void PlayerSearchFilter::DrawSearchBar() const
{
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "Search by gamertag, tag, handle, address...",
        m_Query, sizeof(m_Query));
}

bool PlayerSearchFilter::PassesFilter(const LivePlayer& player) const
{
    if (!IsActive()) return true;

    std::string query = ToLower(m_Query);

    if (ContainsQuery(player.Gamertag, query)) return true;
    if (ContainsQuery(player.Tag, query)) return true;

    char buf[64];
    snprintf(buf, sizeof(buf), "0x%08x", player.Handle);  if (ContainsQuery(buf, query)) return true;
    snprintf(buf, sizeof(buf), "0x%012llx", player.Address); if (ContainsQuery(buf, query)) return true;

    return false;
}