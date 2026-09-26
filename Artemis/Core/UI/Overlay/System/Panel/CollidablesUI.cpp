module;

#include "External/imgui/imgui.h"

module UI.Overlay.System;
import :Collidables;

import UI.Widget.System;
import std;

namespace
{
	using Tick = Export::Tick::Type::Tick;
	using Collidable = Export::Tick::Type::Collidable;

	using PanelUIService = UI::Widget::System::PanelUIService;

	constexpr ImVec4 k_AlertColor{ 1.0f, 0.4f, 0.4f, 1.0f };

	auto DrawCollidable(const Collidable& instance) -> void
	{
		PanelUIService::DrawHeader(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Collidable",
			instance.TagName, instance.Handle);

		ImGui::Spacing();

		PanelUIService::DrawVec3("Position:", instance.Position);
		PanelUIService::DrawVec3("Forward:", instance.Forward);
		PanelUIService::DrawVec3("Up:", instance.Up);

		PanelUIService::DrawSectionSeparator();

		const auto& mesh = instance.WorldMesh;

		ImGui::TextDisabled("World mesh:");
		ImGui::Text("Triangles: %d", static_cast<int>(mesh.Triangles.size()));
		ImGui::Text("Node: %d", static_cast<int>(mesh.NodeIndex));
		ImGui::Text("Region: %d", static_cast<int>(mesh.RegionIndex));
		ImGui::Text("Permutation: %d", static_cast<int>(mesh.PermutationIndex));

		ImGui::Spacing();

		ImGui::TextDisabled("Bounds (model-space):");
		PanelUIService::DrawVec3("Min:", mesh.LocalMin);
		PanelUIService::DrawVec3("Max:", mesh.LocalMax);

		PanelUIService::DrawSectionSeparator();

		PanelUIService::DrawBoolBadge("Ancestor dead:", instance.AncestorDead, k_AlertColor);
		PanelUIService::DrawBoolBadge("Destroyed geometry:", instance.HasDestroyedGeometry, k_AlertColor);
	}
}

namespace UI::Overlay::System
{
	auto CollidablesUI::Draw(const Tick& tick, std::uint32_t handle) -> void
	{
		if (!tick.Collidables)
		{
			ImGui::TextDisabled("No collidable data.");
			return;
		}

		for (const Collidable& instance : *tick.Collidables)
		{
			if (instance.Handle == handle)
			{
				DrawCollidable(instance);
				return;
			}
		}

		ImGui::TextDisabled("Selected object is not a collidable.");
	}
}