export module UI.DefinitionsInspector.System;

import Export.Tick.Type;
import Export.Tick.State;
import Resolved.Definitions.State;
import Service.Settings.State;
import Viewer.Selection.State;
import Gui.Widget.System;
import std;

export namespace UI::DefinitionsInspector::System
{
	class DefinitionsInspectorUIService
	{
	private:
		using AliveObject = Export::Tick::Type::AliveObject;
		using TickStore = Export::Tick::State::TickStore;
		using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
		using SettingsStore = Service::Settings::State::SettingsStore;
		using SelectionStore = Viewer::Selection::State::SelectionStore;
		using InspectorGuiService = Gui::Widget::System::InspectorGuiService;

		// TODO: Move to type
		struct LinkedTag
		{
			std::string FourCC{};
			std::string TagName{};
		};

	public:
		DefinitionsInspectorUIService(TickStore& tickStore, DefinitionsStore& definitionsStore,
			SelectionStore& selectionStore, SettingsStore& settingsStore) :
			m_TickStore(tickStore), m_DefinitionsStore(definitionsStore),
			m_SelectionStore(selectionStore), m_SettingsStore(settingsStore) {
		}
		~DefinitionsInspectorUIService() = default;

		DefinitionsInspectorUIService(const DefinitionsInspectorUIService&) = delete;
		DefinitionsInspectorUIService& operator=(const DefinitionsInspectorUIService&) = delete;

		auto Draw() -> void;

	private:
		TickStore& m_TickStore;
		DefinitionsStore& m_DefinitionsStore;
		SelectionStore& m_SelectionStore;
		SettingsStore& m_SettingsStore;

		InspectorGuiService m_Inspector{};

		std::optional<LinkedTag> m_Pinned{};
		std::optional<std::uint32_t> m_LastHandle{};

		int m_DumpFourCC{ 4 };
		std::string m_DumpStatus{};

		auto DrawDump() -> void;
		auto DumpFourCC(const std::string& fourCC) -> std::string;

		auto FindSelected() const -> const AliveObject*;
		auto HasResolved(const std::string& fourCC, const std::string& tagName) const -> bool;

		auto DrawTag(const std::string& fourCC, const std::string& tagName, std::uint32_t ownerHandle) -> void;

		template <typename TResolved>
		auto DrawResolved(const TResolved* resolved, const std::string& fourCC,
			const std::string& tagName, std::uint32_t ownerHandle) -> void;

		template <typename TResolved>
		auto DrawLinks(const TResolved& resolved) -> void;
	};
}