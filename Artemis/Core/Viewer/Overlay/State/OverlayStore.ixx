export module Viewer.Overlay.State;

import Viewer.Overlay.Type;
import std;

export namespace Viewer::Overlay::State
{
	class OverlayStore
	{
	private:
		using Mode = Viewer::Overlay::Type::Mode;

	public:
		OverlayStore() = default;
		~OverlayStore() = default;

		auto GetMode() const -> Mode;
		auto NextMode() -> void;
		auto PreviousMode() -> void;
		auto ResetMode() -> void;

		auto GetPage() const -> int;
		auto NextPage() -> void;
		auto PreviousPage() -> void;
		auto ResetPage() -> void;
		auto ClampPage(int maxPage) -> void;

	private:
		std::atomic<std::uint8_t> m_Mode{ static_cast<std::uint8_t>(Mode::Default) };
		std::atomic<int> m_Page{ 0 };
	};
}