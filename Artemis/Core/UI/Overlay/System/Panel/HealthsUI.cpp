module;

#include "External/imgui/imgui.h"

module UI.Overlay.System;
import :Vitalities;

import Resolved.World.Type;
import Resolved.Vitality.Type;
import Resolved.World.Type;
import UI.Format.System;
import UI.Widget.System;
import std;

namespace
{
	using Tick = Export::Tick::Type::Tick;
	using Health = Export::Tick::Type::Health;
	using ObjectTable = Export::Tick::Type::ObjectTable;
	using Section = Resolved::Vitality::Type::Vitality::Section;
	using SectionKind = Resolved::Vitality::Type::Vitality::Kind;
	using AimAnchor = Resolved::World::Type::ModelLink::Anchor;
	using AnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
	using ResolvedVitality = Resolved::Vitality::Type::Vitality::Vitality;
	using HexFormater = UI::Format::System::HexFormater;

	using PanelUIService = UI::Widget::System::PanelUIService;

	const ImVec4 k_Critical{ 1.0f, 0.5f, 0.3f, 1.0f };
	const ImVec4 k_Headshot{ 1.0f, 0.85f, 0.2f, 1.0f };
	const ImVec4 k_Shield{ 0.4f, 0.7f, 1.0f, 1.0f };
	const ImVec4 k_Destroys{ 0.85f, 0.45f, 0.85f, 1.0f };

	auto SourceName(AnchorSource source) -> const char*
	{
		switch (source)
		{
		case AnchorSource::ModelTarget:    return "target";
		case AnchorSource::HeadshotTarget: return "head target";
		case AnchorSource::CollRegion:     return "region";
		case AnchorSource::ObjectCenter:   return "center";
		default:                           return "none";
		}
	}

	auto DrawRoleTags(const Section& section) -> void
	{
		if (section.IsCritical)
		{
			ImGui::SameLine();
			ImGui::TextColored(k_Critical, "[critical]");
		}

		if (section.IsHeadshot)
		{
			ImGui::SameLine();
			ImGui::TextColored(k_Headshot, "[headshot]");
		}

		if (section.DestroysObject)
		{
			ImGui::SameLine();
			ImGui::TextColored(k_Destroys, "[destroys]");
		}

		if (section.Kind == SectionKind::Shield)
		{
			ImGui::SameLine();
			ImGui::TextColored(k_Shield, "[shield]");
		}
	}

	auto DrawAimLine(const AimAnchor& aim) -> void
	{
		if (aim.Source == AnchorSource::None)
		{
			ImGui::TextDisabled("  aim: none");
			return;
		}

		const std::string line = std::format(
			"  aim: {} | node {} | off {:.3f}, {:.3f}, {:.3f} | r {:.3f}",
			SourceName(aim.Source), aim.ModelNodeIndex,
			aim.LocalOffset.X, aim.LocalOffset.Y, aim.LocalOffset.Z, aim.Radius);

		ImGui::TextDisabled("%s", line.c_str());
	}

	// --- Vitality (static layout, Resolved) ---

	auto DrawVitalitySection(const Section& section) -> void
	{
		ImGui::Text("Section %u", static_cast<unsigned>(section.NameId));
		DrawRoleTags(section);

		if (section.CollRegion < 0 && section.Kind != SectionKind::Shield)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(no coll)");
		}

		DrawAimLine(section.Aim);
	}

	auto DrawVitality(const std::string* tagName, const ResolvedVitality* layout) -> void
	{
		ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Vitality");
		ImGui::Separator();
		ImGui::Spacing();

		if (tagName) ImGui::TextWrapped("%s", tagName->c_str());

		if (!layout)
		{
			ImGui::TextDisabled("No vitality layout.");
			return;
		}

		ImGui::Text("Max vitality: %.1f | Max shield: %.1f",
			layout->MaximumVitality, layout->MaximumShieldVitality);

		ImGui::Text("Kill: %zu | Headshot: %zu | Shield: %zu",
			layout->KillSections.size(), layout->HeadshotSections.size(),
			layout->ShieldSections.size());

		ImGui::TextDisabled("Object center: %s", SourceName(layout->ObjectCenter.Source));

		ImGui::Spacing();
		ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.0f }, "Sections (%d)",
			static_cast<int>(layout->Sections.size()));
		ImGui::Spacing();

		for (const Section& section : layout->Sections)
		{
			DrawVitalitySection(section);
			ImGui::Spacing();
		}
	}

	// --- Health (dynamic, per tick) ---

	auto DrawHealthRow(std::size_t index, float vitality, const Section* section) -> void
	{
		const bool isShield = section && section->Kind == SectionKind::Shield;
		const bool isCritical = section && section->IsCritical;

		if (section) ImGui::Text("Section %u", static_cast<unsigned>(section->NameId));
		else         ImGui::Text("Section #%zu", index);

		if (section) DrawRoleTags(*section);

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

	auto DrawHealth(const Health& health, const ResolvedVitality* layout) -> void
	{
		ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Health");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Handle: %s", HexFormater::Hex32(health.Handle).c_str());

		ImGui::Spacing();
		if (health.IsDead)
			ImGui::TextColored({ 1.0f, 0.35f, 0.35f, 1.0f }, "[ DEAD ]");
		else
			ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "[ ALIVE ]");

		PanelUIService::DrawSectionSeparator(nullptr);

		ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.0f }, "Damage Sections (%d)",
			static_cast<int>(health.SectionVitalities.size()));
		ImGui::Spacing();

		for (std::size_t i = 0; i < health.SectionVitalities.size(); ++i)
		{
			const Section* section = (layout && i < layout->Sections.size())
				? &layout->Sections[i] : nullptr;

			DrawHealthRow(i, health.SectionVitalities[i], section);
		}
	}
}

namespace UI::Overlay::System
{
	auto HealthsUI::Draw(const Tick& tick, std::uint32_t handle, const VitalityStore& vitalityStore) -> void
	{
		const std::string* tagName = nullptr;
		const ResolvedVitality* layout = nullptr;

		if (tick.ObjectTable)
		{
			auto it = tick.ObjectTable->find(handle);
			if (it != tick.ObjectTable->end())
			{
				tagName = &it->second.TagName;

				if (vitalityStore.IsFrozen())
				{
					layout = vitalityStore.GetResolvedVitality(it->second.TagName);
				}
			}
		}

		// 1. Vitality: what the object is made of (Resolved)
		DrawVitality(tagName, layout);

		ImGui::Spacing();
		ImGui::Spacing();

		// 2. Health: what the object has right now (per tick)
		if (!tick.Healths)
		{
			ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Health");
			ImGui::Separator();
			ImGui::TextDisabled("No health data.");
			return;
		}

		auto healthIt = tick.Healths->find(handle);
		if (healthIt == tick.Healths->end())
		{
			ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Health");
			ImGui::Separator();
			ImGui::TextDisabled("This object has no health data.");
			return;
		}

		DrawHealth(healthIt->second, layout);
	}
}