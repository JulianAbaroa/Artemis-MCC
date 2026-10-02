module;

#include "External/imgui/imgui.h"

module UI.DefinitionsInspector.System;

import Common.Reflect.Type;
import Common.Reflect.System;
import Resolved.Definitions.Reflect;
import std;

namespace
{
	using Common::Reflect::Type::Reflectable;
	using Common::Reflect::Type::ForEachField;

	struct Link
	{
		std::string FourCC{};
		std::string TagName{};
		std::string Field{};
	};

	auto GroupForField(std::string_view field) -> std::string_view
	{
		if (field == "TagName") return {};
		if (field == "ModelTagName") return "hlmt";
		if (field == "CollisionModelTagName") return "coll";
		if (field == "RenderModelTagName") return "mode";
		if (field == "PhysicsModelTagName") return "phmo";
		if (field.ends_with("ProjectileTagName")) return "proj";
		if (field.ends_with("TagName")) return "jpt!";
		return {};
	}

	constexpr std::array<const char*, 15> k_DumpFourCCs{
		"bipd", "bloc", "coll", "ctrl", "eqip", "hlmt", "jpt!", "mach", "mode", "phmo", "proj", "scen", "scnr", "vehi", "weap"};

	auto JsonEscape(std::string_view text) -> std::string
	{
		std::string out;
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
					out += std::format("\\u{:04x}", static_cast<int>(c));
				else
					out += c;
			}
		}
		return out;
	}

	template <typename TMap>
	auto WriteNdjson(const std::filesystem::path& path, std::string_view fourCC, const TMap& tags) -> std::optional<std::size_t>
	{
		std::ofstream file(path, std::ios::trunc);
		if (!file) return std::nullopt;

		std::size_t written = 0;
		for (const auto& [tagName, resolved] : tags)
		{
			file << "{\"tick\":0,\"tag\":\"" << JsonEscape(tagName)
				<< "\",\"fourcc\":\"" << JsonEscape(fourCC)
				<< "\",\"handle\":\"0x0\",\"fields\":{";

			const auto rows = Common::Reflect::System::DumpToRows(resolved);
			for (std::size_t i = 0; i < rows.size(); ++i)
			{
				if (i > 0) file << ",";
				file << "\"" << JsonEscape(rows[i].Path) << "\":\"" << JsonEscape(rows[i].Value) << "\"";
			}
			file << "}}\n";
			++written;
		}
		return written;
	}

	template <typename T>
	struct IsReflectableVector : std::false_type {};

	template <typename T, typename A>
	struct IsReflectableVector<std::vector<T, A>> : std::bool_constant<Reflectable<T>> {};

	template <typename T> requires Reflectable<T>
	auto CollectLinks(const T& obj, std::vector<Link>& out) -> void
	{
		ForEachField(obj, [&](std::string_view name, auto const& value)
			{
				using V = std::remove_cvref_t<decltype(value)>;

				if constexpr (std::same_as<V, std::string>)
				{
					if (value.empty()) return;

					const std::string_view group = GroupForField(name);
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

		std::erase_if(links, [this](const Link& link)
			{
				return !this->HasResolved(link.FourCC, link.TagName);
			});

		if (links.empty()) return;

		const std::string header = std::format("Linked tags ({})##linked_tags", links.size());
		if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) return;

		for (std::size_t i = 0; i < links.size(); ++i)
		{
			const Link& link = links[i];

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

		const auto tick = m_TickStore.Acquire();
		if (!tick || !tick->ObjectTable) return nullptr;

		const std::uint32_t handle = m_SelectionStore.GetSelected();
		const auto it = tick->ObjectTable->find(handle);
		if (it == tick->ObjectTable->end()) return nullptr;

		return &it->second;
	}

	auto DefinitionsInspectorUIService::HasResolved(const std::string& fourCC, const std::string& tagName) const -> bool
	{
		if (fourCC == "hlmt") return m_DefinitionsStore.HasResolvedHlmt(tagName);
		if (fourCC == "coll") return m_DefinitionsStore.HasResolvedColl(tagName);
		if (fourCC == "jpt!") return m_DefinitionsStore.HasResolvedJpt(tagName);
		if (fourCC == "mode") return m_DefinitionsStore.HasResolvedMode(tagName);
		if (fourCC == "phmo") return m_DefinitionsStore.HasResolvedPhmo(tagName);
		if (fourCC == "proj") return m_DefinitionsStore.HasResolvedProj(tagName);
		if (fourCC == "vehi") return m_DefinitionsStore.HasResolvedVehi(tagName);
		if (fourCC == "weap") return m_DefinitionsStore.HasResolvedWeap(tagName);
		return false;
	}

	auto DefinitionsInspectorUIService::DumpFourCC(const std::string& fourCC) -> std::string
	{
		if (!m_DefinitionsStore.IsFrozen()) return "Definitions not ready (map not loaded).";

		const std::string directory = m_SettingsStore.GetAppDataDirectory();
		if (directory.empty()) return "Storage Folder is not set (Settings). Set it and try again.";

		const std::string fileTag = (fourCC == "jpt!") ? "jpt" : fourCC;
		std::error_code ec{};
		std::filesystem::create_directories(directory, ec);
		const std::filesystem::path path = std::filesystem::path(directory) / ("definitions_dump_" + fileTag + ".ndjson");

		std::optional<std::size_t> n;
		if (fourCC == "bipd") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedBipds());
		else if (fourCC == "bloc") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedBlocs());
		else if (fourCC == "coll") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedColls());
		else if (fourCC == "ctrl") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedCtrls());
		else if (fourCC == "eqip") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedEqips());
		else if (fourCC == "hlmt") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedHlmts());
		else if (fourCC == "jpt!") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedJpts());
		else if (fourCC == "mach") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedMachs());
		else if (fourCC == "mode") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedModes());
		else if (fourCC == "phmo") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedPhmos());
		else if (fourCC == "proj") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedProjs());
		else if (fourCC == "scen") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedScens());
		else if (fourCC == "scnr") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedScnrs());
		else if (fourCC == "vehi") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedVehis());
		else if (fourCC == "weap") n = WriteNdjson(path, fourCC, m_DefinitionsStore.GetAllResolvedWeaps());

		if (!n) return std::format("Could not open {} for writing.", path.string());
		if (*n == 0) return std::format("No {} tags in this map's Definitions (file written empty: {}).", fourCC, path.string());
		return std::format("{} tags -> {}", *n, path.string());
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

		const AliveObject* selected = this->FindSelected();

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
			const LinkedTag pinned = *m_Pinned;

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

	auto DefinitionsInspectorUIService::DrawTag(const std::string& fourCC, const std::string& tag,
		std::uint32_t ownerHandle) -> void
	{
		if (fourCC == "bipd") this->DrawResolved(m_DefinitionsStore.GetResolvedBipd(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "bloc") this->DrawResolved(m_DefinitionsStore.GetResolvedBloc(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "coll") this->DrawResolved(m_DefinitionsStore.GetResolvedColl(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "ctrl") this->DrawResolved(m_DefinitionsStore.GetResolvedCtrl(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "eqip") this->DrawResolved(m_DefinitionsStore.GetResolvedEqip(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "hlmt") this->DrawResolved(m_DefinitionsStore.GetResolvedHlmt(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "jpt!") this->DrawResolved(m_DefinitionsStore.GetResolvedJpt(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "mach") this->DrawResolved(m_DefinitionsStore.GetResolvedMach(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "mode") this->DrawResolved(m_DefinitionsStore.GetResolvedMode(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "phmo") this->DrawResolved(m_DefinitionsStore.GetResolvedPhmo(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "proj") this->DrawResolved(m_DefinitionsStore.GetResolvedProj(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "scen") this->DrawResolved(m_DefinitionsStore.GetResolvedScen(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "scnr") this->DrawResolved(m_DefinitionsStore.GetResolvedScnr(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "vehi") this->DrawResolved(m_DefinitionsStore.GetResolvedVehi(tag), fourCC, tag, ownerHandle);
		else if (fourCC == "weap") this->DrawResolved(m_DefinitionsStore.GetResolvedWeap(tag), fourCC, tag, ownerHandle);
		else ImGui::TextDisabled("FourCC \"%s\" without Resolved::Definitions yet.", fourCC.c_str());
	}
}