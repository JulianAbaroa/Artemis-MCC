export module UI.ObjectTable.System;

import Export.Tick.Type;
import Export.Tick.State;
import UI.ObjectTable.State;
import Gui.Widget.System;
import std;

export namespace UI::ObjectTable::System
{
	class ObjectTableUIService
	{
	private:
		using AliveObject = Export::Tick::Type::AliveObject;

		using TickStore = Export::Tick::State::TickStore;
		using ObjectTableUIStore = UI::ObjectTable::State::ObjectTableUIStore;
		using SearchFilterGuiService = Gui::Widget::System::SearchFilterGuiService;
		using CopyableFieldGuiService = Gui::Widget::System::CopyableFieldGuiService;
		using ResponsiveCardGuiService = Gui::Widget::System::ResponsiveCardGuiService;

	public:
		ObjectTableUIService(TickStore& tickStore, ObjectTableUIStore& objectTableUIStore) :
			m_TickStore(tickStore), m_ObjectTableUIStore(objectTableUIStore) {
		}
		~ObjectTableUIService() = default;

		ObjectTableUIService(const ObjectTableUIService&) = delete;
		ObjectTableUIService& operator=(const ObjectTableUIService&) = delete;

		auto Draw() -> void;

	private:
		TickStore& m_TickStore;
		ObjectTableUIStore& m_ObjectTableUIStore;

		SearchFilterGuiService m_SearchFilter{};
		CopyableFieldGuiService m_CopyableField{};

		auto RefreshSnapshot() -> void;
		auto PassesFilter(const AliveObject& object) const -> bool;

		auto DrawObjectCard(const AliveObject& object) -> void;
		auto DrawCardHeader(const AliveObject& object) -> void;
		auto DrawCardFields(const AliveObject& object) -> void;
	};
}