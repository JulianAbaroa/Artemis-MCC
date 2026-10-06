export module UI.ObjectTable.System;

import Export.Tick.Type;
import Export.Tick.State;
import Gui.Widget.System;
import UI.ObjectTable.Type;
import UI.ObjectTable.State;
import std;

export namespace UI::ObjectTable::System
{
    // Draws the live objects as cards grouped by class with a search filter.
    class ObjectTableUIService
    {
    private:
        using AliveObject = Export::Tick::Type::AliveObject;

        using TickStore = Export::Tick::State::TickStore;

        using SearchFilterGuiService = Gui::Widget::System::SearchFilterGuiService;
        using CopyableFieldGuiService = Gui::Widget::System::CopyableFieldGuiService;
        using ResponsiveCardGuiService = Gui::Widget::System::ResponsiveCardGuiService;

        using Groups = UI::ObjectTable::Type::Groups;
        using ObjectTableUIStore = UI::ObjectTable::State::ObjectTableUIStore;

    public:
        ObjectTableUIService(TickStore& tickStore, ObjectTableUIStore& objectTableUIStore) :
            m_TickStore(tickStore), m_ObjectTableUIStore(objectTableUIStore) {}
        ~ObjectTableUIService() = default;

        ObjectTableUIService(const ObjectTableUIService&) = delete;
        auto operator=(const ObjectTableUIService&) -> ObjectTableUIService& = delete;

        // Draws the object table window content.
        auto Draw() -> void;

    private:
        TickStore& m_TickStore;
        ObjectTableUIStore& m_ObjectTableUIStore;

        SearchFilterGuiService m_SearchFilter{};
        CopyableFieldGuiService m_CopyableField{};

        // Rebuilds the groups when the tick has a new object table.
        auto RefreshSnapshot() -> void;

        // Checks whether the object matches the search text.
        auto PassesFilter(const AliveObject& object) const -> bool;

        // Draws the card of an object.
        auto DrawObjectCard(const AliveObject& object) -> void;

        // Draws the tag name of the card.
        auto DrawCardHeader(const AliveObject& object) -> void;

        // Draws the copyable fields of the card.
        auto DrawCardFields(const AliveObject& object) -> void;
    };
}