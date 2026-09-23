module;

#include "External/imgui/imgui.h"

module UI.Overlay.System;
import :Collidables;

import Common.Math.Type;
import UI.Format.System;
import std;

namespace
{
	using Tick = Export::Tick::Type::Tick;
	using Collidable = Export::Tick::Type::Collidable;
	using Vec3 = Common::Math::Type::Vec3;
	using HexFormater = UI::Format::System::HexFormater;

	auto DrawVec3(const char* label, const Vec3& value) -> void
	{
		ImGui::Text("%s %.2f, %.2f, %.2f", label, value.X, value.Y, value.Z);
	}

	auto DrawFlag(const char* label, bool value) -> void
	{
		ImGui::Text("%s", label);
		ImGui::SameLine();

		if (value) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "yes");
		else       ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "no");
	}

	auto DrawCollidable(const Collidable& instance) -> void
	{
		ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Collidable");

		ImGui::Separator();

		ImGui::Text("%s", instance.TagName.c_str());
		ImGui::Text("Handle: %s", HexFormater::Hex32(instance.Handle).c_str());

		ImGui::Spacing();

		DrawVec3("Position:", instance.Position);
		DrawVec3("Forward:", instance.Forward);
		DrawVec3("Up:", instance.Up);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		const auto& mesh = instance.WorldMesh;

		ImGui::TextDisabled("World mesh:");
		ImGui::Text("Triangles: %d", static_cast<int>(mesh.Triangles.size()));
		ImGui::Text("Node: %d", static_cast<int>(mesh.NodeIndex));
		ImGui::Text("Region: %d", static_cast<int>(mesh.RegionIndex));
		ImGui::Text("Permutation: %d", static_cast<int>(mesh.PermutationIndex));

		ImGui::Spacing();

		ImGui::TextDisabled("Bounds (model-space):");
		DrawVec3("Min:", mesh.LocalMin);
		DrawVec3("Max:", mesh.LocalMax);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		DrawFlag("Ancestor dead:", instance.AncestorDead);
		DrawFlag("Destroyed geometry:", instance.HasDestroyedGeometry);
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