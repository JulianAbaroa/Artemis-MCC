module;

#include "External/imgui/imgui.h"

module UI.ObjectTable.System;

import UI.Format.System;
import std;

namespace
{
	using HexFormater = UI::Format::System::HexFormater;

	constexpr float k_CardMinWidth = 250.0f;
	constexpr float k_CardHeight = 270.0f;
	constexpr float k_CardPadding = 25.0f;
	constexpr float k_CardSpacing = 8.0f;

	auto CardWidth(const std::string& tagName) -> float
	{
		const std::string text = "Tag: " + tagName;
		return (std::max)(k_CardMinWidth, ImGui::CalcTextSize(text.c_str()).x + k_CardPadding);
	}
}

namespace UI::ObjectTable::System
{
	auto ObjectTableUIService::Draw() -> void
	{
		this->RefreshSnapshot();

		const auto count = static_cast<int>(m_ObjectTableUIStore.GetObjectCount());
		ImGui::TextDisabled("Live Objects Count: %d", count);

		ImGui::Separator();

		m_SearchFilter.DrawSearchBar("Search by tag, class, handle, address...");

		ImGui::Separator();

		if (!ImGui::BeginChild("Objects Region"))
		{
			ImGui::EndChild();
			return;
		}

		const float windowRightEdge = ImGui::GetCursorScreenPos().x +
			ImGui::GetContentRegionAvail().x;

		std::vector<const AliveObject*> filtered;

		for (const auto& [className, group] : m_ObjectTableUIStore.GetGroups())
		{
			filtered.clear();

			for (const AliveObject* object : group)
			{
				if (this->PassesFilter(*object)) filtered.push_back(object);
			}

			if (filtered.empty()) continue;

			const std::string headerLabel = std::format("{} ({})###header {}",
				className, filtered.size(), className);

			if (!ImGui::CollapsingHeader(headerLabel.c_str())) continue;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(k_CardSpacing, k_CardSpacing));

			for (std::size_t i = 0; i < filtered.size(); ++i)
			{
				this->DrawObjectCard(*filtered[i]);

				const bool hasNext = i + 1 < filtered.size();
				if (!hasNext) continue;

				const float nextCardRightEdge = ImGui::GetItemRectMax().x +
					k_CardSpacing + CardWidth(filtered[i + 1]->TagName);

				if (nextCardRightEdge < windowRightEdge) ImGui::SameLine();
			}

			ImGui::PopStyleVar();
			ImGui::Spacing();
		}

		ImGui::EndChild();
	}

	auto ObjectTableUIService::RefreshSnapshot() -> void
	{
		const auto tick = m_TickStore.Acquire();
		auto table = tick ? tick->ObjectTable : nullptr;

		if (m_ObjectTableUIStore.IsSameSnapshot(table)) return;

		if (!table)
		{
			m_ObjectTableUIStore.Cleanup();
			return;
		}

		ObjectTableUIStore::Groups groups;

		for (const auto& [handle, object] : *table)
		{
			groups[object.FourCC].push_back(&object);
		}

		for (auto& [className, group] : groups)
		{
			std::ranges::sort(group, {}, &AliveObject::Handle);
		}

		m_ObjectTableUIStore.SetSnapshot(std::move(table), std::move(groups));
	}

	auto ObjectTableUIService::PassesFilter(const AliveObject& object) const -> bool
	{
		if (!m_SearchFilter.IsActive()) return true;

		return m_SearchFilter.Matches(object.TagName) ||
			m_SearchFilter.Matches(object.FourCC) ||
			m_SearchFilter.Matches(HexFormater::Hex32(object.Handle)) ||
			m_SearchFilter.Matches(HexFormater::Hex64(object.Address));
	}

	auto ObjectTableUIService::DrawObjectCard(const AliveObject& object) -> void
	{
		const ImVec2 cardSize(CardWidth(object.TagName), k_CardHeight);

		ImGui::PushID(static_cast<int>(object.Handle));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;

		if (ImGui::BeginChild("##card", cardSize, true, flags))
		{
			this->DrawCardHeader(object);
			this->DrawCardFields(object);
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopID();
	}

	auto ObjectTableUIService::DrawCardHeader(const AliveObject& object) -> void
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

		ImGui::TextUnformatted("Tag:");
		ImGui::SameLine();
		ImGui::TextUnformatted(object.TagName.c_str());

		ImGui::PopStyleColor();

		ImGui::Separator();
	}

	auto ObjectTableUIService::DrawCardFields(const AliveObject& object) -> void
	{
		ImGui::Indent(5.0f);

		m_CopyableField.Draw("Class:", object.FourCC, object.Handle);

		m_CopyableField.Draw("Datum Index:",
			HexFormater::Hex32(object.DatumIndex), object.Handle);

		m_CopyableField.Draw("Handle:",
			HexFormater::Hex32(object.Handle), object.Handle);

		m_CopyableField.Draw("Sibling Handle:",
			HexFormater::Hex32(object.NextSiblingHandle), object.Handle);

		m_CopyableField.Draw("Child Handle:",
			HexFormater::Hex32(object.ChildHandle), object.Handle);

		m_CopyableField.Draw("Parent Handle:",
			HexFormater::Hex32(object.ParentHandle), object.Handle);

		m_CopyableField.Draw("Address:",
			HexFormater::Hex64(object.Address), object.Handle);

		ImGui::Unindent(5.0f);
	}
}