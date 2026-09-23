export module UI.Widget.System:HandleDrawer;

import :CopyableField;
import std;

export namespace UI::Widget::System
{
	class HandleDrawerUIService
	{
	private:
		using CopyableFieldUIService = UI::Widget::System::CopyableFieldUIService;

	public:
		HandleDrawerUIService() = default;
		~HandleDrawerUIService() = default;

		static auto DrawU32(const char* label, std::uint32_t handle,
			std::uint32_t ownerHandle, CopyableFieldUIService& copyableField) -> void;
	};
}