export module DevTools.Workbench.ObjectTable.System;

import Engine.Capture.State;
import Blam.Runtime.Object.Type;
import Bridge.Codec.Tick.Type;
import Bridge.Codec.Tick.State;
import DevTools.Gui.Widget.System;
import DevTools.Workbench.ObjectTable.Type;
import DevTools.Workbench.ObjectTable.State;
import DevTools.Workbench.Capture.State;
import std;

export namespace DevTools::Workbench::ObjectTable::System
{
    // Draws the live objects as cards grouped by class with a search filter.
    class ObjectTableUIService
    {
    private:
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;

        using CaptureStore = Engine::Capture::State::CaptureStore;

        using TickStore = Bridge::Codec::Tick::State::TickStore;

        using SearchFilterGuiService = DevTools::Gui::Widget::System::SearchFilterGuiService;
        using CopyableFieldGuiService = DevTools::Gui::Widget::System::CopyableFieldGuiService;
        using ResponsiveCardGuiService = DevTools::Gui::Widget::System::ResponsiveCardGuiService;

        using Groups = DevTools::Workbench::ObjectTable::Type::Groups;
        using ObjectTableUIStore = DevTools::Workbench::ObjectTable::State::ObjectTableUIStore;
        using CaptureUIStore = DevTools::Workbench::Capture::State::CaptureUIStore;

    public:
        ObjectTableUIService(CaptureStore& captureStore, TickStore& tickStore, ObjectTableUIStore& objectTableUIStore, CaptureUIStore& captureUIStore) :
            m_CaptureStore(captureStore), m_TickStore(tickStore), m_ObjectTableUIStore(objectTableUIStore), m_CaptureUIStore(captureUIStore) {}
        ~ObjectTableUIService() = default;

        ObjectTableUIService(const ObjectTableUIService&) = delete;
        auto operator=(const ObjectTableUIService&) -> ObjectTableUIService& = delete;

        // Draws the object table window content.
        auto Draw() -> void;

    private:
        CaptureStore& m_CaptureStore;
        TickStore& m_TickStore;
        ObjectTableUIStore& m_ObjectTableUIStore;
        CaptureUIStore& m_CaptureUIStore;

        SearchFilterGuiService m_SearchFilter{};
        CopyableFieldGuiService m_CopyableField{};

        // Rebuilds the groups when the tick has a new object table and takes the published snapshot the addresses come from (D196).
        auto RefreshSnapshot() -> void;

        // return: Address of the object in the game process, or 0 without a published snapshot or when the handle is not in it (D196).
        auto AddressOf(const AliveObject& object) const -> std::uintptr_t;

        // Checks whether the object matches the search text.
        auto PassesFilter(const AliveObject& object) const -> bool;

        // Draws the card of an object.
        auto DrawObjectCard(const AliveObject& object) -> void;

        // Draws the pin box of the captures and the tag name of the card.
        auto DrawCardHeader(const AliveObject& object) -> void;

        // Draws the copyable fields of the card.
        auto DrawCardFields(const AliveObject& object) -> void;
    };
}