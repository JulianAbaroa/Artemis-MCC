module Platform.Lifecycle.Hook;
import :EngineInterface;

import Platform.Memory.Type;
import Platform.Host.Type;
import Platform.Lifecycle.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using std::chrono::milliseconds;

    using Platform::Host::Type::k_HostModuleName;
    using Platform::Lifecycle::Type::k_EngineGlobalDisplacementOffset;
    using Platform::Lifecycle::Type::k_EngineGlobalNextInstructionOffset;
    using Platform::Lifecycle::Type::k_EngineVTableSize;
    using Platform::Lifecycle::Type::k_EngineDestroyIndex;
    using Platform::Lifecycle::Type::k_EngineStartIndex;
    using Platform::Lifecycle::Type::k_EngineUpdateStateIndex;
    using Platform::Lifecycle::Type::k_EngineCommandIndex;
    using Platform::Lifecycle::Type::k_EngineStatePayloads;
    using Platform::Lifecycle::Type::k_EngineStatePayloadMaxSize;
    using Platform::Lifecycle::Type::k_EngineFirstTextState;
    using Platform::Lifecycle::Type::k_EngineLastTextState;

    // Longest string read from a payload or a text command, and the bytes read at once while looking for its end.
    constexpr std::size_t k_MaxTextLength{ 256 };
    constexpr std::size_t k_TextChunkSize{ 16 };

    // Longest wait for the calls in flight when the hook is removed.
    constexpr milliseconds k_DrainTimeout{ 500 };

    // return: Bytes of the payload of the state, 0 if it carries none.
    auto PayloadSize(std::int32_t state) -> std::size_t
    {
        for (const auto& entry : k_EngineStatePayloads)
        {
            if (entry.State == state) return entry.Size;
        }

        return 0;
    }

    // return: The bytes as hex pairs separated by spaces.
    auto ToHex(std::span<const std::byte> bytes) -> std::string
    {
        std::string text{};

        for (const std::byte value : bytes)
        {
            if (!text.empty()) text += ' ';
            text += std::format("{:02X}", static_cast<unsigned>(value));
        }

        return text;
    }
}

namespace Platform::Lifecycle::Hook
{
    auto __fastcall EngineInterfaceDetour::HookedLoadGameEngine(void* gameLaunch, std::int32_t game) -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        s_OriginalFunction(gameLaunch, game);

        auto* self{ s_Instance };
        void* engine{ *self->m_EngineGlobal };

        if (engine != nullptr) self->Trace(engine, game);
    }

    auto __fastcall EngineInterfaceDetour::HookedEngineDestroy(void* engine) -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };

        {
            std::lock_guard<std::mutex> lock{ self->m_StateMutex };
            self->FlushRepeats();
        }

        {
            std::lock_guard<std::mutex> lock{ self->m_TraceMutex };
            if (self->m_TracedEngine == engine) self->m_TracedEngine = nullptr;
        }

        self->m_LogsService.Message("[EngineInterfaceDetour] INFO: Engine {:016X} destroyed.",
            reinterpret_cast<std::uintptr_t>(engine));

        reinterpret_cast<EngineDestroyFunction>(s_OriginalVTable[k_EngineDestroyIndex])(engine);
    }

    auto __fastcall EngineInterfaceDetour::HookedEngineStart(void* engine, void* systemInterface,
        const std::byte* configuration) -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };

        {
            std::lock_guard<std::mutex> lock{ self->m_StateMutex };
            self->FlushRepeats();
            self->m_LastState = -1;
        }

        self->m_LogsService.Message("[EngineInterfaceDetour] INFO: Engine {:016X} start, system interface {:016X},"
            " configuration {:016X}.", reinterpret_cast<std::uintptr_t>(engine),
            reinterpret_cast<std::uintptr_t>(systemInterface), reinterpret_cast<std::uintptr_t>(configuration));

        // The configuration of MCC is never written: if the transplant goes wrong, the next match of MCC is the normal one. The engine
        // copies the configuration it receives to its runtime configuration before the call returns (A15 sec. 2.5), so the transplanted
        // copy only lives for the call.
        const std::byte* startConfiguration{ configuration };
        std::vector<std::byte> transplanted{};

        if (configuration != nullptr)
        {
            // An exception must not reach MCC. The start then gets the configuration of MCC.
            try
            {
                self->m_MatchConfigService.Record(configuration);

                if (self->m_MatchConfigService.Transplant(configuration, transplanted)) startConfiguration = transplanted.data();
            }
            catch (const std::exception& exception)
            {
                startConfiguration = configuration;
                self->m_LogsService.Message("[EngineInterfaceDetour] ERROR: Failed to record or transplant the configuration ({}).",
                    exception.what());
            }
        }

        reinterpret_cast<EngineStartFunction>(s_OriginalVTable[k_EngineStartIndex])(engine, systemInterface, startConfiguration);
    }

    auto __fastcall EngineInterfaceDetour::HookedEngineUpdateState(void* engine, std::int32_t state, const void* payload) -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        s_Instance->LogState(state, payload);

        reinterpret_cast<EngineUpdateStateFunction>(s_OriginalVTable[k_EngineUpdateStateIndex])(engine, state, payload);
    }

    auto __fastcall EngineInterfaceDetour::HookedEngineCommand(void* engine, const char* command) -> void
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        auto* self{ s_Instance };

        {
            std::lock_guard<std::mutex> lock{ self->m_StateMutex };
            self->FlushRepeats();
        }

        const std::string text{ command != nullptr ? self->ReadText(reinterpret_cast<std::uintptr_t>(command)) : std::string{} };

        self->m_LogsService.Message("[EngineInterfaceDetour] INFO: Text command \"{}\".", text);

        reinterpret_cast<EngineCommandFunction>(s_OriginalVTable[k_EngineCommandIndex])(engine, command);
    }

    auto EngineInterfaceDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        const std::uintptr_t match{ m_AOBService.FindPattern(Signature::LoadGameEngine, k_HostModuleName) };

        if (match == 0)
        {
            m_LogsService.Message("[EngineInterfaceDetour] ERROR: Failed to find the function that creates the engine.");
            return false;
        }

        std::int32_t displacement{ 0 };
        std::memcpy(&displacement, reinterpret_cast<const void*>(match + k_EngineGlobalDisplacementOffset), sizeof(displacement));

        m_EngineGlobal = reinterpret_cast<void**>(static_cast<std::intptr_t>(match + k_EngineGlobalNextInstructionOffset) + displacement);

        void* functionAddress{ reinterpret_cast<void*>(match) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedLoadGameEngine),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[EngineInterfaceDetour]", m_LogsService))
        {
            return false;
        }

        // An engine created before the hook went in is not traced: MCC only creates it when a game starts, after Artemis loads.
        m_IsHookInstalled.store(true);
        return true;
    }

    auto EngineInterfaceDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        // No engine gets traced after the first drain, so the second one only waits for the calls that entered through the copy.
        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        this->Untrace();
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[EngineInterfaceDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }

    auto EngineInterfaceDetour::Trace(void* engine, std::int32_t game) -> void
    {
        std::lock_guard<std::mutex> lock{ m_TraceMutex };

        auto** vtableSlot{ static_cast<void***>(engine) };
        void** vtable{ *vtableSlot };

        if (vtable == s_TracedVTable.data()) return;

        if (m_TracedEngine != nullptr && m_TracedEngine != engine)
        {
            m_LogsService.Message("[EngineInterfaceDetour] WARNING: Engine {:016X} was freed without the interface.",
                reinterpret_cast<std::uintptr_t>(m_TracedEngine));
        }

        s_OriginalVTable = vtable;
        std::copy_n(vtable, k_EngineVTableSize, s_TracedVTable.begin());

        s_TracedVTable[k_EngineDestroyIndex] = reinterpret_cast<void*>(&HookedEngineDestroy);
        s_TracedVTable[k_EngineStartIndex] = reinterpret_cast<void*>(&HookedEngineStart);
        s_TracedVTable[k_EngineUpdateStateIndex] = reinterpret_cast<void*>(&HookedEngineUpdateState);
        s_TracedVTable[k_EngineCommandIndex] = reinterpret_cast<void*>(&HookedEngineCommand);

        m_TracedEngine = engine;
        *vtableSlot = s_TracedVTable.data();

        m_LogsService.Message("[EngineInterfaceDetour] INFO: Engine {:016X} of game {} created, vtable {:016X} traced.",
            reinterpret_cast<std::uintptr_t>(engine), game, reinterpret_cast<std::uintptr_t>(vtable));
    }

    auto EngineInterfaceDetour::Untrace() -> void
    {
        std::lock_guard<std::mutex> lock{ m_TraceMutex };

        void* engine{ m_TracedEngine };
        m_TracedEngine = nullptr;

        // An engine that is no longer in the global of MCC may be freed: it is not touched.
        if (engine == nullptr || *m_EngineGlobal != engine) return;

        auto** vtableSlot{ static_cast<void***>(engine) };

        if (*vtableSlot == s_TracedVTable.data()) *vtableSlot = s_OriginalVTable;
    }

    auto EngineInterfaceDetour::LogState(std::int32_t state, const void* payload) -> void
    {
        const std::size_t size{ payload != nullptr ? PayloadSize(state) : 0 };

        std::array<std::byte, k_EngineStatePayloadMaxSize> bytes{};
        if (size > 0) std::memcpy(bytes.data(), payload, size);

        std::lock_guard<std::mutex> lock{ m_StateMutex };

        if (state == m_LastState && bytes == m_LastPayload)
        {
            ++m_RepeatCount;
            return;
        }

        this->FlushRepeats();

        m_LastState = state;
        m_LastPayload = bytes;

        const std::string hex{ ToHex(std::span<const std::byte>(bytes.data(), size)) };

        if (size == sizeof(std::uintptr_t) && state >= k_EngineFirstTextState && state <= k_EngineLastTextState)
        {
            std::uintptr_t address{ 0 };
            std::memcpy(&address, bytes.data(), sizeof(address));

            m_LogsService.Message("[EngineInterfaceDetour] INFO: State 0x{:02X}, payload {}, text \"{}\".",
                state, hex, this->ReadText(address));
            return;
        }

        m_LogsService.Message("[EngineInterfaceDetour] INFO: State 0x{:02X}, payload [{}].", state, hex);
    }

    auto EngineInterfaceDetour::FlushRepeats() -> void
    {
        if (m_RepeatCount == 0) return;

        m_LogsService.Message("[EngineInterfaceDetour] INFO: State 0x{:02X} repeated {} more times.", m_LastState, m_RepeatCount);

        m_RepeatCount = 0;
    }

    auto EngineInterfaceDetour::ReadText(std::uintptr_t address) -> std::string
    {
        std::string text{};
        std::array<char, k_TextChunkSize> chunk{};

        while (address != 0 && text.size() < k_MaxTextLength)
        {
            if (!m_MemoryReaderService.ReadRaw(address + text.size(), chunk.data(), chunk.size())) break;

            const auto end{ std::find(chunk.begin(), chunk.end(), '\0') };
            text.append(chunk.begin(), end);

            if (end != chunk.end()) break;
        }

        return text;
    }
}