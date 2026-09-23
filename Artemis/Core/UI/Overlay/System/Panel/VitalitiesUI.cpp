module;

#include "External/imgui/imgui.h"

module UI.Overlay.System;
import :Vitalities;

import Resolved.Vitality.Type;
import UI.Format.System;
import std;

namespace
{
	using Tick = Export::Tick::Type::Tick;
	using Health = Export::Tick::Type::Health;
	using ObjectTable = Export::Tick::Type::ObjectTable;
	using Section = Resolved::Vitality::Type::Section;
	using SectionKind = Resolved::Vitality::Type::Kind;
	using ResolvedVitality = Resolved::Vitality::Type::Vitality;
	using HexFormater = UI::Format::System::HexFormater;

	auto DrawSectionRow(std::size_t index, float vitality, const Section* section) -> void
	{
		const bool isShield = section && section->Kind == SectionKind::Shield;
		const bool isCritical = section && section->IsCritical;

		if (section) ImGui::Text("Section %u", static_cast<unsigned>(section->NameId));
		else         ImGui::Text("Section #%zu", index);

		if (isCritical)
		{
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 0.5f, 0.3f, 1.0f }, "[critical]");
		}

		if (section && section->IsHeadshot)
		{
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 0.85f, 0.2f, 1.0f }, "[headshot]");
		}

		if (isShield)
		{
			ImGui::SameLine();
			ImGui::TextColored({ 0.4f, 0.7f, 1.0f, 1.0f }, "[shield]");
		}

		if (section && section->CollRegion < 0 && !isShield)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(no coll)");
		}

		const float clamped = std::clamp(vitality, 0.0f, 1.0f);

		const ImVec4 barColor = isShield
			? ImVec4(0.35f, 0.65f, 1.0f, 1.0f)
			: (isCritical
				? ImVec4(0.9f, 0.45f, 0.3f, 1.0f)
				: ImVec4(0.45f, 0.85f, 0.5f, 1.0f));

		const std::string overlay = std::format("{:.2f}", vitality);

		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
		ImGui::ProgressBar(clamped, ImVec2(-1.0f, 0.0f), overlay.c_str());
		ImGui::PopStyleColor();

		ImGui::Spacing();
	}

	auto DrawHealth(const Health& health, const ObjectTable* objects,
		const Resolved::Vitality::State::VitalityStore& vitalityStore) -> void
	{
		const ResolvedVitality* layout = nullptr;

		if (objects)
		{
			auto it = objects->find(health.Handle);
			if (it != objects->end())
			{
				ImGui::TextWrapped("%s", it->second.TagName.c_str());

				if (vitalityStore.IsFrozen())
				{
					layout = vitalityStore.GetResolvedVitality(it->second.TagName);
				}
			}
		}

		ImGui::Text("Handle: %s", HexFormater::Hex32(health.Handle).c_str());

		ImGui::Spacing();
		if (health.IsDead)
			ImGui::TextColored({ 1.0f, 0.35f, 0.35f, 1.0f }, "[ DEAD ]");
		else
			ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "[ ALIVE ]");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.0f }, "Damage Sections (%d)",
			static_cast<int>(health.SectionVitalities.size()));
		ImGui::Spacing();

		for (std::size_t i = 0; i < health.SectionVitalities.size(); ++i)
		{
			const Section* section = (layout && i < layout->Sections.size())
				? &layout->Sections[i] : nullptr;

			DrawSectionRow(i, health.SectionVitalities[i], section);
		}
	}
}

namespace UI::Overlay::System
{
	auto VitalitiesUI::Draw(const Tick& tick, std::uint32_t handle,
		const VitalityStore& vitalityStore) -> void
	{
		ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Health");
		ImGui::Separator();
		ImGui::Spacing();

		if (!tick.Healths)
		{
			ImGui::TextDisabled("No health data.");
			return;
		}

		auto it = tick.Healths->find(handle);
		if (it == tick.Healths->end())
		{
			ImGui::TextDisabled("This object has no health data.");
			return;
		}

		DrawHealth(it->second, tick.ObjectTable.get(), vitalityStore);
	}
}