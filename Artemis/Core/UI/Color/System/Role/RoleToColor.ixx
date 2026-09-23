module;

#include "External/imgui/imgui.h"

export module UI.Color.System:Role;

import Relations.Classifier.Type;

export namespace UI::Color::System
{
	class RoleToColor
	{
	private:
		using Role = Relations::Classifier::Type::Role;

	public:
		RoleToColor() = default;
		~RoleToColor() = default;

		static auto FromRole(Role role) -> ImVec4;
	};
}