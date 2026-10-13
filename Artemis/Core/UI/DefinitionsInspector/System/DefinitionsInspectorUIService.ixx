export module UI.DefinitionsInspector.System;

import Service.Settings.State;
import Resolved.Definitions.State;
import Export.Tick.Type;
import Export.Tick.State;
import Gui.Widget.System;
import Viewer.Selection.State;
import UI.DefinitionsInspector.Type;
import std;

export namespace UI::DefinitionsInspector::System
{
    // Draws the resolved definitions of the selected object and the tags linked from it.
    // note: Can also dump the definitions of a class to a file for the field usage report, and the object tags of the map
    // for the tag catalog of the semantization (Tools/seed_tags.py merges those files into Data/Tags/ObjectTags.xml).
    class DefinitionsInspectorUIService
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;

        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;

        using AliveObject = Export::Tick::Type::AliveObject;
        using TickStore = Export::Tick::State::TickStore;

        using InspectorGuiService = Gui::Widget::System::InspectorGuiService;

        using SelectionStore = Viewer::Selection::State::SelectionStore;

        using LinkedTag = UI::DefinitionsInspector::Type::LinkedTag;

    public:
        DefinitionsInspectorUIService(TickStore& tickStore, DefinitionsStore& definitionsStore,
            SelectionStore& selectionStore, SettingsStore& settingsStore) :
            m_TickStore(tickStore), m_DefinitionsStore(definitionsStore),
            m_SelectionStore(selectionStore), m_SettingsStore(settingsStore) {}
        ~DefinitionsInspectorUIService() = default;

        DefinitionsInspectorUIService(const DefinitionsInspectorUIService&) = delete;
        auto operator=(const DefinitionsInspectorUIService&) -> DefinitionsInspectorUIService& = delete;

        // Draws the inspector window content.
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
        std::string m_CatalogStatus{};

        // Draws the combo and the button that dump the definitions of a class, and the button that dumps the tag catalog.
        auto DrawDump() -> void;

        // Writes the definitions of a class to the storage folder as NDJSON.
        // return: The text that tells the result of the dump.
        auto DumpFourCC(const std::string& fourCC) -> std::string;

        // Writes the group, kind and multiplayer object kinds of every object tag of the map to TagCatalog/<map>.xml in the storage folder.
        // note: Every object tag of the map file, not only the living objects. The map is named after its scenario tag.
        // return: The text that tells the result of the dump.
        auto DumpCatalog() -> std::string;

        // Returns the selected object of the Viewer, nullptr when there is none.
        auto FindSelected() const -> const AliveObject*;

        // Checks whether the definitions store has the resolved definition of the tag.
        auto IsResolved(const std::string& fourCC, const std::string& tagName) const -> bool;

        // Draws the resolved definition of the tag by its class.
        auto DrawTag(const std::string& fourCC, const std::string& tagName, std::uint32_t ownerHandle) -> void;

        // Draws the links and the fields of a resolved definition.
        template <typename TResolved>
        auto DrawResolved(const TResolved* resolved, const std::string& fourCC,
            const std::string& tagName, std::uint32_t ownerHandle) -> void;

        // Draws the tags linked from the fields of a resolved definition.
        // note: Only the links that have a resolved definition are listed.
        template <typename TResolved>
        auto DrawLinks(const TResolved& resolved) -> void;
    };
}