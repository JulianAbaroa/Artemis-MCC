export module UI.Capture.System;

import Common.Log.System;
import Service.Settings.State;
import Platform.Memory.System;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Datum.Object.Type;
import Datum.Object.State;
import Export.Tick.Type;
import Export.Tick.State;
import UI.Capture.Type;
import UI.Capture.State;
import std;

export namespace UI::Capture::System
{
    // Copies the datum of the local biped, its linked objects and the pinned objects to a session file, cut by component (D81).
    // Each object also gets its Havok component and rigid bodies, the shapes of its physics model (phmo), and each capture the layer
    // table of the Havok collision filter (A10). Each object also gets its collision model (coll), and each capture the flags of the
    // projectile collision test (A13).
    // With the engine option it also copies the engine globals of the thread context (A5: match, scores, clock, respawn).
    // note: A development tool (D72). The datum comes from the copy DatumService already makes each tick. The Havok lines are read live
    // from the render thread, through the safe reader, at fixed RVAs of haloreach.dll (A10).
    class CaptureUIService
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
        using Hlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using Phmo = Resolved::Definitions::Type::Phmo::Phmo;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;

        using Snapshot = Datum::Object::Type::Raw::Snapshot;
        using DatumStore = Datum::Object::State::DatumStore;

        using Tick = Export::Tick::Type::Tick;
        using TickStore = Export::Tick::State::TickStore;

        using Target = UI::Capture::Type::Target;
        using HavokFilter = UI::Capture::Type::HavokFilter;
        using HavokComponent = UI::Capture::Type::HavokComponent;
        using CaptureUIStore = UI::Capture::State::CaptureUIStore;

    public:
        CaptureUIService(Log& log, SettingsStore& settingsStore, MemoryReaderService& memoryReaderService,
            DefinitionsStore& definitionsStore, DatumStore& datumStore, TickStore& tickStore, CaptureUIStore& captureUIStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_MemoryReaderService(memoryReaderService),
            m_DefinitionsStore(definitionsStore), m_DatumStore(datumStore), m_TickStore(tickStore), m_CaptureUIStore(captureUIStore) {}
        ~CaptureUIService() = default;

        CaptureUIService(const CaptureUIService&) = delete;
        auto operator=(const CaptureUIService&) -> CaptureUIService& = delete;

        // Runs the capture asked by the hotkey or the button.
        // note: Called from the render thread on every frame, also while the window is hidden.
        auto Update() -> void;

        // Draws the capture window content.
        auto Draw() -> void;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        MemoryReaderService& m_MemoryReaderService;
        DefinitionsStore& m_DefinitionsStore;
        DatumStore& m_DatumStore;
        TickStore& m_TickStore;
        CaptureUIStore& m_CaptureUIStore;

        // Appends one capture to the session file, keeps its diff against the previous capture and logs a summary of it.
        // return: The status text shown in the window.
        auto Capture() -> std::string;

        // Lists the objects of a capture, each handle once: the local biped, every other biped if asked, the pinned objects,
        // and the weapons, armor ability and vehicle of each of those bipeds.
        auto CollectTargets(const Tick& tick, const Snapshot& snapshot) const -> std::vector<Target>;

        // Reads the Havok world and the layer table of its collision filter.
        // return: Nothing if haloreach.dll, the world or the filter is not found.
        auto ReadHavokFilter() -> std::optional<HavokFilter>;

        // Reads one Havok component and its rigid bodies.
        // param handle: Value of obje 0xCC.
        // return: Nothing if the handle is empty or the component data array is not found.
        auto ReadHavokComponent(std::uint32_t handle) -> std::optional<HavokComponent>;

        // Writes the engine globals reached from the thread context (A5) as objects of the session file, so the analysis script
        // splits and diffs them like a datum: the thread context, the game globals, the time globals, the game engine globals and
        // the player table. Each block is read page by page and ends at the first page that cannot be read.
        // param threadContext: Engine thread context kept by the object table locator, 0 if not found yet.
        // return: How many blocks were written.
        auto WriteEngineBlocks(std::ofstream& file, std::uintptr_t threadContext) -> std::size_t;

        // Copies the whole game state allocation (the block every global of the thread context points into) to a binary file next to
        // the session file, one per capture, and writes a gs-dump line with its name, base and size (A5, A8).
        // note: Pages that are not committed or cannot be read stay zero. Tools/analyze_gamestate.py searches the files by label.
        // return: Bytes written, 0 if the allocation is not found.
        auto WriteGameState(std::ofstream& file, std::uintptr_t threadContext, std::uint32_t index) -> std::size_t;

        // Copies up to maximum bytes from address, stopping at the first page that cannot be read.
        auto ReadPages(std::uintptr_t address, std::size_t maximum) -> std::vector<std::uint8_t>;

        // Reads the flags of the collision test projectiles trace with (low half: collision flags, high half: object types).
        // return: Nothing if haloreach.dll is not loaded or the read fails.
        auto ReadCollisionTestFlags() -> std::optional<std::uint64_t>;

        // Finds the model (hlmt) of an object through its definition.
        // return: Null if the definitions are not built yet or a link of the chain is missing.
        auto FindHlmt(ObjectKind kind, const std::string& tagName) const -> const Hlmt*;

        // Finds the physics model of an object through its definition and its model (hlmt).
        // return: Null if the definitions are not built yet or a link of the chain is missing.
        auto FindPhmo(ObjectKind kind, const std::string& tagName) const -> const Phmo*;

        // Opens the session file to append. The first capture of a session creates it in the Captures folder of the storage folder.
        // return: The open file, or nothing if the storage folder is not set or the file cannot be opened.
        auto OpenSessionFile() -> std::optional<std::ofstream>;

        // Draws the label, the options and the buttons.
        auto DrawControls() -> void;

        // Draws the pinned objects with a button to unpin each one.
        auto DrawPinned() -> void;

        // Draws the byte runs that changed in the last capture, filtered by component.
        auto DrawChanges() -> void;
    };
}