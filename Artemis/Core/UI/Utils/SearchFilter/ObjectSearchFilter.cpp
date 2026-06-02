#include "pch.h"

#include "ObjectSearchFilter.h"

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

void ObjectSearchFilter::DrawSearchBar() const
{
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "Search by tag, class, handle, address...",
        m_Query, sizeof(m_Query));
}

bool ObjectSearchFilter::PassesFilter(const LiveObject& object) const
{
    if (!IsActive()) return true;

    std::string query = ToLower(m_Query);

    if (ContainsQuery(object.TagName, query)) return true;
    if (ContainsQuery(object.FourCC, query)) return true;

    char buf[64];
    snprintf(buf, sizeof(buf), "0x%08x", object.Handle);          if (ContainsQuery(buf, query)) return true;
    snprintf(buf, sizeof(buf), "0x%08x", object.DatumIndex);      if (ContainsQuery(buf, query)) return true;
    snprintf(buf, sizeof(buf), "0x%08x", object.NextSiblingHandle);if (ContainsQuery(buf, query)) return true;
    snprintf(buf, sizeof(buf), "0x%08x", object.ChildHandle);     if (ContainsQuery(buf, query)) return true;
    snprintf(buf, sizeof(buf), "0x%08x", object.ParentHandle);    if (ContainsQuery(buf, query)) return true;
    snprintf(buf, sizeof(buf), "0x%012llx", object.Address);         if (ContainsQuery(buf, query)) return true;

    return false;
}