module;

#include "External/imgui/imgui.h"

export module UI.Widget.System:Panel;

import Common.Math.Type;
import std;

export namespace UI::Widget::System
{
	class PanelUIService
	{
	private:
		using Vec3 = Common::Math::Type::Vec3;

	public:
		PanelUIService() = default;
		~PanelUIService() = default;

		static auto DrawVec3(const char* label, const Vec3& value) -> void;

		static auto DrawBoolBadge(const char* label, bool value, const ImVec4& trueColor) -> void;

		static auto DrawSectionSeparator(const char* title = nullptr) -> void;

		static auto DrawHeader(const ImVec4& color, const char* title,
			const std::string& tagName, std::uint32_t handle) -> void;
	};
}