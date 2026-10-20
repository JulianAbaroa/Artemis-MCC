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
        if (field == "PhysicsModelTagName") return "phmo";
        if (field == "RenderModelTagName") return "mode";
        if (field.ends_with("ProjectileTagName")) return "proj";
        if (field.ends_with("TagName")) return "jpt!";
        return {};
    }

    // Classes that can be dumped.
    constexpr std::array<const char*, 16> k_DumpFourCCs
    {
        "bipd", "bloc", "coll", "ctrl", "eqip", "hlmt", "jpt!", "mach", "mode", "phmo", "proj", "scen", "scnr", "sddt", "vehi", "weap",
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
                if (static_cast<unsigned char>(c) < ' ')
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

    // Folder inside the storage folder that receives one tag catalog file per map.
    constexpr std::string_view k_CatalogFolder{ "TagCatalog" };

    // Escapes the text to be placed inside an XML attribute.
    auto XmlEscape(std::string_view text) -> std::string
    {
        std::string out{};
        out.reserve(text.size());

        for (const char c : text)
        {
            switch (c)
            {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c; break;
            }
        }

        return out;
    }

    // Writes one catalog element per tag of the map.
    // note: kind and mp hold the numeric values of ObjectKind and MultiplayerObjectKind. The script names them from the enums.
    // return: The number of tags written.
    template <typename TMap>
    auto WriteCatalogGroup(std::ofstream& file, std::string_view fourCC, const TMap& tags) -> std::size_t
    {
        std::size_t written{ 0 };

        for (const auto& [tagName, resolved] : tags)
        {
            const auto& base{ resolved.Base };

            std::string kinds{};
            for (const auto& multiplayer : base.MultiplayerObjects)
            {
                if (!kinds.empty()) kinds += ",";

                kinds += std::to_string(static_cast<int>(multiplayer.MultiplayerObjectKind));
            }

            file << std::format("  <tag name=\"{}\" group=\"{}\" kind=\"{}\" mp=\"{}\" />\n",
                XmlEscape(tagName), fourCC, static_cast<int>(base.ObjectKind), kinds);

            ++written;
        }

        return written;
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
        if (fourCC == "hlmt") return m_DefinitionsStore.m_Hlmt.Has(tagName);
        if (fourCC == "coll") return m_DefinitionsStore.m_Coll.Has(tagName);
        if (fourCC == "jpt!") return m_DefinitionsStore.m_Jpt.Has(tagName);
        if (fourCC == "mode") return m_DefinitionsStore.m_Mode.Has(tagName);
        if (fourCC == "phmo") return m_DefinitionsStore.m_Phmo.Has(tagName);
        if (fourCC == "proj") return m_DefinitionsStore.m_Proj.Has(tagName);
        if (fourCC == "vehi") return m_DefinitionsStore.m_Vehi.Has(tagName);
        if (fourCC == "weap") return m_DefinitionsStore.m_Weap.Has(tagName);
        return false;
    }

    auto DefinitionsInspectorUIService::DumpFourCC(const std::string& fourCC) -> std::string
    {
        if (!m_DefinitionsStore.m_Sbsp.IsFrozen()) return "Definitions not ready (map not loaded).";

        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty()) return "Storage Folder is not set (Settings). Set it and try again.";

        const std::string fileTag{ (fourCC == "jpt!") ? "jpt" : fourCC };
        std::error_code ec{};
        std::filesystem::create_directories(directory, ec);
        const path filePath{ path(directory) / ("definitions_dump_" + fileTag + ".ndjson") };

        std::optional<std::size_t> n{};
        if (fourCC == "bipd") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Bipd.All());
        else if (fourCC == "bloc") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Bloc.All());
        else if (fourCC == "coll") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Coll.All());
        else if (fourCC == "ctrl") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Ctrl.All());
        else if (fourCC == "eqip") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Eqip.All());
        else if (fourCC == "hlmt") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Hlmt.All());
        else if (fourCC == "jpt!") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Jpt.All());
        else if (fourCC == "mach") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Mach.All());
        else if (fourCC == "mode") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Mode.All());
        else if (fourCC == "phmo") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Phmo.All());
        else if (fourCC == "proj") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Proj.All());
        else if (fourCC == "scen") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Scen.All());
        else if (fourCC == "scnr") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Scnr.All());
        else if (fourCC == "sddt") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Sddt.All());
        else if (fourCC == "vehi") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Vehi.All());
        else if (fourCC == "weap") n = WriteNdjson(filePath, fourCC, m_DefinitionsStore.m_Weap.All());

        if (!n) return std::format("Could not open {} for writing.", filePath.string());
        if (*n == 0) return std::format("No {} tags in this map's Definitions (file written empty: {}).", fourCC, filePath.string());
        return std::format("{} tags -> {}", *n, filePath.string());
    }

    auto DefinitionsInspectorUIService::DumpCatalog() -> std::string
    {
        if (!m_DefinitionsStore.m_Sbsp.IsFrozen()) return "Definitions not ready (map not loaded).";

        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty()) return "Storage Folder is not set (Settings). Set it and try again.";

        const auto& scenarios{ m_DefinitionsStore.m_Scnr.All() };
        if (scenarios.empty()) return "The map has no scenario tag.";

        const std::string mapName{ path(scenarios.begin()->first).stem().string() };

        const path folder{ path(directory) / k_CatalogFolder };
        std::error_code ec{};
        std::filesystem::create_directories(folder, ec);
        const path filePath{ folder / (mapName + ".xml") };

        std::ofstream file{ filePath, std::ios::trunc };
        if (!file) return std::format("Could not open {} for writing.", filePath.string());

        file << std::format("<map name=\"{}\">\n", XmlEscape(mapName));

        std::size_t written{ 0 };
        written += WriteCatalogGroup(file, "bipd", m_DefinitionsStore.m_Bipd.All());
        written += WriteCatalogGroup(file, "bloc", m_DefinitionsStore.m_Bloc.All());
        written += WriteCatalogGroup(file, "ctrl", m_DefinitionsStore.m_Ctrl.All());
        written += WriteCatalogGroup(file, "eqip", m_DefinitionsStore.m_Eqip.All());
        written += WriteCatalogGroup(file, "mach", m_DefinitionsStore.m_Mach.All());
        written += WriteCatalogGroup(file, "proj", m_DefinitionsStore.m_Proj.All());
        written += WriteCatalogGroup(file, "scen", m_DefinitionsStore.m_Scen.All());
        written += WriteCatalogGroup(file, "vehi", m_DefinitionsStore.m_Vehi.All());
        written += WriteCatalogGroup(file, "weap", m_DefinitionsStore.m_Weap.All());

        file << "</map>";

        return std::format("{} object tags -> {}", written, filePath.string());
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

        if (ImGui::Button("Dump object tag catalog of this map"))
        {
            m_CatalogStatus = this->DumpCatalog();
        }
        if (!m_CatalogStatus.empty()) ImGui::TextWrapped("%s", m_CatalogStatus.c_str());
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
        if (fourCC == "bipd") this->DrawResolved(m_DefinitionsStore.m_Bipd.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "bloc") this->DrawResolved(m_DefinitionsStore.m_Bloc.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "coll") this->DrawResolved(m_DefinitionsStore.m_Coll.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "ctrl") this->DrawResolved(m_DefinitionsStore.m_Ctrl.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "eqip") this->DrawResolved(m_DefinitionsStore.m_Eqip.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "hlmt") this->DrawResolved(m_DefinitionsStore.m_Hlmt.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "jpt!") this->DrawResolved(m_DefinitionsStore.m_Jpt.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "mach") this->DrawResolved(m_DefinitionsStore.m_Mach.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "mode") this->DrawResolved(m_DefinitionsStore.m_Mode.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "phmo") this->DrawResolved(m_DefinitionsStore.m_Phmo.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "proj") this->DrawResolved(m_DefinitionsStore.m_Proj.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "scen") this->DrawResolved(m_DefinitionsStore.m_Scen.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "scnr") this->DrawResolved(m_DefinitionsStore.m_Scnr.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "sddt") this->DrawResolved(m_DefinitionsStore.m_Sddt.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "vehi") this->DrawResolved(m_DefinitionsStore.m_Vehi.Get(tagName), fourCC, tagName, ownerHandle);
        else if (fourCC == "weap") this->DrawResolved(m_DefinitionsStore.m_Weap.Get(tagName), fourCC, tagName, ownerHandle);
        else ImGui::TextDisabled("FourCC \"%s\" without Resolved::Definitions yet.", fourCC.c_str());
    }
}