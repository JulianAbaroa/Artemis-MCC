module;

#include "External/imgui/imgui.h"

module UI.DefinitionsInspector.System;

import Common.Reflect.Type;
import Common.Reflect.System;
import Resolved.Definitions.Reflect;
import std;

namespace
{
    using std::filesystem::path;

    using Common::Reflect::Type::Reflectable;
    using Common::Reflect::Type::ForEachField;

    // Tag referenced by a field of a definition.
    struct Link
    {
        std::string FourCC{};
        std::string TagName{};
        std::string Field{};
    };

    // Returns the class of the tag that a field links to.
    // return: An empty view if the field is not a link.
    auto GroupForField(std::string_view field) -> std::string_view
    {
        if (field == "TagName") return {};
        if (field == "ModelTagName") return "hlmt";
        if (field == "CollisionModelTagName") return "coll";
        if (field == "RenderModelTagName") return "mode";
        if (field.ends_with("ProjectileTagName")) return "proj";
        if (field.ends_with("TagName")) return "jpt!";
        return {};
    }

    // Classes that can be dumped.
    constexpr std::array<const char*, 15> k_DumpFourCCs
    {
        "bipd", "bloc", "coll", "ctrl", "eqip", "hlmt", "jpt!", "mach", "mode", "proj", "scen", "scnr", "sddt", "vehi", "weap",
    };

    // Escapes the text to be placed inside a JSON string.
    auto JsonEscape(std::string_view text) -> std::string
    {
        std::string out{};
        out.reserve(text.size() + 2);
        for (const char c : text)
        {
            switch (c)
            {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    out += std::format("\\u{:04x}", static_cast<int>(c));
                }
                else
                {
                    out += c;
                }
            }
        }
        return out;
    }

    // Writes one JSON line per tag with the rows of its reflected fields.
    // return: The number of lines written, nullopt if the file could not be opened.
    template <typename TMap>
    auto WriteNdjson(const path& filePath, std::string_view fourCC, const TMap& tags) -> std::optional<std::size_t>
    {
        std::ofstream file{ filePath, std::ios::trunc };
        if (!file) return std::nullopt;

        std::size_t written{ 0 };
        for (const auto& [tagName, resolved] : tags)
        {
            file << "{\"tick\":0,\"tag\":\"" << JsonEscape(tagName)
                << "\",\"fourcc\":\"" << JsonEscape(fourCC)
                << "\",\"handle\":\"0x0\",\"fields\":{";

            const auto rows{ Common::Reflect::System::DumpToRows(resolved) };
            for (std::size_t i{ 0 }; i < rows.size(); ++i)
            {
                if (i > 0) file << ",";
                file << "\"" << JsonEscape(rows[i].Path) << "\":\"" << JsonEscape(rows[i].Value) << "\"";
            }
            file << "}}\n";
            ++written;
        }
        return written;
    }

    // Checks whether a type is a vector of reflectable elements.
    template <typename T>
    struct IsReflectableVector : std::false_type {};

    template <typename T, typename A>
    struct IsReflectableVector<std::vector<T, A>> : std::bool_constant<Reflectable<T>> {};

    // Collects the string fields that link to another tag, also inside nested structs and vectors.
    template <typename T> requires Reflectable<T>
    auto CollectLinks(const T& obj, std::vector<Link>& out) -> void
    {
        ForEachField(obj, [&](std::string_view name, auto const& value) {
            using V = std::remove_cvref_t<decltype(value)>;

            if constexpr (std::same_as<V, std::string>)
            {
                if (value.empty()) return;

                const std::string_view group{ GroupForField(name) };
                if (group.empty()) return;

                out.push_back({ std::string(group), value, std::string(name) });
            }
            else if constexpr (Reflectable<V>)
            {
                CollectLinks(value, out);
            }
            else if constexpr (IsReflectableVector<V>::value)
            {
                for (const auto& element : value)
                {
                    CollectLinks(element, out);
                }
            }
        });
    }
}

namespace UI::DefinitionsInspector::System
{
    template <typename TResolved>
    auto DefinitionsInspectorUIService::DrawLinks(const TResolved& resolved) -> void
    {
        std::vector<Link> links{};
        CollectLinks(resolved, links);

        std::erase_if(links, [this](const Link& link) {
            return !this->IsResolved(link.FourCC, link.TagName);
        });

        if (links.empty()) return;

        const std::string header{ std::format("Linked tags ({})##linked_tags", links.size()) };
        if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) return;

        for (std::size_t i{ 0 }; i < links.size(); ++i)
        {
            const Link& link{ links[i] };

            ImGui::PushID(static_cast<int>(i));
            if (ImGui::SmallButton("Open"))
            {
                m_Pinned = LinkedTag{ link.FourCC, link.TagName };
            }
            ImGui::SameLine();
            ImGui::Text("%s -> [%s] %s", link.Field.c_str(), link.FourCC.c_str(), link.TagName.c_str());
            ImGui::PopID();
        }

        ImGui::Separator();
    }

    template <typename TResolved>
    auto DefinitionsInspectorUIService::DrawResolved(const TResolved* resolved, const std::string& fourCC,
        const std::string& tagName, std::uint32_t ownerHandle) -> void
    {
        if (!resolved)
        {
            ImGui::TextDisabled("Without Resolved::Definitions for %s (%s)", tagName.c_str(), fourCC.c_str());
            return;
        }

        this->DrawLinks(*resolved);
        m_Inspector.Draw(*resolved, ownerHandle);
    }

    auto DefinitionsInspectorUIService::FindSelected() const -> const AliveObject*
    {
        if (!m_SelectionStore.HasSelection()) return nullptr;

        const auto tick{ m_TickStore.Acquire() };
        if (!tick || !tick->ObjectTable) return nullptr;

        const std::uint32_t handle{ m_SelectionStore.GetSelected() };
        const auto it{ tick->ObjectTable->find(handle) };
        if (it == tick->ObjectTable->end()) return nullptr;

        return &it->second;
    }

    auto DefinitionsInspectorUIService::IsResolved(const std::string& fourCC, const std::string& tagName) const -> bool
    {
        if (fourCC == "hlmt") return m_DefinitionsStore.Hlmt.Has(tagName);
        if (fourCC == "coll") return m_DefinitionsStore.Coll.Has(tagName);
        if (fourCC == "jpt!") return m_DefinitionsStore.Jpt.Has(tagName);
        if (fourCC == "mode") return m_DefinitionsStore.Mode.Has(tagName);
        if (fourCC == "proj") return m_DefinitionsStore.Proj.Has(tagName);
        if (fourCC == "vehi") return m_DefinitionsStore.Vehi.Has(tagName);
        if (fourCC == "weap") return m_DefinitionsStore.Weap.Has(tagName);
        return false;
    }

    auto DefinitionsInspectorUIService::DumpFourCC(const std::string& fourCC) -> std::string
    {
        if (!m_DefinitionsStore.Sbsp.IsFrozen()) return "Definitions not ready (map not loaded).";

        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty()) return "Storage Folder is not set (Settings). Set it and try again.";

        const std::string fileTag{ (fourCC == "jpt!") ? "jpt" : fourCC };
        std::error_code ec{};
        std::filesystem::create_directories(directory, ec);
        const path filePath{ path(directory) / ("definitions_dump_" + fileTag + ".ndjson") };

        std::optional<std::size_t> n{};
        if (fourCC == "bipd") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Bipd.All());
        else if (fourCC == "bloc") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Bloc.All());
        else if (fourCC == "coll") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Coll.All());
        else if (fourCC == "ctrl") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Ctrl.All());
        else if (fourCC == "eqip") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Eqip.All());
        else if (fourCC == "hlmt") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Hlmt.All());
        else if (fourCC == "jpt!") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Jpt.All());
        else if (fourCC == "mach") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Mach.All());
        else if (fourCC == "mode") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Mode.All());
        else if (fourCC == "proj") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Proj.All());
        else if (fourCC == "scen") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Scen.All());
        else if (fourCC == "scnr") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Scnr.All());
        else if (fourCC == "sddt") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Sddt.All());
        else if (fourCC == "vehi") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Vehi.All());
        else if (fourCC == "weap") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.Weap.All());

        if (!n) return std::format("Could not open {} for writing.", filePath.string());
        if (*n == 0) return std::format("No {} tags in this map's Definitions (file written empty: {}).", fourCC, filePath.string());
        return std::format("{} tags -> {}", *n, filePath.string());
    }

    auto DefinitionsInspectorUIService::DrawDump() -> void
    {
        if (!ImGui::CollapsingHeader("Dump for field-usage report##dump")) return;

        ImGui::SetNextItemWidth(80.0f);
        ImGui::Combo("##dump_fourcc", &m_DumpFourCC, k_DumpFourCCs.data(), static_cast<int>(k_DumpFourCCs.size()));
        ImGui::SameLine();
        if (ImGui::Button("Dump all tags of this type"))
        {
            m_DumpStatus = this->DumpFourCC(k_DumpFourCCs[m_DumpFourCC]);
        }
        if (!m_DumpStatus.empty()) ImGui::TextWrapped("%s", m_DumpStatus.c_str());
        ImGui::Separator();
    }

    auto DefinitionsInspectorUIService::Draw() -> void
    {
        this->DrawDump();

        const AliveObject* selected{ this->FindSelected() };

        if (!selected)
        {
            m_Pinned.reset();
            m_LastHandle.reset();
            ImGui::TextDisabled("Select an object in the Viewer to inspect its definitions.");
            return;
        }

        if (m_LastHandle != selected->Handle)
        {
            m_Pinned.reset();
            m_LastHandle = selected->Handle;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::Text("Tag: %s  (%s)", selected->TagName.c_str(), selected->FourCC.c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        this->DrawTag(selected->FourCC, selected->TagName, selected->Handle);

        if (m_Pinned)
        {
            const LinkedTag pinned{ *m_Pinned };

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
            ImGui::Text("Linked: [%s] %s", pinned.FourCC.c_str(), pinned.TagName.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if (ImGui::SmallButton("Close"))
            {
                m_Pinned.reset();
                return;
            }
            ImGui::Separator();

            ImGui::PushID("pinned_tag");
            this->DrawTag(pinned.FourCC, pinned.TagName, selected->Handle);
            ImGui::PopID();
        }
    }

    auto DefinitionsInspectorUIService::DrawTag(const std::string& fourCC, const std::string& tagName,
        std::uint32_t ownerHandle) -> void
    {
        if (fourCC == "bipd") this->DrawResolved(m_DefinitionsStore.Bipd.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "bloc") this->DrawResolved(m_DefinitionsStore.Bloc.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "coll") this->DrawResolved(m_DefinitionsStore.Coll.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "ctrl") this->DrawResolved(m_DefinitionsStore.Ctrl.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "eqip") this->DrawResolved(m_DefinitionsStore.Eqip.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "hlmt") this->DrawResolved(m_DefinitionsStore.Hlmt.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "jpt!") this->DrawResolved(m_DefinitionsStore.Jpt.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "mach") this->DrawResolved(m_DefinitionsStore.Mach.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "mode") this->DrawResolved(m_DefinitionsStore.Mode.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "proj") this->DrawResolved(m_DefinitionsStore.Proj.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "scen") this->DrawResolved(m_DefinitionsStore.Scen.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "scnr") this->DrawResolved(m_DefinitionsStore.Scnr.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "sddt") this->DrawResolved(m_DefinitionsStore.Sddt.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "vehi") this->DrawResolved(m_DefinitionsStore.Vehi.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "weap") this->DrawResolved(m_DefinitionsStore.Weap.Get(tagName), fourCC, tagName, ownerHandle);
        else ImGui::TextDisabled("FourCC \"%s\" without Resolved::Definitions yet.", fourCC.c_str());
    }
}