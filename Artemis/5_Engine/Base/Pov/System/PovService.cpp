module;

#include <windows.h>

module Engine.Base.Pov.System;

import Common.Engine.Type;
import Engine.Base.Hook.System;
import Engine.Base.Memory.Type;
import Blam.Layout.Interaction.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import std;

namespace
{
    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_SaltShift;

    using Blam::Layout::Object::Type::Constant::k_TelemetryDataPlayerTableOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMappingOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;
    using Blam::Layout::Player::Type::Mapping::k_ControllerPlayer;
    using Blam::Layout::Player::Type::Mapping::k_EntryStride;
    using Blam::Layout::Player::Type::Mapping::k_InputUserPlayer;
    using Blam::Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Blam::Layout::Player::Type::Offset::k_Handle;
    using Blam::Layout::Player::Type::Offset::k_Loadout;
    using Blam::Layout::Player::Type::Offset::k_LoadoutAcknowledged;
    using Blam::Layout::Player::Type::Size::k_Base;

    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserPrologue;
    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserRva;
    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserWindow;
    using Engine::Base::Pov::System::ShouldAckLoadout;
    using Engine::Base::Pov::Type::PovAssignFunction;
    using Engine::Base::Pov::Type::PovRequest;
    using Engine::Base::Pov::Type::PovStatus;
    using Engine::Base::Pov::Type::k_PovReleasePoll;
    using Engine::Base::Pov::Type::k_PovRequestMaxAge;
    using Engine::Base::Pov::Type::k_PovUser;

    using Blam::Layout::Interaction::Type::Offset::k_OutputUserStride;
    using Blam::Layout::Object::Type::Bipd::Offset::k_BipedFlags;
    using Blam::Layout::Object::Type::Constant::k_ControlGlobalsOutputUserTableOffset;
    using Blam::Layout::Object::Type::Constant::k_EntryAddressOffset;
    using Blam::Layout::Object::Type::Constant::k_EntrySize;
    using Blam::Layout::Object::Type::Constant::k_ObjectDataTableOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextControlGlobalsOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameEngineOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextObjectDataOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;
    using Blam::Layout::Player::Type::Offset::k_AliveBipedHandle;

    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Base::Memory::Type::Signature::k_ProbeControlByte;
    using Engine::Base::Memory::Type::Signature::k_ProbeGameEngineViewStride;
    using Engine::Base::Memory::Type::Signature::k_ProbeGameEngineViewValue;
    using Engine::Base::Memory::Type::Signature::k_ProbeMappingInputUnit;
    using Engine::Base::Memory::Type::Signature::k_ProbeMappingViewCount;
    using Engine::Base::Memory::Type::Signature::k_ProbeMappingViewMask;
    using Engine::Base::Memory::Type::Signature::k_ProbeMappingViewUnit;
    using Engine::Base::Memory::Type::Signature::k_ProbeMaskEntries;
    using Engine::Base::Memory::Type::Signature::k_ProbeThreadContextUi;
    using Engine::Base::Memory::Type::Signature::k_ProbeUiEntryStride;
    using Engine::Base::Memory::Type::Signature::k_ProbeUiFields;
    using Engine::Base::Memory::Type::Signature::k_ProbeUiViewBias;
    using Engine::Base::Memory::Type::Signature::k_ProbeControllerGlobalsRva;
    using Engine::Base::Memory::Type::Signature::k_ProbeControllerStride;
    using Engine::Base::Memory::Type::Signature::k_ProbePlayerRespawnElapsed;
    using Engine::Base::Memory::Type::Signature::k_ProbePlayerRespawnSeconds;
    using Engine::Base::Pov::Type::PovProbeCursor;
    using Engine::Base::Pov::Type::PovProbeDrain;
    using Engine::Base::Pov::Type::PovProbeEvent;
    using Engine::Base::Pov::Type::PovProbeGoneScreen;
    using Engine::Base::Pov::Type::PovProbeKind;
    using Engine::Base::Pov::Type::PovProbePlayerCopy;
    using Engine::Base::Pov::Type::PovProbePlayerState;
    using Engine::Base::Pov::Type::PovProbeReason;
    using Engine::Base::Pov::Type::PovProbeScreenField;
    using Blam::Layout::Player::Type::Offset::k_RespawnTicks;

    using Engine::Base::Pov::Type::k_PovProbeControllers;
    using Engine::Base::Pov::Type::k_PovProbeDiffWords;
    using Engine::Base::Pov::Type::k_PovProbeMaxLinesPerSecond;
    using Engine::Base::Pov::Type::k_PovProbePlayerRoles;
    using Engine::Base::Pov::Type::k_PovProbePlayerWords;
    using Engine::Base::Pov::Type::k_PovProbeScreenFieldCount;
    using Engine::Base::Pov::Type::k_PovProbeScreenSlots;
    using Engine::Base::Pov::Type::k_PovProbeUnreadable;
    using Engine::Base::Pov::Type::k_PovProbeViews;

    using std::chrono::seconds;
    using std::chrono::steady_clock;

    // TEMPORAL VW.9a.x (the probe). Every read goes through MemoryReaderService: a failed one gives nothing and the field is written as "??".

    // return: The value at base + offset, or nothing if there is no base or the read fails.
    template <typename T>
    auto ReadAt(MemoryReaderService& reader, std::optional<std::uintptr_t> base, std::uintptr_t offset) -> std::optional<T>
    {
        if (!base || *base == 0) return std::nullopt;

        T value{};
        if (!reader.ReadRaw(*base + offset, &value, sizeof(T))) return std::nullopt;
        return value;
    }

    // return: The value in hex, or "??" when it was not read.
    auto HexText(std::optional<std::uint64_t> value) -> std::string
    {
        return value ? std::format("{:X}", *value) : std::string{ "??" };
    }

    // return: The count uint32 at base + first, base + first + stride, ..., in hex and separated by commas.
    auto JoinU32(MemoryReaderService& reader, std::optional<std::uintptr_t> base, std::uintptr_t first, std::uint32_t count, std::uintptr_t stride)
        -> std::string
    {
        std::string text{};
        for (std::uint32_t index{ 0 }; index < count; ++index)
        {
            if (index > 0) text += ',';
            text += HexText(ReadAt<std::uint32_t>(reader, base, first + (index * stride)));
        }

        return text;
    }

    // return: The player indexes that have a view, as index:mask pairs separated by commas ("-" when none, "??" when it cannot be read).
    auto MaskText(MemoryReaderService& reader, std::optional<std::uintptr_t> mapping) -> std::string
    {
        std::string text{};
        for (std::uint32_t index{ 0 }; index < k_ProbeMaskEntries; ++index)
        {
            const std::optional<std::uint8_t> mask{ ReadAt<std::uint8_t>(reader, mapping, k_ProbeMappingViewMask + index) };
            if (!mask) return "??";
            if (*mask == 0) continue;

            if (!text.empty()) text += ',';
            text += std::format("{}:{:X}", index, *mask);
        }

        return text.empty() ? std::string{ "-" } : text;
    }

    // return: The base of the player table of the thread context, or nothing.
    auto ReadPlayerTable(MemoryReaderService& reader, std::uintptr_t threadContext) -> std::optional<std::uintptr_t>
    {
        const std::optional<std::uintptr_t> telemetry{ ReadAt<std::uintptr_t>(reader, threadContext, k_ThreadContextTelemetryDataOffset) };
        return ReadAt<std::uintptr_t>(reader, telemetry, k_TelemetryDataPlayerTableOffset);
    }

    // return: The address of the datum of a player in the table, or nothing for no handle, an index past the table or no table.
    auto PlayerAddress(std::optional<std::uintptr_t> players, std::uint32_t handle) -> std::optional<std::uintptr_t>
    {
        if (!players || *players == 0 || handle == k_NoHandle) return std::nullopt;

        const std::uint32_t index{ handle & k_HandleIndexMask };
        if (index >= k_MaxPlayers) return std::nullopt;

        return *players + (static_cast<std::uintptr_t>(index) * k_Base);
    }

    // return: The respawn state of a player: unit, ticks left, ticks elapsed and seconds of the wait (nothing for what cannot be read).
    auto ReadPlayerState(MemoryReaderService& reader, std::optional<std::uintptr_t> players, std::uint32_t handle) -> PovProbePlayerState
    {
        PovProbePlayerState state{};
        state.Handle = handle;

        const std::optional<std::uintptr_t> address{ PlayerAddress(players, handle) };
        state.Unit = ReadAt<std::uint32_t>(reader, address, k_AliveBipedHandle);
        state.RespawnTicks = ReadAt<std::uint16_t>(reader, address, k_RespawnTicks);
        state.RespawnElapsed = ReadAt<std::uint16_t>(reader, address, k_ProbePlayerRespawnElapsed);
        state.RespawnSeconds = ReadAt<std::uint16_t>(reader, address, k_ProbePlayerRespawnSeconds);
        return state;
    }

    // return: Whether the player exists and is known to have no unit.
    auto HasNoUnit(const PovProbePlayerState& state) -> bool
    {
        return state.Handle != k_NoHandle && state.Unit && *state.Unit == k_NoHandle;
    }

    // return: Why the probe must be on, none when it must not.
    auto DecideReason(bool isRequested, const PovProbePlayerState& view, const PovProbePlayerState& local) -> PovProbeReason
    {
        if (isRequested) return PovProbeReason::Pov;
        if (HasNoUnit(view)) return PovProbeReason::ViewDead;
        if (HasNoUnit(local)) return PovProbeReason::LocalDead;
        return PovProbeReason::None;
    }

    // return: The reason as the log says it.
    auto ReasonText(PovProbeReason reason) -> const char*
    {
        switch (reason)
        {
        case PovProbeReason::Pov: return "pov";
        case PovProbeReason::ViewDead: return "view-dead";
        case PovProbeReason::LocalDead: return "local-dead";
        case PovProbeReason::None: return "none";
        }

        return "none";
    }

    // return: Whether a counter that moves with the ticks that passed (down when isCountdown, up otherwise) did only that, or did not move.
    auto IsNormalAdvance(std::optional<std::uint16_t> previous, std::optional<std::uint16_t> current, std::uint32_t tickDelta, bool isCountdown) -> bool
    {
        if (previous == current) return true;
        if (!previous || !current) return false;

        if (isCountdown)
        {
            const std::uint32_t expected{ *previous > tickDelta ? *previous - tickDelta : 0 };
            return *current == expected;
        }

        return *current == static_cast<std::uint16_t>(*previous + tickDelta);
    }

    // return: Whether the respawn state changed in something besides the normal advance of the ticks left and the ticks elapsed.
    auto IsResponseChanged(const PovProbePlayerState& previous, const PovProbePlayerState& current, std::uint32_t tickDelta) -> bool
    {
        return previous.Handle != current.Handle || previous.Unit != current.Unit || previous.RespawnSeconds != current.RespawnSeconds ||
            !IsNormalAdvance(previous.RespawnTicks, current.RespawnTicks, tickDelta, true) ||
            !IsNormalAdvance(previous.RespawnElapsed, current.RespawnElapsed, tickDelta, false);
    }

    // return: The player as the "resp" line writes it: handle, unit, ticks left, ticks elapsed and seconds, all in hex.
    auto ResponseText(const char* name, const PovProbePlayerState& state) -> std::string
    {
        return std::format("{}={:X} u={} 1bc={} 1cc={} 1c8={}", name, state.Handle, HexText(state.Unit), HexText(state.RespawnTicks),
            HexText(state.RespawnElapsed), HexText(state.RespawnSeconds));
    }

    // return: The state word of every controller of the game module, in hex and separated by commas.
    auto ControllersText(MemoryReaderService& reader, std::uint64_t moduleBase) -> std::string
    {
        if (moduleBase == 0) return "??";
        return JoinU32(reader, static_cast<std::uintptr_t>(moduleBase + k_ProbeControllerGlobalsRva), 0, k_PovProbeControllers, k_ProbeControllerStride);
    }

    // return: The player diff of the words of two copies of a datum, without the tick and the frame: "off:old>new" in hex, up to the limit and then
    // "+N", empty when nothing changed. The two counters of the respawn (ticks left and elapsed) are left out when they moved by the ticks that passed.
    auto DiffText(const PovProbePlayerCopy& previous, const std::array<std::uint32_t, k_PovProbePlayerWords>& current, std::uint32_t tickDelta)
        -> std::string
    {
        std::string text{};
        std::size_t count{ 0 };
        for (std::size_t index{ 0 }; index < current.size(); ++index)
        {
            const std::uint32_t before{ previous.Words[index] };
            const std::uint32_t after{ current[index] };
            if (before == after) continue;

            const std::size_t offset{ index * sizeof(std::uint32_t) };
            if (offset == k_RespawnTicks || offset == k_ProbePlayerRespawnElapsed)
            {
                const bool isSameHigh{ (before >> 16) == (after >> 16) };
                const std::uint32_t low{ before & 0xFFFF };
                const std::uint32_t lowNow{ after & 0xFFFF };
                const std::uint32_t moved{ low > lowNow ? low - lowNow : lowNow - low };
                if (isSameHigh && moved == (std::max)(tickDelta, 1u)) continue;
            }

            if (count < k_PovProbeDiffWords) text += std::format("{}0x{:X}:0x{:X}>0x{:X}", text.empty() ? "" : " ", offset, before, after);
            ++count;
        }

        if (count > k_PovProbeDiffWords) text += std::format(" +{}", count - k_PovProbeDiffWords);
        return text;
    }
    // return: The snapshot of the state of the views, in one line without the tick and the frame. Fields are the ones of VW9ax_Sonda.txt.
    auto BuildSnapshot(MemoryReaderService& reader, std::uintptr_t threadContext, const PovStatus& status, std::uint32_t requested) -> std::string
    {
        const std::optional<std::uintptr_t> mapping{ ReadAt<std::uintptr_t>(reader, threadContext, k_ThreadContextMappingOffset) };
        const std::optional<std::uintptr_t> control{ ReadAt<std::uintptr_t>(reader, threadContext, k_ThreadContextControlGlobalsOffset) };
        const std::optional<std::uintptr_t> ui{ ReadAt<std::uintptr_t>(reader, threadContext, k_ProbeThreadContextUi) };
        const std::optional<std::uintptr_t> gameEngine{ ReadAt<std::uintptr_t>(reader, threadContext, k_ThreadContextGameEngineOffset) };

        // The unit of the requested player and the flags byte of that unit (bit 4 is the one the theater uses to know it can be watched).
        const std::optional<std::uintptr_t> players{ ReadPlayerTable(reader, threadContext) };
        const std::optional<std::uintptr_t> objectData{ ReadAt<std::uintptr_t>(reader, threadContext, k_ThreadContextObjectDataOffset) };
        const std::optional<std::uintptr_t> objects{ ReadAt<std::uintptr_t>(reader, objectData, k_ObjectDataTableOffset) };

        std::string unitText{ "-" };
        std::string flagsText{ "-" };
        const std::uint32_t playerIndex{ requested & k_HandleIndexMask };

        if (requested != k_NoHandle && playerIndex < k_MaxPlayers)
        {
            const std::optional<std::uint32_t> unit{ ReadAt<std::uint32_t>(reader, players, (playerIndex * k_Base) + k_AliveBipedHandle) };
            unitText = HexText(unit);

            if (unit && *unit != k_NoHandle)
            {
                const std::optional<std::uintptr_t> datum{ ReadAt<std::uintptr_t>(reader, objects, ((*unit & k_HandleIndexMask) * k_EntrySize) + k_EntryAddressOffset) };
                flagsText = HexText(ReadAt<std::uint8_t>(reader, datum, k_BipedFlags));
            }
            else if (unit)
            {
                flagsText = "none";
            }
        }

        const std::uintptr_t uiFirst{ (k_ProbeUiViewBias + k_PovUser) * k_ProbeUiEntryStride };

        return std::format("map b8={} c8={} 04={} 14={} cnt={} mask={} ctl680={} 94b={} ui0={} 35c4={} req={:X} unit={} f9d8={} calls={} re={}",
            JoinU32(reader, mapping, k_OutputUserPlayer, k_PovProbeViews, k_EntryStride),
            JoinU32(reader, mapping, k_ProbeMappingViewUnit, k_PovProbeViews, k_EntryStride),
            JoinU32(reader, mapping, k_InputUserPlayer, k_PovProbeViews, k_EntryStride),
            JoinU32(reader, mapping, k_ProbeMappingInputUnit, k_PovProbeViews, k_EntryStride),
            HexText(ReadAt<std::uint16_t>(reader, mapping, k_ProbeMappingViewCount)),
            MaskText(reader, mapping),
            JoinU32(reader, control, k_ControlGlobalsOutputUserTableOffset, k_PovProbeViews, k_OutputUserStride),
            HexText(ReadAt<std::uint8_t>(reader, control, k_ProbeControlByte)),
            JoinU32(reader, ui, uiFirst, k_ProbeUiFields, sizeof(std::uint32_t)),
            JoinU32(reader, gameEngine, k_ProbeGameEngineViewValue, k_PovProbeViews, k_ProbeGameEngineViewStride),
            requested, unitText, flagsText, status.Calls, status.Reasserts);
    }

    // return: The tick of the engine, or nothing.
    auto ReadTick(MemoryReaderService& reader, std::uintptr_t threadContext) -> std::optional<std::uint32_t>
    {
        const std::optional<std::uintptr_t> timeGlobals{ ReadAt<std::uintptr_t>(reader, threadContext, k_ThreadContextTimeGlobalsOffset) };
        return ReadAt<std::uint32_t>(reader, timeGlobals, k_TimeGlobalsEngineTickOffset);
    }

    // Gives the base and the size of the image of a loaded module, or zeros when it is not a valid image.
    auto ReadImageRange(HMODULE module, std::uint64_t& base, std::uint64_t& size) -> void
    {
        base = 0;
        size = 0;

        if (module == nullptr) return;

        __try
        {
            const auto* dos{ reinterpret_cast<const IMAGE_DOS_HEADER*>(module) };
            const auto* headers{ reinterpret_cast<const IMAGE_NT_HEADERS*>(reinterpret_cast<const std::uint8_t*>(module) + dos->e_lfanew) };
            if (dos->e_magic != IMAGE_DOS_SIGNATURE || headers->Signature != IMAGE_NT_SIGNATURE) return;

            base = reinterpret_cast<std::uint64_t>(module);
            size = headers->OptionalHeader.SizeOfImage;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            base = 0;
            size = 0;
        }
    }

    // Gives the range of the image of haloreach.dll, or zeros when it is not loaded.
    auto LocateHaloModule(std::uint64_t& base, std::uint64_t& size) -> void
    {
        ReadImageRange(::GetModuleHandleW(L"haloreach.dll"), base, size);
    }

    // Gives the range of the image of this DLL (the module that holds this function), or zeros.
    auto LocateOwnModule(std::uint64_t& base, std::uint64_t& size) -> void
    {
        HMODULE own{ nullptr };
        constexpr DWORD flags{ GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT };

        if (!::GetModuleHandleExW(flags, reinterpret_cast<LPCWSTR>(&LocateOwnModule), &own)) own = nullptr;
        ReadImageRange(own, base, size);
    }

    // return: Whether the address is inside the range.
    auto IsInRange(std::uint64_t address, std::uint64_t base, std::uint64_t size) -> bool
    {
        return base != 0 && address >= base && address < base + size;
    }

    // return: "haloreach+0x<rva>" for an address inside haloreach.dll, "artemis" inside this DLL, else the absolute address.
    // note: The ranges are resolved once when the probe starts: nothing is asked of the system for each event.
    auto DescribeCaller(std::uint64_t address, std::uint64_t haloBase, std::uint64_t haloSize, std::uint64_t ownBase, std::uint64_t ownSize) -> std::string
    {
        if (IsInRange(address, haloBase, haloSize)) return std::format("haloreach+0x{:X}", address - haloBase);
        if (IsInRange(address, ownBase, ownSize)) return "artemis";
        return std::format("0x{:X}", address);
    }

    // return: The line of one noted call, without the prefix.
    auto FormatEvent(const PovProbeEvent& event, const std::string& tick, std::uint64_t frame, const std::string& caller, bool isGameThread) -> std::string
    {
        const std::uint32_t first{ static_cast<std::uint32_t>(event.Args[0]) };
        const std::uint32_t second{ static_cast<std::uint32_t>(event.Args[1]) };

        switch (event.Kind)
        {
        case PovProbeKind::ViewPlayer:
            return std::format("t={} f={} call 53d7c(v={}, p=0x{:X}) from={} game={}", tick, frame, first, second, caller, isGameThread ? 1 : 0);
        case PovProbeKind::ViewUnit:
            return std::format("t={} f={} call 54070(v={}, u=0x{:X}) from={} game={}", tick, frame, first, second, caller, isGameThread ? 1 : 0);
        case PovProbeKind::PlayerUnit:
            return std::format("t={} f={} call 53ca4(p=0x{:X}, u=0x{:X}) from={} game={}", tick, frame, first, second, caller, isGameThread ? 1 : 0);
        case PovProbeKind::PlayerAssignUnit:
            return std::format("t={} f={} call 45814(rcx=0x{:X}, rdx=0x{:X}, r8=0x{:X}, r9=0x{:X}) from={} game={}", tick, frame, event.Args[0],
                event.Args[1], event.Args[2], event.Args[3], caller, isGameThread ? 1 : 0);
        case PovProbeKind::ScreenCreate:
        case PovProbeKind::ScreenCreateAlt:
        case PovProbeKind::ScreenCreateEx:
        case PovProbeKind::ScreenNew:
        case PovProbeKind::ScreenChange:
        case PovProbeKind::ScreenChangePost:
        case PovProbeKind::ScreenEvict:
            break;
        }

        return {};
    }

    // return: The text of one field of a screen (the vtable as the caller is described, signed fields in decimal, the others in hex).
    auto ScreenFieldText(PovProbeScreenField field, std::uint64_t value, std::uint64_t haloBase, std::uint64_t haloSize, std::uint64_t ownBase,
        std::uint64_t ownSize) -> std::string
    {
        switch (field)
        {
        case PovProbeScreenField::Vtable: return std::format("vt={}", DescribeCaller(value, haloBase, haloSize, ownBase, ownSize));
        case PovProbeScreenField::Window: return std::format("win={}", static_cast<std::int32_t>(value));
        case PovProbeScreenField::Mask: return std::format("mask=0x{:X}", value);
        case PovProbeScreenField::Flags: return std::format("flags=0x{:X}", value);
        case PovProbeScreenField::State: return std::format("st={}", value);
        case PovProbeScreenField::Field1AC: return std::format("1ac={}", static_cast<std::int32_t>(value));
        case PovProbeScreenField::Field154: return std::format("154=0x{:X}", value);
        case PovProbeScreenField::Field1C0: return std::format("1c0=0x{:X}", value);
        case PovProbeScreenField::Field1C4: return std::format("1c4=0x{:X}", value);
        }

        return {};
    }

    // return: The line of a new screen (all its fields) or of a change (only the fields in the mask, args[1]), without the prefix.
    auto FormatScreen(const PovProbeEvent& event, const std::string& tick, std::uint64_t frame, std::uint64_t haloBase, std::uint64_t haloSize,
        std::uint64_t ownBase, std::uint64_t ownSize) -> std::string
    {
        const bool isNew{ event.Kind == PovProbeKind::ScreenNew };
        std::string fields{};
        for (std::size_t index{ 0 }; index < k_PovProbeScreenFieldCount && index < event.ExtraCount; ++index)
        {
            if (((event.Args[1] >> index) & 1) == 0) continue;

            fields += ' ';
            fields += ScreenFieldText(static_cast<PovProbeScreenField>(index), event.Extra[index], haloBase, haloSize, ownBase, ownSize);
        }

        const char* suffix{ event.Kind == PovProbeKind::ScreenChangePost ? " (post)" : "" };
        return std::format("t={} f={} screen {} 0x{:X}{}{}", tick, frame, isNew ? "new" : "chg", event.Args[0], fields, suffix);
    }

    // return: The line of a screen creation, without the prefix. stackNumber is 0 when there is none.
    auto FormatCreate(const PovProbeEvent& event, const std::string& tick, std::uint64_t frame, const std::string& caller, bool isGameThread,
        std::uint32_t stackNumber) -> std::string
    {
        const char* name{ event.Kind == PovProbeKind::ScreenCreate ? "2c9efc" : (event.Kind == PovProbeKind::ScreenCreateAlt ? "2c9f88" : "2ca014") };
        const std::string fifth{ event.Kind == PovProbeKind::ScreenCreateEx ? std::format(", a5=0x{:X}", event.Args[3]) : std::string{} };
        const std::string stack{ stackNumber == 0 ? std::string{ "-" } : std::format("{}", stackNumber) };

        return std::format("t={} f={} create {}(win={}, name=0x{:X}, a4=0x{:X}{}) -> 0x{:X} from={} game={} stack=#{}", tick, frame, name,
            static_cast<std::int32_t>(event.Args[0]), event.Args[1], event.Args[2], fifth, event.Result, caller, isGameThread ? 1 : 0, stack);
    }

    // return: The line that writes in full the stack of a screen creation, without the prefix.
    auto FormatStack(const PovProbeEvent& event, std::uint32_t number, std::uint64_t haloBase, std::uint64_t haloSize, std::uint64_t ownBase,
        std::uint64_t ownSize) -> std::string
    {
        std::string frames{};
        for (std::uint32_t index{ 0 }; index < event.ExtraCount; ++index)
        {
            if (index > 0) frames += " < ";
            frames += DescribeCaller(event.Extra[index], haloBase, haloSize, ownBase, ownSize);
        }

        return std::format("stack #{} = {}", number, frames);
    }

    // return: A hash of the return addresses of an event.
    auto HashFrames(const PovProbeEvent& event) -> std::uint64_t
    {
        std::uint64_t hash{ 14695981039346656037ull };
        for (std::uint32_t index{ 0 }; index < event.ExtraCount; ++index)
        {
            hash = (hash ^ event.Extra[index]) * 1099511628211ull;
        }

        return hash;
    }
    // return: Whether the cursor line must be written: the calls went from none to some or back, or with calls the list of this pointers or the
    // value read changed.
    auto IsCursorChanged(const PovProbeCursor& previous, const PovProbeCursor& current) -> bool
    {
        if ((previous.Calls > 0) != (current.Calls > 0)) return true;
        if (current.Calls == 0) return false;

        return previous.ThisCount != current.ThisCount || previous.This != current.This || previous.FirstValue != current.FirstValue;
    }

    // return: The line of the cursor candidate, without the prefix.
    auto FormatCursor(const PovProbeCursor& cursor, const std::string& tick, std::uint64_t frame) -> std::string
    {
        std::string list{};
        for (std::uint32_t index{ 0 }; index < cursor.ThisCount; ++index)
        {
            if (index > 0) list += ',';
            list += std::format("0x{:X}", cursor.This[index]);
        }

        if (list.empty()) list = "-";

        const std::string value{ cursor.FirstValue == k_PovProbeUnreadable ? std::string{ "??" } : std::format("0x{:X}", cursor.FirstValue) };
        return std::format("t={} f={} cursor calls={} this={} p0={}", tick, frame, cursor.Calls, list, value);
    }

    // The handles of the player mapping that the POV needs.
    struct MappingHandles
    {
        std::uint32_t EngineHandle{ k_NoHandle };
        std::uint32_t LocalHandle{ k_NoHandle };
    };

    // return: Whether the mapping was read. Guarded: the thread context can be stale.
    auto ReadMapping(std::uintptr_t threadContext, MappingHandles& out) -> bool
    {
        __try
        {
            const std::uintptr_t mapping{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextMappingOffset) };
            if (mapping == 0) return false;

            out.EngineHandle = *reinterpret_cast<const std::uint32_t*>(mapping + k_OutputUserPlayer + (k_PovUser * k_EntryStride));
            out.LocalHandle = *reinterpret_cast<const std::uint32_t*>(mapping + k_InputUserPlayer + (k_PovUser * k_EntryStride));
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // return: Whether the handle is a player of the table with the same salt as the one in the table. Guarded.
    // note: A handle that outlived a map load is the case this guards. The Viewer sets the observed player at every scene update (none without players
    // in the tick) and the request ages out in k_PovRequestMaxAge, so an old handle lives a few frames at most, and the salt of the entry must match
    // the handle: a new table does not reuse the pair by design, only by chance, and a wrong player in the view is then one frame long.
    auto IsPlayerHandleValid(std::uintptr_t threadContext, std::uint32_t handle) -> bool
    {
        const std::uint32_t index{ handle & k_HandleIndexMask };
        const std::uint32_t salt{ handle >> k_SaltShift };
        if (handle == k_NoHandle || salt == 0 || index >= k_MaxPlayers) return false;

        __try
        {
            const std::uintptr_t telemetryData{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextTelemetryDataOffset) };
            if (telemetryData == 0) return false;

            const std::uintptr_t table{ *reinterpret_cast<const std::uintptr_t*>(telemetryData + k_TelemetryDataPlayerTableOffset) };
            if (table == 0) return false;

            return *reinterpret_cast<const std::uint16_t*>(table + (index * k_Base) + k_Handle) == salt;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // Acknowledges the loadout of a player of the table: writes the byte the UI reads (k_LoadoutAcknowledged) equal to the chosen one, when
    // ShouldAckLoadout says so (VW.9a.x, D345). The handle is resolved here, at every call: no pointer is kept between ticks.
    // return: Whether the byte was written. Guarded: the only access to engine memory that writes, one byte, the same mark the game makes for the
    // player of a controller.
    auto AckLoadout(std::uintptr_t threadContext, std::uint32_t handle) -> bool
    {
        const std::uint32_t index{ handle & k_HandleIndexMask };
        const std::uint32_t salt{ handle >> k_SaltShift };
        if (handle == k_NoHandle || salt == 0 || index >= k_MaxPlayers) return false;

        __try
        {
            const std::uintptr_t mapping{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextMappingOffset) };
            const std::uintptr_t telemetryData{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextTelemetryDataOffset) };
            if (mapping == 0 || telemetryData == 0) return false;

            const std::uintptr_t table{ *reinterpret_cast<const std::uintptr_t*>(telemetryData + k_TelemetryDataPlayerTableOffset) };
            if (table == 0) return false;

            std::array<std::uint32_t, k_MaxLocalUsers> controllerPlayers{};
            for (std::size_t controller{ 0 }; controller < k_MaxLocalUsers; ++controller)
            {
                controllerPlayers[controller] = *reinterpret_cast<const std::uint32_t*>(mapping + k_ControllerPlayer + (controller * k_EntryStride));
            }

            // The entry must still be this player: the guard of the only write is checked here, with no engine code run in between.
            const std::uintptr_t player{ table + (index * k_Base) };
            if (*reinterpret_cast<const std::uint16_t*>(player + k_Handle) != salt) return false;

            const std::uint8_t chosen{ *reinterpret_cast<const std::uint8_t*>(player + k_Loadout) };
            const std::uint8_t acknowledged{ *reinterpret_cast<const std::uint8_t*>(player + k_LoadoutAcknowledged) };
            if (!ShouldAckLoadout(handle, controllerPlayers, chosen, acknowledged)) return false;

            *reinterpret_cast<std::uint8_t*>(player + k_LoadoutAcknowledged) = chosen;
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            return false;
        }
    }

    // return: False if the engine function faulted.
    auto CallAssign(PovAssignFunction assign, std::uint32_t handle) -> bool
    {
        __try
        {
            assign(k_PovUser, handle);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}

namespace Engine::Base::Pov::System
{
    auto DecidePovCall(std::optional<std::uint32_t> requested, std::uint32_t engineCurrent, std::uint32_t local, bool isRequestedValid, bool ownsView)
        -> std::optional<std::uint32_t>
    {
        if (requested && isRequestedValid)
        {
            if (*requested == engineCurrent) return std::nullopt;
            return *requested;
        }

        if (!ownsView || local == k_NoHandle || local == engineCurrent) return std::nullopt;
        return local;
    }

    auto NextAssigned(std::uint32_t previous, std::optional<std::uint32_t> assignedNow, std::uint32_t engineCurrent, std::uint32_t local) -> std::uint32_t
    {
        const std::uint32_t viewAfter{ assignedNow ? *assignedNow : engineCurrent };

        if (local != k_NoHandle && viewAfter == local) return k_NoHandle;
        if (assignedNow) return *assignedNow;
        return previous;
    }

    auto ShouldAckLoadout(std::uint32_t playerHandle, const std::array<std::uint32_t, k_MaxLocalUsers>& controllerPlayers, std::uint8_t chosen,
        std::uint8_t acknowledged) -> bool
    {
        if (playerHandle == k_NoHandle || chosen == acknowledged) return false;
        return std::ranges::find(controllerPlayers, playerHandle) == controllerPlayers.end();
    }

    auto IsPovOnOtherPlayer(const PovStatus& status) -> bool
    {
        return status.RequestedHandle != k_NoHandle && status.EngineHandle == status.RequestedHandle && status.EngineHandle != status.LocalHandle;
    }

    auto PovService::Install() -> bool
    {
        if (m_Assign.load() != nullptr) return true;

        // The window is 0: the prologue must begin exactly at the RVA.
        const std::uintptr_t address{ m_AOBService.FindFunctionAtRva(k_AssignOutputUserPrologue, k_AssignOutputUserRva, k_AssignOutputUserWindow) };

        if (address == 0)
        {
            m_Log.Message("[PovService] ERROR: The view assignment was not found at rva {:X}, the POV is not available.", k_AssignOutputUserRva);
            m_StatusStore.SetInstalled(false);
            return false;
        }

        // A new engine starts with no view of its own: nothing of a previous one (handles, ownership, faults) is kept.
        m_StatusStore.Reset();
        this->ProbeReset();
        m_IsReleased.store(false);
        m_Assign.store(reinterpret_cast<PovAssignFunction>(address));
        m_StatusStore.SetInstalled(true);
        m_Log.Message("[PovService] INFO: The view assignment was found at rva {:X}.", k_AssignOutputUserRva);
        return true;
    }

    auto PovService::Uninstall() -> void
    {
        m_Assign.store(nullptr);
        m_GameThread.store(std::thread::id{});
        m_GameThreadContext.store(0);
        m_StatusStore.Reset();
        this->ProbeReset();
    }

    auto PovService::Disable() -> void
    {
        m_StatusStore.CountFault();

        // The first fault is the one that logs: the exchange tells it.
        if (m_Assign.exchange(nullptr) != nullptr)
        {
            m_Log.Message("[PovService] ERROR: The view assignment faulted, the POV is off until the next engine.");
        }

        m_StatusStore.SetInstalled(false);
    }

    auto PovService::Update(std::uintptr_t threadContext) -> void
    {
        this->UpdateView(threadContext);
        this->Probe(threadContext);
    }

    auto PovService::UpdateView(std::uintptr_t threadContext) -> void
    {
        const PovAssignFunction assign{ m_Assign.load() };
        if (assign == nullptr || threadContext == 0) return;

        // Update runs on the game thread only: Release finds it through these.
        m_GameThread.store(std::this_thread::get_id());
        m_GameThreadContext.store(threadContext);

        MappingHandles mapping{};
        if (!ReadMapping(threadContext, mapping)) return;

        const std::optional<PovRequest> request{ m_IsReleased.load() ? std::nullopt : m_RequestStore.Read(k_PovRequestMaxAge) };
        std::optional<std::uint32_t> requested{};
        if (request && request->PlayerHandle != k_NoHandle) requested = request->PlayerHandle;

        const bool isRequestedValid{ requested && IsPlayerHandleValid(threadContext, *requested) };

        // A request that begins starts the counts over, so the telemetry shows what this one did.
        const PovStatus previous{ m_StatusStore.Read() };
        if (requested && previous.RequestedHandle == k_NoHandle) m_StatusStore.ResetCounts();

        // The view is this service's only while it holds a player it assigned: with none, a view that differs from the local player is somebody
        // else's (the theater, a join in progress) and stays.
        const bool ownsView{ previous.AssignedHandle != k_NoHandle };

        const std::optional<std::uint32_t> target{ DecidePovCall(requested, mapping.EngineHandle, mapping.LocalHandle, isRequestedValid, ownsView) };
        std::optional<std::uint32_t> assignedNow{};

        if (target)
        {
            const bool isRequestedTarget{ isRequestedValid && *target == *requested };

            // It is a reassert when the player was already set by this service and the engine took it away.
            const bool isReassert{ isRequestedTarget && previous.AssignedHandle == *target };

            if (CallAssign(assign, *target))
            {
                m_StatusStore.CountCall(isReassert);
                assignedNow = *target;
            }
            else
            {
                this->Disable();
            }
        }

        const std::uint32_t assigned{ NextAssigned(previous.AssignedHandle, assignedNow, mapping.EngineHandle, mapping.LocalHandle) };
        m_StatusStore.SetHandles(requested.value_or(k_NoHandle), mapping.EngineHandle, mapping.LocalHandle, assigned);

        // The engine view already shows a requested player that is not the local one (not on the tick that assigned it): the UI never
        // acknowledges his loadout (D345), so this does.
        if (isRequestedValid && mapping.EngineHandle == *requested && mapping.EngineHandle != mapping.LocalHandle && AckLoadout(threadContext, mapping.EngineHandle))
        {
            m_StatusStore.CountLoadoutAck();
        }
    }

    auto PovService::Probe(std::uintptr_t threadContext) -> void
    {
        const std::optional<PovRequest> request{ m_IsReleased.load() ? std::nullopt : m_RequestStore.Read(k_PovRequestMaxAge) };
        const std::uint32_t requested{ request ? request->PlayerHandle : k_NoHandle };

        // Who is shown in the view of user 0 and who is the local player, and whether either has no unit.
        PovProbeReason reason{ PovProbeReason::None };
        std::optional<std::uintptr_t> players{};
        PovProbePlayerState view{};
        PovProbePlayerState local{};
        if (threadContext != 0)
        {
            const std::optional<std::uintptr_t> mapping{ ReadAt<std::uintptr_t>(m_Reader, threadContext, k_ThreadContextMappingOffset) };
            const std::uint32_t viewHandle{ ReadAt<std::uint32_t>(m_Reader, mapping, k_OutputUserPlayer + (k_PovUser * k_EntryStride)).value_or(k_NoHandle) };
            const std::uint32_t localHandle{ ReadAt<std::uint32_t>(m_Reader, mapping, k_InputUserPlayer + (k_PovUser * k_EntryStride)).value_or(k_NoHandle) };

            players = ReadPlayerTable(m_Reader, threadContext);
            view = ReadPlayerState(m_Reader, players, viewHandle);
            local = ReadPlayerState(m_Reader, players, localHandle);
            reason = DecideReason(requested != k_NoHandle, view, local);
        }

        if (reason == PovProbeReason::None)
        {
            if (m_IsProbeRunning)
            {
                m_ProbeStore.SetActive(false);
                this->ProbeFlushLosses();
                this->ProbeEmit("inactive");
                m_IsProbeRunning = false;
                m_ProbeReason = PovProbeReason::None;
            }

            return;
        }

        if (!m_IsProbeRunning)
        {
            m_ProbeStore.SetGameThread(::GetCurrentThreadId());
            m_ProbeStore.Discard();
            m_ProbeStore.TakeCursor();
            m_ProbeStore.ResetScreens();
            m_ProbeLastBody.clear();
            m_ProbeLastCursor = PovProbeCursor{};
            m_ProbeLines = 0;
            m_ProbeLinesDropped = 0;
            m_ProbeRingLost = 0;
            m_ProbeStackCount = 0;
            m_IsProbeResponseKnown = false;
            m_ProbeLastTick.reset();
            for (PovProbePlayerCopy& copy : m_ProbeCopies) copy.IsValid = false;
            LocateHaloModule(m_ProbeModuleBase, m_ProbeModuleSize);
            LocateOwnModule(m_ProbeOwnBase, m_ProbeOwnSize);
            m_ProbeStore.SetOwnRange(m_ProbeOwnBase, m_ProbeOwnSize);
            m_ProbeOffThread = 0;
            m_ProbeCreateLost = 0;
            m_ProbeStore.SetActive(true);
            m_IsProbeRunning = true;
            m_ProbeReason = PovProbeReason::None;
        }

        ++m_ProbeFrame;
        const std::optional<std::uint32_t> tickValue{ ReadTick(m_Reader, threadContext) };
        const std::string tick{ tickValue ? std::format("{}", *tickValue) : std::string{ "??" } };

        if (reason != m_ProbeReason)
        {
            m_ProbeReason = reason;
            this->ProbeEmit(std::format("active reason={} t={} f={}", ReasonText(reason), tick, m_ProbeFrame));
        }

        // The hooks read this tick for the screens they follow.
        m_ProbeStore.SetTick(tickValue.value_or(0));
        const std::uint32_t tickDelta{ tickValue && m_ProbeLastTick && *tickValue > *m_ProbeLastTick ? *tickValue - *m_ProbeLastTick : 1 };

        this->ProbeDrain(tick);
        m_ProbeOffThread += m_ProbeStore.TakeOffThread();

        // The screens whose update was not called for a while.
        if (tickValue)
        {
            std::array<PovProbeGoneScreen, k_PovProbeScreenSlots> gone{};
            const std::size_t goneCount{ m_ProbeStore.ExpireScreens(*tickValue, gone) };
            for (std::size_t index{ 0 }; index < goneCount; ++index)
            {
                this->ProbeEmit(std::format("t={} f={} screen gone 0x{:X} calls={} first={} last={}", tick, m_ProbeFrame, gone[index].This, gone[index].Calls,
                    gone[index].FirstTick, gone[index].LastTick));
            }
        }

        const PovProbeCursor cursor{ m_ProbeStore.TakeCursor() };
        if (IsCursorChanged(m_ProbeLastCursor, cursor)) this->ProbeEmit(FormatCursor(cursor, tick, m_ProbeFrame));
        m_ProbeLastCursor = cursor;

        // The snapshot is written only when a field changed since the last one.
        const std::string body{ BuildSnapshot(m_Reader, threadContext, m_StatusStore.Read(), requested) };
        if (body != m_ProbeLastBody)
        {
            m_ProbeLastBody = body;
            this->ProbeEmit(std::format("t={} f={} {}", tick, m_ProbeFrame, body));
        }

        this->ProbeResponse(view, local, tick, tickDelta);
        this->ProbePlayerDiff(players, view, local, tick, tickDelta);
        m_ProbeLastTick = tickValue;
    }

    auto PovService::ProbeDrain(const std::string& tick) -> void
    {
        const std::uint32_t gameThread{ m_ProbeStore.GetGameThread() };

        // The noted events, oldest first, in batches until the ring is empty: first the calls and screens, then the creations (their own ring).
        for (const bool isCreationRing : { false, true })
        {
            for (;;)
            {
                const PovProbeDrain drain{ isCreationRing ? m_ProbeStore.DrainCreations(m_ProbeEvents) : m_ProbeStore.Drain(m_ProbeEvents) };
                (isCreationRing ? m_ProbeCreateLost : m_ProbeRingLost) += drain.Dropped;

                for (std::size_t index{ 0 }; index < drain.Count; ++index)
                {
                    const PovProbeEvent& event{ m_ProbeEvents[index] };
                    const bool isGameThread{ event.ThreadId == gameThread };
                    const std::string caller{ DescribeCaller(event.Caller, m_ProbeModuleBase, m_ProbeModuleSize, m_ProbeOwnBase, m_ProbeOwnSize) };

                    if (event.Kind == PovProbeKind::ScreenNew || event.Kind == PovProbeKind::ScreenChange || event.Kind == PovProbeKind::ScreenChangePost)
                    {
                        this->ProbeEmit(FormatScreen(event, tick, m_ProbeFrame, m_ProbeModuleBase, m_ProbeModuleSize, m_ProbeOwnBase, m_ProbeOwnSize));
                        continue;
                    }

                    if (event.Kind == PovProbeKind::ScreenEvict)
                    {
                        this->ProbeEmit(std::format("t={} f={} screen evict 0x{:X} total={}", tick, m_ProbeFrame, event.Args[0], event.Args[1]));
                        continue;
                    }

                    if (isCreationRing)
                    {
                        bool isNewStack{ false };
                        const std::uint32_t number{ this->ProbeNoteStack(event, isNewStack) };
                        this->ProbeEmit(FormatCreate(event, tick, m_ProbeFrame, caller, isGameThread, number));

                        // The line of a stack is written once, so it never goes through the limit of lines per second.
                        if (isNewStack)
                        {
                            this->ProbeEmit(FormatStack(event, number, m_ProbeModuleBase, m_ProbeModuleSize, m_ProbeOwnBase, m_ProbeOwnSize), true);
                        }

                        continue;
                    }

                    const std::string line{ FormatEvent(event, tick, m_ProbeFrame, caller, isGameThread) };
                    if (!line.empty()) this->ProbeEmit(line);
                }

                if (drain.Count < m_ProbeEvents.size()) break;
            }
        }
    }

    auto PovService::ProbeNoteStack(const PovProbeEvent& event, bool& isNew) -> std::uint32_t
    {
        isNew = false;
        if (event.ExtraCount == 0) return 0;

        const std::uint64_t hash{ HashFrames(event) };
        for (std::size_t index{ 0 }; index < m_ProbeStackCount; ++index)
        {
            if (m_ProbeStackHashes[index] == hash) return static_cast<std::uint32_t>(index + 1);
        }

        if (m_ProbeStackCount >= m_ProbeStackHashes.size()) return 0;

        m_ProbeStackHashes[m_ProbeStackCount++] = hash;
        isNew = true;
        return static_cast<std::uint32_t>(m_ProbeStackCount);
    }

    auto PovService::ProbeResponse(const PovProbePlayerState& view, const PovProbePlayerState& local, const std::string& tick, std::uint32_t tickDelta) -> void
    {
        const std::string controllers{ ControllersText(m_Reader, m_ProbeModuleBase) };

        const bool isChanged{ !m_IsProbeResponseKnown || controllers != m_ProbeLastControllers || IsResponseChanged(m_ProbeLastView, view, tickDelta) ||
            IsResponseChanged(m_ProbeLastLocal, local, tickDelta) };

        m_IsProbeResponseKnown = true;
        m_ProbeLastView = view;
        m_ProbeLastLocal = local;
        m_ProbeLastControllers = controllers;

        if (!isChanged) return;

        this->ProbeEmit(std::format("t={} f={} resp {} {} ctrl={}", tick, m_ProbeFrame, ResponseText("view", view), ResponseText("local", local), controllers));
    }

    auto PovService::ProbePlayerDiff(std::optional<std::uintptr_t> players, const PovProbePlayerState& view, const PovProbePlayerState& local,
        const std::string& tick, std::uint32_t tickDelta) -> void
    {
        const std::array<std::uint32_t, k_PovProbePlayerRoles> handles{ view.Handle, local.Handle };

        for (std::size_t role{ 0 }; role < handles.size(); ++role)
        {
            PovProbePlayerCopy& copy{ m_ProbeCopies[role] };
            const std::optional<std::uintptr_t> address{ PlayerAddress(players, handles[role]) };

            // The same player twice is one line: the second role keeps no copy until it is another player.
            std::array<std::uint32_t, k_PovProbePlayerWords> words{};
            const bool isRead{ address && !(role > 0 && handles[role] == handles[0]) && m_Reader.ReadRaw(*address, words.data(), sizeof(words)) };
            if (!isRead)
            {
                copy.IsValid = false;
                continue;
            }

            if (copy.IsValid && copy.Handle == handles[role])
            {
                const std::string diff{ DiffText(copy, words, tickDelta) };
                if (!diff.empty()) this->ProbeEmit(std::format("t={} f={} pdiff {:X} {}", tick, m_ProbeFrame, handles[role], diff));
            }

            copy.IsValid = true;
            copy.Handle = handles[role];
            copy.Words = words;
        }
    }
    auto PovService::ProbeEmit(const std::string& text, bool isForced) -> void
    {
        if (isForced)
        {
            m_Log.Message("[PovProbe] INFO: {}", text);
            return;
        }

        const steady_clock::time_point now{ steady_clock::now() };

        // A new second of the clock: say how many lines the last one dropped and start the count over.
        if (now - m_ProbeWindowStart >= seconds{ 1 })
        {
            this->ProbeFlushLosses();
            m_ProbeWindowStart = now;
            m_ProbeLines = 0;
        }

        if (m_ProbeLines >= k_PovProbeMaxLinesPerSecond)
        {
            ++m_ProbeLinesDropped;
            return;
        }

        ++m_ProbeLines;
        m_Log.Message("[PovProbe] INFO: {}", text);
    }

    auto PovService::ProbeFlushLosses() -> void
    {
        if (m_ProbeRingLost > 0) m_Log.Message("[PovProbe] INFO: ring lost {}", m_ProbeRingLost);
        if (m_ProbeCreateLost > 0) m_Log.Message("[PovProbe] INFO: create lost {}", m_ProbeCreateLost);
        if (m_ProbeOffThread > 0) m_Log.Message("[PovProbe] INFO: screen offthread {}", m_ProbeOffThread);
        if (m_ProbeLinesDropped > 0) m_Log.Message("[PovProbe] INFO: lines dropped {}", m_ProbeLinesDropped);
        m_ProbeRingLost = 0;
        m_ProbeCreateLost = 0;
        m_ProbeOffThread = 0;
        m_ProbeLinesDropped = 0;
    }

    auto PovService::ProbeReset() -> void
    {
        m_ProbeStore.SetActive(false);
        m_IsProbeRunning = false;
        m_ProbeModuleBase = 0;
        m_ProbeModuleSize = 0;
        m_ProbeOwnBase = 0;
        m_ProbeOwnSize = 0;
        m_ProbeLastBody.clear();
        m_ProbeLastCursor = PovProbeCursor{};
        m_ProbeLines = 0;
        m_ProbeLinesDropped = 0;
        m_ProbeRingLost = 0;
        m_ProbeWindowStart = {};
        m_ProbeReason = PovProbeReason::None;
        m_ProbeCreateLost = 0;
        m_ProbeOffThread = 0;
        m_ProbeStackCount = 0;
        m_IsProbeResponseKnown = false;
        m_ProbeLastView = PovProbePlayerState{};
        m_ProbeLastLocal = PovProbePlayerState{};
        m_ProbeLastControllers.clear();
        m_ProbeLastTick.reset();
        for (PovProbePlayerCopy& copy : m_ProbeCopies) copy.IsValid = false;
    }

    auto PovService::RestoreView(std::uintptr_t threadContext) -> void
    {
        const PovAssignFunction assign{ m_Assign.load() };
        if (assign == nullptr) return;

        MappingHandles mapping{};
        if (!ReadMapping(threadContext, mapping)) return;

        if (mapping.LocalHandle != k_NoHandle && mapping.LocalHandle != mapping.EngineHandle)
        {
            if (!CallAssign(assign, mapping.LocalHandle))
            {
                this->Disable();
                return;
            }

            m_Log.Message("[PovService] INFO: The view went back to the local player.");
        }

        m_StatusStore.SetHandles(k_NoHandle, mapping.LocalHandle, mapping.LocalHandle, k_NoHandle);
    }

    auto PovService::Release(std::chrono::milliseconds timeout) -> void
    {
        if (m_Assign.load() == nullptr) return;

        // First: from now on Update ignores the request, so it cannot take the view again while this checks the ownership.
        m_IsReleased.store(true);

        if (m_StatusStore.Read().AssignedHandle == k_NoHandle) return;

        // On the game thread nobody else can serve the return, so it is done here with the context of the last Update, and it never waits.
        if (std::this_thread::get_id() == m_GameThread.load())
        {
            const std::uintptr_t threadContext{ m_GameThreadContext.load() };
            if (threadContext != 0) this->RestoreView(threadContext);
            return;
        }

        // Another thread: Update does it, in a frame, once it ignores the request.
        const auto deadline{ std::chrono::steady_clock::now() + timeout };
        while (m_Assign.load() != nullptr && m_StatusStore.Read().AssignedHandle != k_NoHandle)
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                m_Log.Message("[PovService] WARNING: The game thread did not give the view back to the local player in time.");
                return;
            }

            std::this_thread::sleep_for(k_PovReleasePoll);
        }
    }
}